"""Local incremental Qwen+FTS/RRF+BGE retrieval. Never a correctness oracle."""
import argparse
import contextlib
import hashlib
import importlib.metadata
import json
import re
import sqlite3
from pathlib import Path
import urllib.request
import urllib.parse

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
RUNTIME = ROOT / ".ai" / "retrieval"
EMBEDDING_DIMENSIONS = 2560
CONFIG = json.loads((HERE / 'config.json').read_text(encoding='utf-8-sig'))
if urllib.parse.urlparse(CONFIG['ollama_url']).hostname not in {'127.0.0.1', 'localhost', '::1'}:
    raise RuntimeError('Source embeddings must stay on the local Ollama service')


class StaleIndex(RuntimeError):
    """Source content changed; cached embeddings can be refreshed incrementally."""


def digest(value):
    return hashlib.sha256(value).hexdigest()


def api(route, payload=None):
    request = urllib.request.Request(CONFIG['ollama_url'] + route,
        data=None if payload is None else json.dumps(payload).encode(),
        headers={'Content-Type': 'application/json'})
    with urllib.request.urlopen(request, timeout=600) as response:
        return json.load(response)


def verify_model():
    models = api('/api/tags')['models']
    if not any(model['name'] == CONFIG['embedding_model'] and model['digest'] == CONFIG['embedding_digest'] for model in models):
        raise RuntimeError('Exact Qwen model unavailable/changed; explicit index review required')


def embed(texts, query=False):
    import numpy as np
    if query:
        texts = ['Instruct: Retrieve relevant C++ engine source code for the given query.\nQuery: ' + text for text in texts]
    result = api('/api/embed', {'model': CONFIG['embedding_model'], 'input': texts,
        'truncate': False, 'keep_alive': 0 if query else '2m',
        'options': {'num_ctx': 4096, 'num_batch': CONFIG.get('ollama_num_batch', 64)}})
    if 'error' in result: raise RuntimeError(result['error'])
    values = np.asarray(result['embeddings'], dtype=np.float32)
    if values.shape != (len(texts), EMBEDDING_DIMENSIONS) or not np.isfinite(values).all():
        raise RuntimeError('Invalid/incomplete embedding response')
    norm = np.linalg.norm(values, axis=1, keepdims=True)
    if (norm == 0).any(): raise RuntimeError('Zero embedding vector')
    return values / norm


def files():
    for directory in CONFIG['roots']:
        base = ROOT / directory
        for path in sorted(base.rglob('*')):
            if path.is_file() and path.suffix.lower() in CONFIG['suffixes'] and not any(part in CONFIG['excluded_parts'] for part in path.relative_to(ROOT).parts):
                yield path


def chunks(text):
    lines = text.splitlines(keepends=True)
    begin = 0
    while begin < len(lines):
        end, size = begin, 0
        while end < len(lines) and (size < CONFIG['chunk_chars'] or end == begin):
            size += len(lines[end]); end += 1
        yield begin + 1, end, ''.join(lines[begin:end])
        if end == len(lines): break
        begin = max(begin + 1, end - CONFIG['overlap_lines'])


def index_identity():
    keys = ('embedding_model', 'embedding_digest', 'roots', 'suffixes',
            'excluded_parts', 'chunk_chars', 'overlap_lines')
    return digest(json.dumps({key: CONFIG[key] for key in keys}, sort_keys=True).encode())


def readiness():
    """Read-only prerequisites, never inference, indexing or model downloads."""
    result = {'status': 'NOT_READY', 'inference': 'NOT_TESTED'}
    missing = []
    for package in ('numpy', 'torch', 'transformers', 'huggingface-hub', 'sentencepiece', 'safetensors'):
        try:
            importlib.metadata.version(package)
        except importlib.metadata.PackageNotFoundError:
            missing.append(package)
    result['dependencies'] = {'status': 'MISSING' if missing else 'AVAILABLE', 'missing': missing}
    index = RUNTIME / 'index.sqlite'
    result['index'] = {'status': 'MISSING'}
    if index.is_file():
        try:
            with contextlib.closing(sqlite3.connect(index.resolve().as_uri() + '?mode=ro', uri=True)) as db:
                prior = db.execute("SELECT value FROM metadata WHERE key='config'").fetchone()
                if not prior or prior[0] != index_identity():
                    raise RuntimeError('Incompatible index identity; explicit index review required')
                count = check(db)
            result['index'] = {'status': 'CURRENT', 'files': count}
        except StaleIndex as exc:
            result['index'] = {'status': 'STALE', 'detail': str(exc)}
        except Exception as exc:
            result['index'] = {'status': 'INVALID_OR_STALE', 'detail': str(exc)}
    model = RUNTIME / 'models' / 'bge-reranker-v2-m3'
    required = ('config.json', 'tokenizer_config.json', 'model.safetensors')
    absent = [name for name in required if not (model / name).is_file()]
    if not any((model / name).is_file() for name in ('tokenizer.json', 'sentencepiece.bpe.model', 'spiece.model')):
        absent.append('tokenizer vocabulary')
    result['reranker'] = {'status': 'MISSING', 'missing': absent, 'inference': 'NOT_TESTED'}
    if not absent:
        try:
            from safetensors import safe_open
            for name in ('config.json', 'tokenizer_config.json', 'tokenizer.json'):
                path = model / name
                if path.is_file() and not isinstance(json.loads(path.read_text(encoding='utf-8')), dict):
                    raise RuntimeError('Invalid model JSON: ' + name)
            with safe_open(model / 'model.safetensors', framework='np') as weights:
                if not weights.keys():
                    raise RuntimeError('Empty reranker weights')
            result['reranker']['status'] = 'LOCAL_FILES_VALIDATED'
        except Exception as exc:
            result['reranker'].update(status='INVALID', detail=str(exc))
    try:
        with urllib.request.urlopen(CONFIG['ollama_url'] + '/api/tags', timeout=2) as response:
            models = json.load(response)['models']
        exact = any(item['name'] == CONFIG['embedding_model'] and
                    item['digest'] == CONFIG['embedding_digest'] for item in models)
        result['embedding'] = {'status': 'AVAILABLE' if exact else 'MISSING_OR_DIFFERENT',
                               'inference': 'NOT_TESTED'}
    except Exception as exc:
        result['embedding'] = {'status': 'UNAVAILABLE', 'detail': str(exc)}
    try:
        import client
        health = client.request('GET', '/health', timeout=2)
        same_root = Path(health.get('root', '')).resolve() == ROOT.resolve()
        result['daemon'] = {'status': 'READY' if same_root and health.get('status') == 'READY'
                            and not health.get('dirty', True) else 'WRONG_ROOT_OR_STALE',
                            'ownership': 'EXTERNAL_NOT_USED_BY_SEARCH'}
    except Exception as exc:
        result['daemon'] = {'status': 'UNAVAILABLE', 'detail': str(exc)}
    if (not missing and result['index']['status'] == 'CURRENT'
            and result['reranker']['status'] == 'LOCAL_FILES_VALIDATED'
            and result['embedding']['status'] == 'AVAILABLE'):
        result['status'] = 'PREREQUISITES_READY'
    return result


def database():
    RUNTIME.mkdir(parents=True, exist_ok=True)
    db = sqlite3.connect(
        RUNTIME / 'index.sqlite',
        check_same_thread=False,
    )
    db.executescript('''
        PRAGMA journal_mode=WAL;
        CREATE TABLE IF NOT EXISTS metadata(key TEXT PRIMARY KEY, value TEXT NOT NULL);
        CREATE TABLE IF NOT EXISTS files(path TEXT PRIMARY KEY, sha TEXT NOT NULL);
        CREATE TABLE IF NOT EXISTS chunks(id INTEGER PRIMARY KEY, path TEXT NOT NULL, start INTEGER NOT NULL, end INTEGER NOT NULL, sha TEXT NOT NULL, text TEXT NOT NULL, vector BLOB NOT NULL);
        CREATE INDEX IF NOT EXISTS chunk_path ON chunks(path);
        CREATE TABLE IF NOT EXISTS embeddings(sha TEXT PRIMARY KEY, vector BLOB NOT NULL);
        CREATE VIRTUAL TABLE IF NOT EXISTS lexical USING fts5(path,text,content='chunks',content_rowid='id');
        CREATE TRIGGER IF NOT EXISTS chunks_ai AFTER INSERT ON chunks BEGIN INSERT INTO lexical(rowid,path,text) VALUES(new.id,new.path,new.text); END;
        CREATE TRIGGER IF NOT EXISTS chunks_ad AFTER DELETE ON chunks BEGIN INSERT INTO lexical(lexical,rowid,path,text) VALUES('delete',old.id,old.path,old.text); END;
    ''')
    # Only settings that affect persisted chunks/vectors belong to the
    # index identity. Runtime/query/daemon settings must not invalidate
    # an otherwise compatible semantic index.
    identity = index_identity()

    prior = db.execute(
        "SELECT value FROM metadata WHERE key='config'"
    ).fetchone()

    if prior and prior[0] != identity:
        # Migration from the previous schema, which incorrectly hashed
        # the entire config including query/runtime settings.
        legacy_config = {
            'schema_version': 1,
            'embedding_model': CONFIG['embedding_model'],
            'embedding_digest': CONFIG['embedding_digest'],
            'ollama_url': CONFIG['ollama_url'],
            'reranker_model': CONFIG['reranker_model'],
            'reranker_revision': CONFIG['reranker_revision'],
            'reranker_device': CONFIG['reranker_device'],
            'roots': CONFIG['roots'],
            'suffixes': CONFIG['suffixes'],
            'excluded_parts': CONFIG['excluded_parts'],
            'chunk_chars': CONFIG['chunk_chars'],
            'overlap_lines': CONFIG['overlap_lines'],
            'candidate_count': CONFIG['candidate_count'],
            'rerank_count': CONFIG['rerank_count'],
            'embedding_batch': CONFIG['embedding_batch'],
        }

        legacy_identity = digest(
            json.dumps(legacy_config, sort_keys=True).encode()
        )

        if prior[0] == legacy_identity:
            db.execute(
                "UPDATE metadata SET value=? WHERE key='config'",
                (identity,),
            )
            db.commit()
        else:
            raise RuntimeError(
                'Index configuration changed in an embedding-affecting way; '
                'explicit index rebuild required'
            )

    db.execute(
        "INSERT OR IGNORE INTO metadata VALUES('config',?)",
        (identity,),
    )
    db.commit()
    return db


def build(db):
    verify_model()
    current, embedded, updated = set(), 0, 0
    try:
        for path in files():
            relative = path.relative_to(ROOT).as_posix(); current.add(relative)
            raw = path.read_bytes(); sha = digest(raw)
            old = db.execute('SELECT sha FROM files WHERE path=?', (relative,)).fetchone()
            if old and old[0] == sha: continue
            records = []
            for start, end, text in chunks(raw.decode('utf-8-sig')):
                value = relative + '\n' + text; key = digest(value.encode())
                cached = db.execute('SELECT vector FROM embeddings WHERE sha=?', (key,)).fetchone()
                records.append([relative, start, end, key, text, None if cached is None else cached[0], value])
            missing = [record for record in records if record[5] is None]
            for offset in range(0, len(missing), CONFIG['embedding_batch']):
                batch = missing[offset:offset + CONFIG['embedding_batch']]
                for record, vector in zip(batch, embed([record[6] for record in batch])):
                    record[5] = vector.tobytes()
                    db.execute('INSERT OR REPLACE INTO embeddings VALUES(?,?)', (record[3], record[5]))
                    embedded += 1
                db.commit()  # Content cache survives interruption; file admission is atomic below.
            with db:
                db.execute('DELETE FROM chunks WHERE path=?', (relative,))
                db.executemany('INSERT INTO chunks(path,start,end,sha,text,vector) VALUES(?,?,?,?,?,?)', [record[:6] for record in records])
                db.execute('INSERT OR REPLACE INTO files VALUES(?,?)', (relative, sha))
            updated += 1
            print(json.dumps({'indexed': relative, 'updated_files': updated, 'new_embeddings': embedded}), flush=True)
        for stale in set(row[0] for row in db.execute('SELECT path FROM files')) - current:
            with db:
                db.execute('DELETE FROM chunks WHERE path=?', (stale,)); db.execute('DELETE FROM files WHERE path=?', (stale,))
    finally:
        if embedded:
            api('/api/embed', {'model': CONFIG['embedding_model'], 'input': [], 'keep_alive': 0})
    print(json.dumps({'status': 'PASS', 'files': len(current), 'chunks': db.execute('SELECT count(*) FROM chunks').fetchone()[0], 'updated_files': updated, 'new_embeddings': embedded}), flush=True)


def check(db):
    import numpy as np
    indexed = dict(db.execute('SELECT path,sha FROM files'))
    actual = {}
    expected_count = 0
    for path in files():
        relative = path.relative_to(ROOT).as_posix()
        raw = path.read_bytes(); actual[relative] = digest(raw)
        if indexed.get(relative) != actual[relative]:
            raise StaleIndex('Semantic source changed: ' + relative)
        expected = [(start, end, digest((relative + '\n' + text).encode()), text)
                    for start, end, text in chunks(raw.decode('utf-8-sig'))]
        records = db.execute('SELECT start,end,sha,text,vector FROM chunks WHERE path=? ORDER BY start,end,id', (relative,)).fetchall()
        if [record[:4] for record in records] != expected:
            raise RuntimeError('Incomplete/duplicate/stale semantic chunks: ' + relative)
        for record in records:
            vector = np.frombuffer(record[4], dtype=np.float32)
            if vector.shape != (EMBEDDING_DIMENSIONS,) or not np.isfinite(vector).all() or abs(float(np.linalg.norm(vector)) - 1.0) > 1e-4:
                raise RuntimeError('Invalid semantic vector: ' + relative)
        expected_count += len(expected)
    if indexed != actual: raise StaleIndex('Semantic index stale/missing; run retrieve.py build incrementally')
    if not indexed: raise RuntimeError('Empty semantic corpus')
    if db.execute('SELECT count(*) FROM chunks').fetchone()[0] != expected_count:
        raise RuntimeError('Orphan semantic chunks')
    if db.execute('PRAGMA integrity_check').fetchone()[0] != 'ok': raise RuntimeError('Index integrity failure')
    return len(indexed)


def rerank(query, records):
    import torch
    from transformers import AutoModelForSequenceClassification, AutoTokenizer
    model_path = RUNTIME / 'models' / 'bge-reranker-v2-m3'
    tokenizer = AutoTokenizer.from_pretrained(model_path, local_files_only=True, trust_remote_code=False)
    model = AutoModelForSequenceClassification.from_pretrained(model_path, local_files_only=True,
        trust_remote_code=False, use_safetensors=True).to('cpu').eval()
    scores = []
    with torch.inference_mode():
        for start in range(0, len(records), 4):
            batch = records[start:start + 4]
            inputs = tokenizer([(query, record[1] + '\n' + record[4]) for record in batch], padding=True, truncation=True, max_length=512, return_tensors='pt')
            scores.extend(model(**inputs).logits.view(-1).float().tolist())
    if len(scores) != len(records) or any(not __import__('math').isfinite(score) for score in scores):
        raise RuntimeError('Invalid reranker output')
    return scores


def query(db, text, top):
    import numpy as np
    check(db); verify_model()
    records = db.execute('SELECT id,path,start,end,text,vector FROM chunks ORDER BY path,start,end').fetchall()
    matrix = np.vstack([np.frombuffer(record[5], dtype=np.float32) for record in records])
    vector = embed([text], query=True)[0]
    dense = np.argsort(-(matrix @ vector), kind='stable')[:CONFIG['candidate_count']]
    tokens = re.findall(r'[A-Za-z_][A-Za-z_0-9]*', text)
    tokens += re.findall(r'[A-Z]?[a-z]+|[A-Z]+(?![a-z])', text)
    expression = ' OR '.join('"' + token + '"' for token in dict.fromkeys(tokens))
    lexical = [] if not expression else db.execute('SELECT rowid FROM lexical WHERE lexical MATCH ? ORDER BY bm25(lexical),path LIMIT ?', (expression, CONFIG['candidate_count'])).fetchall()
    fused = {}
    for ranked in ([records[index][0] for index in dense], [row[0] for row in lexical]):
        for rank, identity in enumerate(ranked): fused[identity] = fused.get(identity, 0) + 1 / (60 + rank + 1)
    lookup = {record[0]: record for record in records}
    candidates = [lookup[identity] for identity in sorted(fused, key=lambda identity: (-fused[identity], lookup[identity][1:4]))[:CONFIG['rerank_count']]]
    scores = rerank(text, candidates)
    best = sorted(zip(candidates, scores), key=lambda pair: (-pair[1], pair[0][1:4]))[:top]
    print(json.dumps({'embedding_model': CONFIG['embedding_model'], 'reranker': CONFIG['reranker_model'], 'device': 'cpu',
        'fusion': 'reciprocal_rank_semantic_and_fts5', 'results': [{'path': record[1], 'start': record[2], 'end': record[3], 'score': score, 'snippet': record[4][:1200]} for record, score in best]}, ensure_ascii=False))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('operation', choices=['build', 'check', 'query', 'status', 'download-reranker'])
    parser.add_argument('text', nargs='?')
    parser.add_argument('--top', type=int, choices=range(1, 11), default=5)
    args = parser.parse_args()
    if args.operation == 'status':
        result = readiness()
        print(json.dumps(result, ensure_ascii=False))
        raise SystemExit(0 if result['status'] == 'PREREQUISITES_READY' else 2)
    if args.operation == 'download-reranker':
        from huggingface_hub import snapshot_download
        snapshot_download(CONFIG['reranker_model'], revision=CONFIG['reranker_revision'],
            local_dir=RUNTIME / 'models' / 'bge-reranker-v2-m3', allow_patterns=['*.json', '*.safetensors', '*.model', 'README.md'])
        print('PASS: pinned local BGE safetensors model available'); return
    with database() as db:
        if args.operation == 'build': build(db)
        elif args.operation == 'check': print(json.dumps({'status': 'PASS', 'files': check(db)}))
        else:
            if not args.text: parser.error('query text required')
            query(db, args.text, args.top)


if __name__ == '__main__': main()
