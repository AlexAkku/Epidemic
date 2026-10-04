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
