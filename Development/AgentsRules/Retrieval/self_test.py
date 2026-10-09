"""Offline regression through the real daemon search, SQLite and FTS index."""
import contextlib
import io
import json
import os
import sqlite3
import tempfile
import threading
import sys
from pathlib import Path
from unittest.mock import patch, Mock

import numpy as np
import daemon
import retrieve


def check_negatives(fixture):
    mutations = [
        'DELETE FROM chunks WHERE id=(SELECT min(id) FROM chunks)',
        'UPDATE chunks SET vector=zeroblob(length(vector)) WHERE id=(SELECT min(id) FROM chunks)',
        'UPDATE chunks SET text=text || "corrupt" WHERE id=(SELECT min(id) FROM chunks)',
        'UPDATE chunks SET sha="stale" WHERE id=(SELECT min(id) FROM chunks)',
        'UPDATE files SET sha="stale" WHERE path=(SELECT min(path) FROM files)',
        'INSERT INTO chunks(path,start,end,sha,text,vector) SELECT path,start,end,sha,text,vector FROM chunks LIMIT 1',
    ]
    for mutation in mutations:
        fixture.execute('SAVEPOINT negative')
        fixture.execute(mutation)
        try:
            retrieve.check(fixture)
        except (RuntimeError, ValueError):
            pass
        else:
            raise AssertionError('Accepted corrupt/incomplete index: ' + mutation)
        finally:
            fixture.execute('ROLLBACK TO negative')
            fixture.execute('RELEASE negative')
    retrieve.check(fixture)


def test_bounded_query_admission():
    ready = {'status': 'PREREQUISITES_READY', 'index': {'status': 'CURRENT'},
             'dependencies': {'status': 'AVAILABLE'}, 'embedding': {'status': 'AVAILABLE'},
             'reranker': {'status': 'LOCAL_FILES_VALIDATED'}}
    stale = {**ready, 'status': 'NOT_READY', 'index': {'status': 'STALE'}}
    with patch.object(sys, 'argv', ['daemon.py', '--query', 'question', '--top', '4']), \
            patch.object(daemon, 'ENGINE', None), patch.dict(daemon.CONFIG), \
            contextlib.redirect_stdout(io.StringIO()) as output:
        fake = Mock()
        fake.search.return_value = {'status': 'PASS', 'results': [], 'refresh': {'changed': False}}
        with patch.object(retrieve, 'readiness', side_effect=[stale, ready]), \
                patch.object(retrieve, 'database') as database, \
                patch.object(daemon, 'quiet_build', return_value={'status': 'PASS', 'updated_files': 1}) as build, \
                patch.object(daemon, 'Engine', return_value=fake) as engine:
            daemon.main()
            build.assert_called_once_with(database.return_value)
            database.return_value.close.assert_called_once()
            engine.assert_called_once_with(refresh=False)
            fake.search.assert_called_once_with('question', 4)
            fake.stop_event.set.assert_called_once()
            fake.db.close.assert_called_once()
            assert json.loads(output.getvalue())['refresh']['changed']
        unavailable_cases = [
            {**ready, 'status': 'NOT_READY', 'index': {'status': state}}
            for state in ('MISSING', 'INVALID_OR_STALE')]
        unavailable_cases.extend(
            {**stale, key: {'status': 'MISSING'}} for key in ('embedding', 'reranker', 'dependencies'))
        for unavailable in unavailable_cases:
            with patch.object(retrieve, 'readiness', return_value=unavailable), \
                    patch.object(retrieve, 'database') as database, patch.object(daemon, 'Engine') as engine:
                try:
                    daemon.main()
                except SystemExit as error:
                    assert error.code == 2
                else:
                    raise AssertionError('Unavailable index admitted')
                database.assert_not_called()
                engine.assert_not_called()


def test_incremental_search():
    vector = np.zeros(retrieve.EMBEDDING_DIMENSIONS, dtype=np.float32)
    vector[0] = 1
    embedded = []

    def embed(texts, query=False):
        if not query:
            embedded.extend(texts)
        return np.tile(vector, (len(texts), 1))

    temporary = retrieve.ROOT / '.ai' / 'tmp'
    temporary.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='retrieval-test-', dir=temporary) as directory:
        root = Path(directory)
        sources = root / 'src'
        sources.mkdir()
        changed = sources / 'changed.cpp'
        changed.write_text('BeforeToken\n', encoding='utf-8', newline='')
        runtime = root / '.ai' / 'retrieval'
        runtime.mkdir(parents=True)
        config = dict(retrieve.CONFIG, roots=['src'])
        with contextlib.ExitStack() as mocks:
            for module in (retrieve, daemon):
                mocks.enter_context(patch.object(module, 'ROOT', root))
                mocks.enter_context(patch.object(module, 'CONFIG', config))
            mocks.enter_context(patch.object(retrieve, 'RUNTIME', runtime))
            mocks.enter_context(patch.object(retrieve, 'verify_model'))
            mocks.enter_context(patch.object(retrieve, 'embed', embed))
            mocks.enter_context(patch.object(retrieve, 'api', return_value={}))
            mocks.enter_context(patch.object(daemon.Engine, 'keep_embedding_model_alive'))
            mocks.enter_context(patch.object(daemon.Engine, 'embed_query', return_value=vector))
            mocks.enter_context(patch.object(daemon.Engine, 'rerank',
                lambda self, text, records: [float(text in record[4]) for record in records]))
            with contextlib.closing(retrieve.database()) as db:
                engine = daemon.Engine.__new__(daemon.Engine)
                engine.lock = threading.RLock()
                engine.db = db
                daemon.quiet_build(db)
                engine.reload_memory_index()
                engine.signatures = engine.capture_signatures()
                initial = engine.search('BeforeToken')
                assert initial['results'][0]['snippet'] == 'BeforeToken\n'
                assert not initial['refresh']['changed']
                check_negatives(db)

                # A same-size edit with restored timestamps must still refresh.
                stat = changed.stat()
                changed.write_text('After_Token\n', encoding='utf-8', newline='')
                os.utime(changed, ns=(stat.st_atime_ns, stat.st_mtime_ns))
                result = engine.search('After_Token')
                assert result['refresh']['changed'], 'Same-size/timestamp edit was missed'
                assert result['results'][0]['snippet'] == 'After_Token\n'
                assert not db.execute("SELECT rowid FROM lexical WHERE lexical MATCH 'BeforeToken'").fetchall()
                assert len(embedded) == 2

                added = sources / 'added.cpp'
                added.write_text('AddedToken\n', encoding='utf-8', newline='')
                result = engine.search('AddedToken')
                assert result['refresh']['changed']
                assert result['results'][0]['path'] == 'src/added.cpp'
                assert db.execute("SELECT rowid FROM lexical WHERE lexical MATCH 'AddedToken'").fetchall()
                assert len(embedded) == 3, 'Unchanged files were re-embedded'

                added.unlink()
                result = engine.search('AddedToken')
                assert result['refresh']['changed']
                assert all(item['path'] != 'src/added.cpp' for item in result['results'])
                assert not db.execute("SELECT rowid FROM lexical WHERE lexical MATCH 'AddedToken'").fetchall()
                assert len(embedded) == 3, 'Deletion re-embedded unchanged files'
                retrieve.check(db)
                assert not engine.search('After_Token')['refresh']['changed']


if __name__ == '__main__':
    test_bounded_query_admission()
    test_incremental_search()
    # Preserve admission of the real corpus when a manually built index exists.
    # The offline regression remains runnable after a clean bootstrap.
    if (retrieve.RUNTIME / 'index.sqlite').is_file():
        with contextlib.closing(retrieve.database()) as source:
            retrieve.check(source)
            with sqlite3.connect(':memory:') as fixture:
                source.backup(fixture)
                check_negatives(fixture)
    print('PASS: incremental search modified/added/deleted, FTS refresh, embedding reuse; 6 admission negatives')

