# Milestone 6 Auditor plan — quality audit

Revision: 2026-10-06

This document records the review applied to the MS6 plan before execution.

## 1. Fidelity checks

PASS — canonical repository copy parses as exactly460 sequential IDs1..460.
Clarification2026-10-06: origin conversation SHA and actual canonical file SHA
are recorded separately; future canonical drift must fail admission.

PASS — every source ID has exactly one primary execution stage.

PASS — the traceability matrix preserves the original wording verbatim.

PASS — `DEFERRED` is not an allowed final status.

PASS — requirements that were explicitly written as conditional remain conditional rather than being silently deleted.

PASS — source requirement 281 ("future Action state changes") is not dropped; the plan promotes it into the shadow state-change probe capability so the full Auditor contains the function.

PASS — original future milestone labels in 437–442 are not deleted; they are explicitly remapped to the new MS6→MS7→MS8→MS9 order.

## 2. Removal of abstract acceptance

The plan was rejected/reworked wherever a stage could otherwise close on phrases such as:

- "implement profiler";
- "support memory";
- "add causal tracing";
- "show a dashboard";
- "support fault injection".

Each execution slice now specifies:

- required input/data contract;
- canonical structured output;
- positive fixture;
- negative/control fixture;
- machine-readable closure;
- source requirement checklist.

## 3. Evidence quality checks

PASS — raw evidence is immutable and hash-manifested.

PASS — lossy collectors require dropped-event/completeness metadata.

PASS — derived findings must retain input evidence references.

PASS — old raw capture reanalysis is explicitly required.

PASS — internal Epidemic PASS/log claims are not accepted as independent proof.

PASS — engine probes have distinct provenance and cannot masquerade as external observation.

PASS — call tree and causal DAG are separate models.

PASS — import-table presence is explicitly separated from runtime API execution.

PASS — coverage is explicitly separated from correctness.

## 4. Actual workspace Auditor baseline handling

PASS — the plan does not require a rewrite merely because the milestone is new.

PASS — actual workspace claims start unverified; exit classifications are
VERIFIED_REUSE / REPAIR / REPLACE / MISSING. Currently0.1.0 is authoritative;
historical0.2 source is absent and must not be fabricated or rebuilt for admission.

PASS — reuse/repair/replace decisions are driven by acceptance fixtures.

This prevents both needless rewrite and unverified inheritance of old behavior.

## 5. Build safety

PASS — normal engine builds must remain free of Auditor exact-trace/probe dependencies.

PASS — Deep instrumentation is isolated.

PASS — Targeted Exact exists to avoid using full exact tracing as an ordinary performance mode.

PASS — comparison between incompatible instrumentation profiles is rejected or marked partial.

## 6. Detector truthfulness

PASS — each critical detector family requires known-bad and known-good fixtures.

PASS — missing telemetry is represented as missing/incomplete, not zero.

PASS — trace overflow cannot report a complete trace.

PASS — missing cross-thread correlation cannot create a guessed causal edge.

PASS — fault injectors require proof-of-effect, scope and cleanup; an injector API returning success is insufficient.

## 7. Agent-context discipline

PASS — agent investigation uses stable IDs.

PASS — packages are bounded.

PASS — truncation requires explicit `truncated=true` and continuation.

PASS — raw gigabyte-scale captures are referenced, not injected into model context.

## 8. Final closure

The final closure must fail if any source obligation is:

- unmapped;
- NOT_STARTED;
- IMPLEMENTED_UNQUALIFIED;
- mandatory BLOCKED;
- silently deferred.

Only the final machine gate may emit:

`GOAL6_AUDITOR_COMPLETE`

## 9. Known deliberate non-decisions

The plan does not invent arbitrary numeric performance-overhead thresholds for Fast/Full/Deep because the supplied source requirements do not define them.

Instead it requires:

- exact instrumentation profile recording;
- measured observer overhead;
- compatible-profile comparison rules;
- explicit completeness and loss reporting.

A numeric overhead budget may be frozen later from measured controlled fixtures without weakening any source obligation.

## 10. Review verdict

The original specification remains functionally complete, but its old per-packet generated-receipt **execution process has been superseded** by Layer 3 P.2–P.5. This audit is a historical plan review, not fresh code or stage evidence. After cleanup, no legacy `_goal6_handoff` file is an obligatory stage gate.

Current review verdict: execute in broad cohesive implementation blocks, perform grouped tests after the block, and require one independent, adversarial, current-source final stage qualification after a real freeze. Preserve the original 460 product obligations, immutable raw and valid sensor completeness semantics.

The document set is execution-ready as a specification.

It is deliberately strict about proof and mapping, while leaving implementation choices open only where the source requirements do not constrain them.
