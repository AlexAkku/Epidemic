# Epidemic agent execution contract

## Doctrine

- Preserve the dependency direction `EngineBase -> EngineRuntime -> EngineFramework`; lower layers never depend on higher layers.
- Do not edit generated canonical freeze JSON manually. Update its source inventory/handoff and run the owning generator.
- A change is not complete until the relevant local CI entry point passes.

## Project map

- `EngineBase/`: platform, diagnostics, input, memory, RHI and application lifecycle.
- `EngineRuntime/`: independent runtime majors; cross-major technical composition belongs in Runtime Support.
- `EngineFramework/`: reusable gameplay state owners, integration adapters and Framework tests.
- `docs/freeze/`: architecture, public-surface, coverage and LOCAL_READY validators.
- `_goal4_handoff/`: raw Goal 4 evidence; canonical outputs are produced by its merge script.
- `tools/ci/`: language/toolchain layer for the local CI runner.

## Read-only/generated zones

- Treat `build/`, `.ci_cache/`, `.enforcer/`, `.idea/` and `Deltas/` as local artifacts.
- Do not hand-edit `docs/freeze/coverage_manifests.json`, `public_api_anchors.json`,
  `module_dossier_reviews.json` or `local_ready_ledger.json`.

## Quality control

- Default final verification: `./run.ps1 -Mode Full -NoCache`.
- For dependency/profile changes: `./run.ps1 -Mode Matrix -NoCache`.
- For a focused iteration: `./run.ps1 -Mode Module -Module <name>`.
- Give full local CI at least a 25-minute terminal timeout; command-level heartbeats and logs are built in.

The runner is adapted from the AgentEnforcer2 blueprint:
https://github.com/Artemonim/AgentEnforcer2

## Large codebase navigation

This is a large C++ codebase. Minimize context consumption.

Before reading source files:

1. Use clangd symbol navigation when a symbol is known.
2. Use definitions, references, implementations and call hierarchy before broad source exploration.
3. Use ripgrep for exact textual searches.
4. Use ast-grep for structural C++ searches.
5. Use Repomix compressed output for architectural context; narrow it to the subsystem.
6. Read narrow source ranges instead of complete files whenever possible.
7. Do not recursively open large numbers of files.
8. Do not read complete large files unless full context is required.
9. Prefer symbol-level navigation over directory exploration.
10. Treat root `compile_commands.json` as the compilation database.

Preferred tool selection:

- known symbol -> clangd
- references/callers/types -> clangd
- exact text -> ripgrep
- AST/code pattern -> ast-grep
- subsystem architecture -> Repomix

Run `powershell -ExecutionPolicy Bypass -File .ai/check-ai-tools.ps1` to check
local navigation prerequisites. Run `node .ai/smoke-ai-tools.mjs` to verify LSP
background indexing, MCP queries and Repomix sandbox confinement. Use
`npm run ai:repomix-mcp` for Repomix MCP: always keep `--sandbox .`; never expose
an unrestricted MCP filesystem server. Use `.ai/run-clangd-mcp.ps1` for clangd MCP.

For navigation-infrastructure-only changes, these checks plus `npm run ai:tokens`
are the relevant verification entry points; do not build/test the C++ project
unless the task requires it. Refresh the generated compilation database after
CMake source/target changes; do not hand-edit its compiler commands.

<!-- graft:start -->
## Graft — repo context graph

This repo is indexed in `graft/`: small linked markdown nodes that explain each
system and carry exact file:line spans, kept in sync with the code through git.

For ANY task here — understanding how something works, finding where code lives,
or scoping a change — get context from the graph before grepping or opening
source files. Re-ask freely (it's cheap) and reuse literal identifiers you
already have (symbol, error string, file name) as the query. New to this repo?
Run `graft map` first — a token-budgeted orientation (dir clusters, hubs,
hotspots), no LLM, no key.

- Run `graft ask "<your question>" --source` → ranked nodes with the relevant
  code spans inlined (each hit's ≤8-line crux by default; `--full` for whole
  definitions when the crux isn't enough). Match the tool to the task shape:
  for understanding or editing, the top node IS the answer — cite its
  `covers:` file:line spans and edit straight from `--source`. For
  exhaustive tasks ("every occurrence / every caller of this pattern"), ranked
  results are top-N, not complete — run `graft grep "<literal>"` instead
  (exhaustive over indexed files, grouped by enclosing symbol), falling back
  to raw `grep -rn` only for unindexed files.
- `graft skeleton <file>` → every definition's signature + span, ~10× cheaper
  than reading the file; use it to skim an API surface.
- `graft callers <symbol>` gives precomputed, exact edges — who calls this.
  Add `--direction out` for what it calls, or `--depth N` to walk
  transitively for the full blast radius. For structural questions, skip
  ranking and use this directly.
- Or browse: `graft/INDEX.md` lists every node; follow the links.
- Monorepos and folders of multiple repos rank fairly across sub-projects —
  hits carry `[scope/]` labels naming which one they're from. Narrow with
  `graft ask "<task>" --in <scope>/` once you know where you're working.

If a returned span is truncated ("+N more lines"), open the file at that exact
range before finalizing. Only open source files when a node genuinely lacks a
needed detail, and then at the exact file:line the node points to — never
re-read whole files.

After big code changes, refresh the graph with `graft build` (deterministic,
no API key, $0).
<!-- graft:end -->
