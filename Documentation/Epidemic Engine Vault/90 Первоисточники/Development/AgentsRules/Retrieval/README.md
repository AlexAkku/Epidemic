# Optional local semantic retrieval

This service is intentionally optional and lazy. Normal work should start with `rg` or clangd.
`Search-Code.ps1` runs one query in a bounded local process and exits. No `-StartDaemon` flag is required; the flag remains accepted for compatibility. `Run-Quiet.ps1` controls timeout, memory and process-tree cleanup. Search never downloads models or creates a missing index.

The indexed corpus excludes `Development/Tasks/` and historical milestone material. It indexes current engine source, tests, Auditor source and CMake code only.

A search compares content hashes with indexed hashes, then refreshes changed/new/deleted files in SQLite and FTS before returning results. Same-size edits with preserved timestamps are detected. Unchanged chunks reuse cached embeddings. Bounded queries refresh an existing index under the same timeout and memory limit; missing models/indexes and corruption fail explicitly. There is no continuous background repository watcher.

## Runtime locations

All heavy/runtime state is disposable under `.ai/retrieval/`:

```text
.ai/retrieval/.venv/
.ai/retrieval/index.sqlite
.ai/retrieval/models/bge-reranker-v2-m3/
```

The embedding model is expected through local Ollama using the model/digest in `config.json`.
The configured model is `qwen3-embedding:4b-q8_0`, pinned by its full digest. An embedding model/digest change requires a fresh index. Initial indexing is an explicit manual operation; setup does not build an index.
The runtime token batch is limited to 64 (`ollama_num_batch`) to reduce Ollama scratch memory. The context remains 4096 tokens and input truncation remains disabled. Ollama exposes these settings through the embedding endpoint's [runtime options](https://docs.ollama.com/api/embed) and [`num_batch` option](https://github.com/ollama/ollama/blob/main/api/types.go).

## Model deployment

Model downloads are deliberately commented out in `Setup-AgentTools.ps1`.
Copy your existing BGE directory to:

```text
.ai/retrieval/models/bge-reranker-v2-m3/
```

Ensure the configured Qwen embedding model already exists in your local Ollama installation. If you copy an existing Ollama model store rather than using the normal global store, configure Ollama itself before starting retrieval.

## Usage

Manual index construction from repository root (does not start Auditor):

```powershell
.ai/retrieval/.venv/Scripts/python.exe -B Development/AgentsRules/Retrieval/retrieve.py build
.ai/retrieval/.venv/Scripts/python.exe -B Development/AgentsRules/Retrieval/retrieve.py check
```

Run `check` only after `build` succeeds. The model may need substantial free virtual memory; use `Run-Quiet.ps1` to enforce the project's memory reserve. `Search-Code.ps1` refuses to start when the index is absent, so a search will not recreate a deleted index automatically.

From repository root:

```powershell
Development/AgentsRules/Scripts/Search-Code.ps1 -Query 'semantic question' -Top 3
```

The response is navigation assistance, never correctness evidence.

Offline focused regression без моделей, сетевых вызовов и production index:

```powershell
.ai/retrieval/.venv/Scripts/python.exe -B Development/AgentsRules/Retrieval/self_test.py
```

Тест выполняет настоящий daemon search и обновление SQLite/FTS/матрицы для изменения, добавления и удаления; подменены только нейронные вызовы. `retrieve.py query` проверяет уже актуальный индекс: сначала выполните `build` либо используйте daemon search для автоматического refresh.
