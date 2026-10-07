# Epidemic persistent local retrieval

The normal Codex workflow uses one persistent retrieval daemon.

## Architecture

GPU / VRAM:
- qwen3-embedding:4b-q8_0
- kept resident through Ollama keep_alive

CPU / RAM:
- BAAI/bge-reranker-v2-m3
- loaded once by daemon.py
- SQLite FTS5
- dense vector matrix
- metadata/file signatures

SSD:
- index.sqlite
- model files
- repository

## Start

From repository root:

`.ai/retrieval/.venv/Scripts/python.exe .ai/retrieval/daemon.py`

The process stays alive for the whole Codex session.

## Health

`.ai/retrieval/.venv/Scripts/python.exe .ai/retrieval/client.py health`

## Search

Default:

`.ai/retrieval/.venv/Scripts/python.exe .ai/retrieval/client.py search "query"`

Explicit maximum normal result count:

`.ai/retrieval/.venv/Scripts/python.exe .ai/retrieval/client.py search "query" --top 5`

Default is top 3.

## Incremental refresh

The daemon polls file metadata.

When indexed source changes:
1. only changed/new/deleted files are reconciled by the existing incremental builder;
2. unchanged embeddings are reused;
3. FTS5 is updated;
4. the resident vector matrix is refreshed;
5. Qwen is kept resident again.

A dirty file does not permanently disable semantic retrieval.

A foreground search synchronizes pending changes before returning results.

## Important

Retrieval is navigation evidence, not a correctness oracle.

Use:
- Graft for architecture/scope/blast radius;
- clangd for known C++ symbols;
- rg for exact textual evidence;
- ast-grep for structural evidence.

Never expose internal candidate sets to Codex.