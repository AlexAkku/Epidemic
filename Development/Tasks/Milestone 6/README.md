# Milestone 6 Auditor — document set

## Minimal reading path for an implementation block

1. Read **P.2–P.5** in `Milestone_6_Auditor_Layer3_Execution_Plan.md` (workflow/qualification contract).
2. Read the **one active** `MS6_0N_...md` stage specification and only its mapped `SRC-...` range in `Milestone_6_Auditor_Requirement_Traceability.md`.
3. Consult `Milestone_6_Auditor_Layer2_Technical_Specification.md`, Layer 1 roadmap or subsystem notes **only when the active capability needs them**; do not dump all milestone documents into the agent context by default.
4. Historical MS6.0/MS6.1 execution records describe past observations; they **do not** requalify a new working tree. The current source and tests are the authoritative implementation.

The 460 original source obligations and wording are preserved verbatim by `Milestone_6_Auditor_Requirement_Traceability.md`. Stage checklists group requirements but cannot weaken them. Run tests after **large cohesive implementation blocks**, then run the **strict final stage gate once after freeze**. No per-fix evidence-packet bureaucracy.

`build/` holds compilations; `.ai/` holds disposable agent tooling/intermediates. Actual Auditor product evidence (raw captures/manifests/history and secured service storage) follows its product contracts and cannot be discarded merely because the agent workspace is disposable.
