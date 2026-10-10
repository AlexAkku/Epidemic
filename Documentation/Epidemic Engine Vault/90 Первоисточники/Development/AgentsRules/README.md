# Agent pipeline files

This directory is the persistent, tracked home of the AI-development pipeline.
`build/` contains only CMake/compiler builds, including native Auditor and `build/clangd/compile_commands.json`. Tools consume that database directly. All other generated/downloaded runtime data belongs under `.ai/`.

## Persistent vs disposable

Persistent here:
- `AGENT_RULES.md` — the single workflow contract.
- `PIPELINE.md` — human/agent reference for the execution flow.
- `Scripts/` — reusable orchestration, process-safety and tooling scripts.
- `Retrieval/` — source/config for the optional local semantic retrieval service.
- `Templates/` — files deployed to root tool configuration.

Disposable in `.ai/`:
- tool clones and downloaded runtimes;
- Python virtual environments;
- semantic index/model copies;
- logs, caches, audit results, runtime bundles, temporary scripts and investigation output.

## First deployment

From repository root:

```powershell
powershell -ExecutionPolicy Bypass -File Development/AgentsRules/Scripts/Setup-AgentTools.ps1
```

Or double-click `Scripts/Setup-AgentTools.cmd`. Visual Studio C++ Build Tools must already be installed; the script installs/checks the lighter command-line dependencies through winget.

The setup script installs/checks normal development dependencies, deploys project tool configuration, installs the pinned clangd MCP server into `.ai/tools`, creates the retrieval Python environment, and prepares the clangd compilation database. When the Auditor component is present, it reuses that component's setup-only entry point to restore its Python environment without starting the application.
The pinned MCP source receives one reproducible Windows compatibility adjustment: it does not pass the unsupported `--malloc-trim` option to clangd on Windows.
Setup also pins the runtime MCP SDK to `1.31.0` and applies compatible runtime dependency security fixes to the disposable tool lock before `npm ci`. Development-only dependencies are used while compiling and pruned from the deployed runtime afterward.

Neural model installation/download commands are intentionally commented out in `Setup-AgentTools.ps1`. Copy the existing model files into the paths documented in `Retrieval/README.md` instead.

## Reset

Reset stops this checkout's retrieval/indexing and MCP worker trees before deleting disposable AI/tool artifacts. CMake/compiler builds in `build/` remain:

```powershell
powershell -ExecutionPolicy Bypass -File Development/AgentsRules/Scripts/Reset-AiWorkspace.ps1 -Force
```

Or double-click `Scripts/Reset-AiWorkspace.cmd`.

Then rerun setup; it restores configuration from permanent templates and recreates tools/venvs. Doctor checks the result. Neural models and the index remain absent until explicit manual deployment/indexing. Clean `build/` separately.

## Repository integration

Root `AGENTS.md` should be the small loader included with this package.
Root `.gitignore` must ignore `/.ai/`. If the previous repository still tracks legacy `.ai` scripts, remove those tracked entries during the migration commit; the new `.ai/` is runtime-only.
The setup script checks/adds that ignore entry and deploys `.codex/config.toml` plus `.clangd` from `Templates/`.
