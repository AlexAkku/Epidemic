# Epidemic agent execution contract

## Core doctrine

Preserve dependency direction:

EngineBase -> EngineRuntime -> EngineFramework

Allowed dependency direction is downward:
- EngineRuntime may depend on EngineBase.
- EngineFramework may depend on EngineRuntime and permitted lower-layer contracts.
- EngineBase must never depend on EngineRuntime or EngineFramework.
- EngineRuntime must never depend on EngineFramework.

Do not hand-edit generated canonical freeze artifacts.
Update their owning source/inventory/handoff and run the owning generator.

Correctness must be demonstrated by code, tests and machine-readable evidence.

## Single-agent rule

Use exactly ONE Codex/LLM reasoning agent.

Never create LLM subagents for:
- analysis
- research
- review
- builds
- tests
- validators
- static analysis
- auditing

LLM concurrency = 1.

Local tools, Auditor collectors/analyzers, compiler processes, test processes and other non-LLM OS processes are not agents.

Parallelism is allowed only between mechanically independent local OS processes.

Never run concurrent processes that write to the same:
- build directory
- generated canonical artifact
- evidence database
- mutable task-state file
- shared output directory

unless that concurrency is explicitly designed and proven safe.

## Context budget

Correctness first. Then minimize model-visible context.

Normal step:
- <= 3 ranges
- <= 200 LOC total
- <= 12 search results

Deep step:
- <= 5 ranges
- <= 400 LOC total
- <= 20 search results

These are TOTALS per reasoning step, not per file/tool call.

Never dump whole JSON/log/source files.
Query fields/ranges first.
If the budget is insufficient, refine the query before reading more.

## Navigation routing

Use one primary navigation mechanism per question.

- architecture / subsystem scope / blast radius -> Graft
- known C++ symbol / references / callers / types -> clangd
- unknown symbol but known behavior -> persistent local retrieval daemon
- exact text / identifier / diagnostic -> ripgrep
- structural invariant -> ast-grep

All exploratory `rg` output must normally be capped to 12 matches. Use 20 only for deep investigation.

Do not cascade all navigation tools for one question.

Do not use Repomix in the normal workflow.

Root compile_commands.json is the authoritative clangd compilation database.

## Persistent semantic retrieval

The retrieval daemon must already be running.

Use:

.ai/retrieval/.venv/Scripts/python.exe .ai/retrieval/client.py search "<query>"

Default result count: 3.
Maximum normal result count: 5.

Never expose internal candidate sets to Codex.

The retrieval daemon owns:
- Qwen3 embedding queries
- FTS5
- reciprocal-rank fusion
- resident CPU BGE reranker
- in-memory vector matrix
- incremental refresh of changed files

A changed source file must not disable semantic retrieval.

Do not repair or redesign retrieval infrastructure during milestone work.

## Mechanical commands

Builds, tests, validators, and static analysis are local processes, never agents.

Use `.ai/run-quiet.ps1` for noisy commands. 

Example: 

.ai/run-quiet.ps1 `
    -Name test `
    -TimeoutSec 600 `
    -Command 'ctest ...'

It owns PowerShell 7, MSBuild node reuse/telemetry, and task process cleanup.

Do not manually:
- poll processes/logs
- use `Get-Process`, `Get-CimInstance`, or `Stop-Process` for cleanup
- set `MSBUILDDISABLENODEREUSE` or `VSCMD_SKIP_SENDTELEMETRY`

On PASS consume only status, duration, and log path. Do not read raw logs unless required to establish missing evidence.
On FAIL consume only exit code, concise diagnostics, and log path.
Read more only if that evidence is insufficient.

Use `.ai/verify-changed.ps1` for changed C++ static checks.
Run `.ai/preflight-generated.ps1` before full Validators.

Do not embed large Python/PowerShell scripts in shell commands.
If temporary logic exceeds ~20 lines, place it under `.ai/` and execute or patch that file.

## Process safety

Normal process cleanup is owned by wrappers. Do not poll or manually kill build/test children.

Exception: after confirmed OOM/C1060 or a proven orphan-process leak, cleanup of repo-owned orphan tool processes is allowed after verifying ownership. Never terminate active MCP servers, retrieval daemons, or unrelated processes.

On OOM/C1060 or <4 GiB free virtual memory:
- stop builds, clangd and static-analysis launches
- perform one memory/process diagnosis
- clean confirmed orphan tool processes
- resume only after memory headroom is restored

Use only one persistent clangd MCP server. Do not leave per-query clangd/bridge processes alive after a request.

## Task state

Maintain .ai/task-state.json.

Keep it small:
- current requirement
- completed requirements
- working files
- confirmed facts
- known blockers
- open questions

Update it after each completed logical subpoint.

After compaction/restart, use `.ai/state-summary.ps1`.
Do not read `.ai/task-state.json` directly unless modifying or investigating its structure.

## Generated/read-only areas

Treat build/, .ci_cache/, .enforcer/, .idea/ and Deltas/ as local artifacts.

Do not manually edit generator-owned canonical freeze JSON when an owning generator exists.

Temporary scripts >20 lines MUST be created/edited as files. 
Never generate them through `$code=@'...'@`, Python `-c`, or equivalent shell embedding.

Do not rewrite an existing temporary script through string replacement if a direct file edit is possible.

## Final verification

Before declaring a milestone complete:

1. Verify every requirement against actual machine-readable evidence.
2. Verify that all final evidence belongs to the exact source/input tree being qualified.
3. Run required builds/tests/validators through quiet wrappers.
4. Enumerate the complete git diff mechanically.
5. Semantically review every changed production, build, validator and test file in bounded slices.
6. Do not dump a large complete diff into model context merely to satisfy "review complete diff".
7. Review git status and classify every remaining file.
8. Verify no weakened validators, deleted negative fixtures, temporary bypasses, fake PASS paths or note-dependent acceptance.
9. Verify generated artifacts were produced by their owning generators and are idempotent where required.
10. Update owning final records and work-plan.
11. State PASS or BLOCKED explicitly.

During a final qualification campaign, freeze all source, CMake, generator, validator and test inputs.

If any such input changes after a qualification receipt was produced,
that receipt is stale for final closure and the affected qualification must be rerun.