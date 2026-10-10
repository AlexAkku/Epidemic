# Efficient development pipeline

The pipeline is deliberately small. It separates cheap development feedback from one strict final verification pass.

## Runtime layout

```text
.ai/
  logs/             latest command logs
  tmp/              command/test temporary files
  tools/            downloaded tools and runtime bundles
  retrieval/        local retrieval index/model runtime
  venv/             Python environments
  cache/            tool/package caches
  auditor/          audit results
  state/            optional current-session scratch state
```

Nothing in `.ai/` is authoritative or required to survive cleanup.
CMake/compiler builds live separately under `build/`, including native Auditor. clangd reads `build/clangd/compile_commands.json` directly. Resetting `.ai/` preserves builds; setup restores configuration, tools and venvs from `Development/AgentsRules/`. Models and initial indexing remain manual.

## Normal work cycle

1. Read only the current request/task and the smallest relevant code surface. Reuse knowledge of studied subsystems; reread only changed ranges or when new evidence requires it.
2. Navigate with `rg` or clangd. Use retrieval only when the location/behavior is genuinely unknown.
3. Implement 2-5 tightly related changes.
4. Run one focused build/test batch.
5. Keep a regression test when a real defect was reproduced.
6. Repeat until the implementation scope is complete.
7. Perform one semantic review of the completed stage/change set.
8. Fix review findings.
9. Freeze inputs and run one final verification campaign.

## Commands

Configure/build:

```powershell
Development/AgentsRules/Scripts/Invoke-Dev.ps1 -Action Build -Profile full -Configuration Debug
```

Build + focused tests:

```powershell
Development/AgentsRules/Scripts/Invoke-Dev.ps1 -Action Test -Profile full -Configuration Debug -TestRegex 'RelevantTest'
```

Final local pass:

```powershell
Development/AgentsRules/Scripts/Invoke-Dev.ps1 -Action Full -Profile full -Configuration Debug -Clean
```

Changed-source static checks:

```powershell
Development/AgentsRules/Scripts/Verify-Changed.ps1 -Mode Fast
Development/AgentsRules/Scripts/Verify-Changed.ps1 -Mode Deep
```

Prepare clangd database only when needed:

```powershell
Development/AgentsRules/Scripts/Prepare-Clangd.ps1
```

Optional semantic search (starts retrieval lazily and refreshes changed files on the query):

```powershell
Development/AgentsRules/Scripts/Search-Code.ps1 -Query 'where is runtime save reconciliation implemented'
```

## Tool routing

- `rg`: default exact search; fastest and cheapest.
- clangd MCP: known C++ symbol relations and types.
- retrieval: unknown behavior/location only; top 3 snippets.
- compiler/CTest: correctness authority for executable behavior.
- permanent CMake architecture checks: architecture authority.
- static analysis: batch/final boundary, not per edit.

Graft, Repomix and a separate Enforcer are intentionally absent from the default pipeline. Their overlap did not justify their orchestration/context cost.

## Final verification

Final verification starts only after semantic review and code freeze. A typical final pass is:
- clean configure/build;
- architecture adversarial self-test;
- public-header self-containment target;
- required CTest suite;
- deep changed-file static analysis;
- mechanical git diff/status check.

Do not produce a new receipt/evidence hierarchy for the pass. The current code, permanent tests, command exit status and CI are the proof surface.
