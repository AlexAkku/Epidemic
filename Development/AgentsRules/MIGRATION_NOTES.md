# Migration notes from the previous pipeline

Reused and simplified:
- `Scripts/Run-Quiet.ps1` (Job Object, timeout, memory reserve, concise diagnostics retained; outputs moved to disposable `.ai/`; logs are latest-per-name).
- `Scripts/Verify-Changed.ps1` (dynamic tool discovery, compilation database in `build/clangd/`, Fast/Deep modes, no parallel static-analysis storm).
- `Scripts/Run-ClangdMcp.ps1` (pinned downloaded server under `.ai/tools`, background indexing disabled).
- `Retrieval/` (source is tracked here; venv/index/models stay in `.ai`; no continuous watcher; refresh happens on search).

Removed from the default workflow:
- Graft: overlaps exact/clangd/retrieval navigation and adds another MCP lifecycle.
- Repomix: large-context packing conflicts with the context-budget goal.
- Enforcer/proof/receipt/packet layers: permanent tests and one final verification pass replace them.
- per-edit full qualification and historical build/evidence retention.

The setup script does not install/download neural model weights. Those lines are deliberately commented for manual reuse of the existing local model data.
