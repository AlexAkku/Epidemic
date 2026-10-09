# Epidemic agent execution rules

## 1. Priorities
Apply these priorities in order:
1. Minimize model-visible context and repeated reasoning.
2. Preserve code correctness and required behavior.
3. Minimize wall-clock work and repeated mechanical runs.
4. Limit local CPU, RAM and disk pressure.

## 2. Scope and sources of truth
- Use exactly one LLM/reasoning agent. Local compiler/test/tool processes are not agents.
- Current user request is authoritative. Read a task file only when the user points to that task.
- Do not proactively scan `Development/Tasks/`, historical milestones, old handoffs, old evidence, or deleted/quarantined material.
- For implementation truth prefer, in order: current task -> current production code/tests -> `Documentation/architecture.md` -> other current documentation.
- `.ai/` is disposable runtime state. Never treat files there as requirements, historical proof, or canonical design.
- `Development/AgentsRules/` is stable pipeline code. Do not modify it during product work unless the user explicitly asks to change tooling/pipeline behavior.
- Layer order, from lowest to highest: `EngineBase -> EngineRuntime -> EngineFramework -> Game`. Allowed dependencies go from higher to lower layers; lower layers never depend on upper layers.
- Do not broaden architecture allowlists or public surfaces as a shortcut.

## 3. Context budget
- Start from the smallest relevant scope; do not inventory the whole repository unless the task requires it.
- Normal reasoning step: <= 3 code ranges, <= 200 LOC total, <= 12 search hits.
- Deep investigation: <= 5 ranges, <= 400 LOC total, <= 20 search hits.
- Never dump whole large logs, JSON files, generated tables, traces, or diffs into model context.
- On PASS consume only status, duration and paths. On FAIL read the concise diagnostics first, then only the relevant log range.
- Do not reread unchanged files in the same work phase unless new evidence requires it.

## 4. Navigation routing
Use one primary mechanism per question; stop when it answers the question.
- Exact identifier/text/diagnostic -> `rg`.
- Known C++ symbol, definition, references, callers, type -> clangd MCP.
- After CMake/source-list changes, refresh the clangd compilation database before relying on clangd/static analysis.
- Unknown behavior/location after exact/symbol search is insufficient -> local retrieval, top 3 results by default.
- Build/CMake structure -> direct CMake/file inspection plus `rg`; do not start semantic retrieval for obvious paths.
- Do not cascade multiple navigation tools merely for confidence.
- Local retrieval is optional. A search refreshes changed indexed files before answering. If unavailable, continue with `rg`/clangd; do not repair tooling during product work.

## 5. Implementation workflow
Work in coherent batches, normally 2-5 closely related changes sharing one subsystem.
For each batch:
1. Inspect the minimum relevant code and tests.
2. Implement the complete batch before broad verification.
3. Run the smallest focused build/test that can falsify the change.
4. If a real defect is found, add/keep one regression that fails before the fix and passes after it.
5. Continue to the next batch; do not fully qualify the stage after every edit.

Do not create permanent helper frameworks for one-off problems. Temporary investigation scripts belong under `.ai/` and may be deleted without migration.
Prefer extending existing production tests/helpers over adding parallel validators or evidence systems.

## 6. Verification levels
There are only two verification levels.

### Development verification
- Focused compile/build/test for changed behavior.
- Changed-file static analysis only when useful at a batch boundary.
- One strong regression is normally sufficient for a defect during development.
- No receipts, proof-of-proof, qualification packets, closure JSON, reviewed inventories, or generated evidence ledgers.

### Final/stage verification
Order: implementation batches -> focused tests -> semantic review of the completed diff -> corrections -> freeze -> one final qualification.
1. Review every changed production/build/test/tool file in bounded slices; fix findings and run affected focused tests.
2. Freeze production/build/test/tool inputs once review findings are resolved.
3. Clean configure/build as required by the task.
4. Run required tests and permanent architecture/contract checks.
5. Run changed-file/deep static analysis where applicable.
6. Check git diff/status mechanically and report actual results and concrete failures.

If final verification finds a defect, fix it and rerun only affected checks plus the final gate. Do not rebuild an evidence bureaucracy.

## 7. Mechanical commands
- Use `Development/AgentsRules/Scripts/Run-Quiet.ps1` for noisy/long commands.
- Use `Development/AgentsRules/Scripts/Invoke-Dev.ps1` for normal configure/build/test flows.
- Use `Development/AgentsRules/Scripts/Prepare-Clangd.ps1` only when the clangd compilation database is absent/stale.
- Use `Development/AgentsRules/Scripts/Verify-Changed.ps1` at batch/final boundaries, not after every edit.
- `build/` contains only CMake/compiler builds, including native Auditor and `build/clangd/compile_commands.json`. Tools consume that database directly.
- `.ai/` contains temporary scripts, logs, indexes, models, venvs, downloaded tools, caches, audit results and runtime bundles. Deleting it loses no instructions/configuration/source; setup restores tools, while model deployment/indexing remain manual. Clean `build/` separately.
- `Development/AgentsRules/` contains permanent instructions, tool sources, setup and configuration templates. Use stable paths instead of UUID build trees.
- Parallel compilation is allowed. Do not run independent heavy jobs concurrently when they compete for the same RAM/disk or write the same outputs.

## 8. Resource policy
- Keep at least 4 GiB free physical and virtual memory during heavy local commands.
- Default owned job memory cap is 8 GiB; raise it only for a known task that genuinely needs more.
- Large inputs must be streamed/chunked; do not hold multiple complete copies or serialize giant datasets through redundant in-memory forms.
- Do not preserve historical builds, decoded tables, traces, logs, caches or analysis outputs merely for possible future use.
- Keep only current runtime artifacts needed for the active work. Everything under `.ai/` may be deleted after the work session.

## 9. Process and repository safety
- Let wrappers own their child process trees and timeouts. Do not poll/kill arbitrary system processes.
- Never terminate unrelated processes or persistent services without verified ownership.
- Do not use destructive git operations (`reset --hard`, mass checkout/clean, history rewrite) unless the user explicitly asks.
- Preserve unrelated user changes.
- Privileged operations use the minimum required rights and narrow authenticated interfaces. Do not elevate a whole application or weaken ACLs to bypass a failure. Component deployment/protocol/path specifics belong alongside their sources, not in this contract.
- Do not weaken tests, validators, architecture checks, warnings, or failure paths to obtain PASS.
- Do not silently substitute notes/comments for executable behavior.
- If elevation/user action is required, report it once and continue independent work; do not build a new subsystem around the blocker unless the product itself requires one.

## 10. Output discipline
- Keep agent-visible command output concise.
- Store full transient logs only under `.ai/logs/`; do not copy them into tracked documentation.
- Keep task progress concise and current; do not maintain cumulative histories of already closed work.
- If context compaction needs a resume note, overwrite `.ai/state/current.md`; keep it <=100 lines and only record current scope, completed items, blockers and next action.
- At completion report: implemented changes, checks actually run, concrete remaining blockers, and no speculative proof claims.
