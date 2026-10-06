# Part 5/7 — 5.5 Deterministic runner

Дата pipeline-ревизии: 2026-10-05

Этот файл является самодостаточным execution-slice для текущего этапа. Он сохраняет полные numbered requirements данного этапа и повторяет только те общие ограничения, которые нужны для безопасного выполнения без чтения остальных шести частей. Предыдущие этапы считаются входным contract только если их PASS зафиксирован в machine-readable/task state; при красном предыдущем gate этот этап не начинается.

---

# P. Pipeline execution contract

## P.1. Requirement fidelity

Authoritative semantic detail находится в numbered requirements ниже. Packet map не заменяет их и не имеет права объявлять PASS, если хотя бы один входящий `M5.x.y` не выполнен.

Legacy command blocks внутри сохранённого плана трактуются как **proof endpoints / expected gates**, а не как требование модели вручную оркестрировать конкретные инструменты или читать большие логи. Механический запуск проходит через project execution contract и quiet local wrappers; semantic meaning команды/validator/test при этом не меняется.

## P.2. Packet completion schema

Каждый operational packet закрывается только при наличии:

- `objective_status`: PASS/BLOCKED;
- `requirement_ids`: полный диапазон входящих `M5.x.y`;
- `finite_surface`: IDs записей/сценариев/дефектов, если применимо;
- `changed_scope`: ownership-local files/contracts;
- `proof`: exact validator/test/evidence identities;
- `remaining_gaps`: пусто либо explicit blockers;
- `handoff`: краткие stable facts для следующего packet.

Нельзя закрывать packet narrative summary без machine proof.

## P.3. Operational packets for this part

### P5-A — Deterministic semantic domain/digest
Requirements: `M5.5.1–M5.5.5`.

Цель: reviewed deterministic owner set + canonical digest encoding/coverage.

### P5-B — Scenario runner model
Requirements: `M5.5.6–M5.5.10`.

Цель: scenario steps/descriptors/event trace/result model и adversarial runner self-tests.

### P5-C — Runner system qualification
Requirements: `M5.5.11–M5.5.15`.

Цель: Truth target + control/fresh-composition/fresh-process mini-scenarios и закрытый 5.5 gate.

## P.4. Work sizing inside a packet

Если packet surface всё ещё велик (например сотни candidates), он делится только по stable finite IDs или ownership boundaries. Sub-slice не получает отдельный semantic definition; он наследует requirement IDs packet и возвращает reviewed records в один canonical contract.

Разрешённые slice boundaries:
- module/owner;
- candidate kind;
- participant set;
- scenario ID;
- defect ID;
- deterministic risk category.

Запрещённые boundaries:
- случайные file batches;
- «первые N найденных» как acceptance;
- tool-specific batches;
- разные LLM workers с независимыми semantic decisions.

## P.5. Proof discipline

Mechanical evidence подтверждает requirement, но не определяет его semantics. PASS output хранится вне reasoning context; FAIL должен сводиться к первой значимой причине и exact evidence location. Полные логи нужны только для расследования конкретного failure.

## P.6. Resume discipline

После завершения packet/sub-slice structured task state хранит requirement IDs, reviewed finite IDs, change-set, proof identities и blockers. При неизменных inputs закрытый packet не пересматривается после compaction.

---

# 1. Общие правила выполнения

## 1.1. Работа строго последовательная

Milestone 5 выполняется только в порядке:

1. Admission.
2. 5.1 Persistence ownership map.
3. 5.2 Durable representation.
4. 5.3 Unified save point.
5. 5.4 Whole-engine restore.
6. 5.5 Deterministic runner.
7. 5.6 Determinism qualification.
8. 5.7 Final qualification.

Нельзя начинать следующий этап, если предыдущий имеет `BLOCKED`, `UNCLASSIFIED`, stale generated artifacts или validator failures.

## 1.2. Один рабочий baseline

После каждого этапа:

1. Все generated canonical artifacts перегенерируются штатным generator.
2. Повторный generator run должен дать zero diff.
3. Validators должны быть green.
4. Relevant tests должны быть green.
5. Изменения фиксируются одним serial checkpoint commit или delta.
6. Следующий этап начинается только от этого состояния.

## 1.3. Defect workflow

Любая найденная production problem проходит один порядок:

1. Создать stable defect ID.
2. Зафиксировать reproducer.
3. Добавить failing regression.
4. Подтвердить failure на текущем code.
5. Выполнить минимальный production fix.
6. Получить regression PASS.
7. Прогнать affected module/integration tests.
8. Прогнать affected Goal 5 validator.
9. Обновить machine-readable contract.
10. Не менять unrelated code.

Рекомендуемые ID prefixes:

`G5-MAP-*`
`G5-SAVE-*`
`G5-SCHEMA-*`
`G5-STORAGE-*`
`G5-BARRIER-*`
`G5-RESTORE-*`
`G5-RECON-*`
`G5-DET-*`

## 1.4. Новые machine-readable artifacts

Все Goal 5 canonical files размещаются в `docs/freeze`.

Планируемый набор:

```text
docs/freeze/goal5_persistence_candidates.json
docs/freeze/goal5_persistence_inventory.py
docs/freeze/goal5_persistence_contract.json
docs/freeze/goal5_persistence_contract.py
docs/freeze/goal5_persistence_gaps.json

docs/freeze/goal5_participant_contract.json
docs/freeze/goal5_participant_contract.py
docs/freeze/goal5_schema_contract.json
docs/freeze/goal5_schema_contract.py

docs/freeze/goal5_save_generation_contract.json
docs/freeze/goal5_save_generation_contract.py

docs/freeze/goal5_restore_contract.json
docs/freeze/goal5_restore_contract.py

docs/freeze/goal5_determinism_candidates.json
docs/freeze/goal5_determinism_inventory.py
docs/freeze/goal5_determinism_contract.json
docs/freeze/goal5_determinism_contract.py

docs/freeze/goal5_evidence_quality.py
docs/freeze/goal5_ledger.json
docs/freeze/goal5_contract.py
```

Необязательно создавать каждый файл заранее.

Файл появляется только на своём этапе.

Каждый Python validator должен поддерживать минимум:

```text
--check
--self-test
```

Generator, если нужен, должен отдельно поддерживать deterministic regeneration.

## 1.5. Stable evidence identity

Goal 5 evidence не должен опираться только на line number.

Для executable evidence хранятся минимум:

```text
test_target
test_symbol или test_case
contract_assertion
source_path
optional_line
```

Line используется как navigation hint.

Если line изменился, evidence остаётся валидным по stable test identity.

## 1.6. CI policy

Новый validator включается в CI только после того, как:

1. Его own positive fixture PASS.
2. Все negative fixtures действительно FAIL.
3. Current production tree PASS.
4. Generator idempotent.
5. Validator не читает roadmap/Milestone Markdown.
6. Validator не auto-repairs canonical state в `--check`.

До этого validator запускается локально как development gate.

---

---

# 7. Stage 5.5: Deterministic semantic scenario runner

# 5.5.A. Define deterministic domain

## M5.5.1. Сформировать deterministic owner set

Input:

5.1 persistence contract.
Existing module dossiers.
Gameplay authoritative owners.

Каждый state owner получает:

```text
INCLUDED
DERIVED_AND_VERIFIED
EXCLUDED_NON_SEMANTIC
```

`UNKNOWN` запрещён.

## M5.5.2. Не включать по умолчанию

Pointers.
Allocator addresses.
Runtime transient handles.
Thread IDs.
Diagnostics.
Log timestamps.
Wall-clock timings.
Renderer backend identities.
Audio backend identities.
OS handles.

Exclusion requires reviewed reason.

## M5.5.3. Определить canonical semantic digest interface

Не добавлять digest methods во все public service APIs без необходимости.

Предпочтительно test/qualification adapters or internal visitors that use existing public snapshot/query contracts.

Digest contribution must be canonical.

## M5.5.4. Define encoding for digest inputs

Stable little-endian/integer encoding.
Explicit string length.
Sorted records.
Stable IDs.
No native struct bytes.
Floating fields require quantization/tolerance policy.

## M5.5.5. Создать digest coverage registry

Part of:

```text
docs/freeze/goal5_determinism_contract.json
```

Fields:

```text
module
state_id
decision
digest_adapter
ordering_policy
floating_policy
evidence
```

Validator ensures every deterministic-relevant state mapped.

---

# 5.5.B. Canonical scenario model

## M5.5.6. Создать scenario step enum/model

Minimum logical steps:

```text
SubmitCommand
AdvanceGameTime
Tick
CompleteExternalOperation
ApplyBudget
StreamOut
StreamIn
SaveReload
Reconcile
Materialize
```

Input data stable and serializable for reproduction.

## M5.5.7. Create scenario descriptor

Fields:

```text
scenario_id
config_id
seed_set
initial_state_id
steps
expected_contract
```

No lambda/callback identity in scenario description.

## M5.5.8. Create semantic event trace adapter

Only semantic commits.

Each event:

```text
event_type
stable subject/object IDs
semantic payload
logical order key
```

Internal retries not emitted as semantic commit unless contract says observable.

## M5.5.9. Create deterministic runner result

Result:

```text
semantic_digest
event_trace_digest
optional full normalized trace
step_count
failure metadata
```

Failure output must include:

Scenario ID.
Seed.
Step index.
Expected digest.
Actual digest.
First differing trace event.

## M5.5.10. Add runner self-tests

Adversarial fixtures:

1. Pointer address included.
2. Unordered iteration.
3. Ambient wall-clock.
4. Random seed from `random_device`.
5. Missing owner digest contribution.
6. Tie without stable order.
7. Duplicate event order.
8. Different async completion order treated implicitly.
9. Floating raw bytes.
10. Hidden static mutable state.

Self-test should prove runner detects divergence or contract validator rejects policy.

---

# 5.5.C. Add system target

## M5.5.11. Создать Truth target

```text
EpidemicDeterminismTruthTests
```

Labels:

```text
truth
determinism
```

Keep target headless where possible.

## M5.5.12. Implement control mini-scenario

Run same scenario twice in same process.

Compare:

Semantic digest.
Semantic trace.

Must match exactly under D1.

## M5.5.13. Implement fresh-composition mini-scenario

Build two independent compositions in same process.

Same initial state and steps.

Must match.

## M5.5.14. Implement fresh-process mini-scenario

Wrapper runs scenario executable in separate processes with same descriptor.

Result JSON files compared.

This establishes D2 infrastructure.

## M5.5.15. Закрыть 5.5 gate

Create:

```text
docs/freeze/goal5_evidence_quality.py
```

At this stage it verifies:

1. Digest coverage evidence is semantic.
2. Test target exists.
3. No generic setup-only anchor.
4. Scenario IDs unique.
5. Every required deterministic owner covered.

Run:

```powershell
python docs/freeze/goal5_evidence_quality.py --check
python docs/freeze/goal5_evidence_quality.py --self-test
ctest --test-dir build/ae2/full-debug -C Debug -L determinism --output-on-failure
```

Checkpoint.

---

---

# Stage-specific scope guard

## 10.5. 5.5

Expected production changes:

Mostly test/qualification infrastructure.

Avoid adding debug-only public methods solely for digest.

If introspection insufficient:

prefer internal friend/test adapters or existing snapshot/query APIs.

---

# 11. Stop conditions requiring design review

Execution must stop before continuing if any condition occurs.

## 11.1. New production module appears necessary

Reason:

Changes production inventory and local-ready baseline.

Required action:

Separate design decision and local audit.

## 11.2. SaveGame core needs direct gameplay major dependency

This violates current architecture intent.

Do not add dependency.

Move adapter to IntegrationLayer.

## 11.3. Runtime needs Framework dependency

Forbidden.

Move coordinator upward.

## 11.3a. Integration needs a new Runtime major dependency

Do not add it silently.

First try a narrow injected port.

If direct dependency remains necessary, treat the allowlist expansion as a reviewed architecture delta and extend architecture adversarial fixtures so the permission remains exact rather than broad.

## 11.4. Whole save cannot obtain consistent cut without broad global lock

Do not immediately introduce engine-wide mutex.

First compare epoch/snapshot-isolation alternatives.

## 11.5. Restore requires fallible operation after semantic commit with no retry state

This is a correctness defect.

Design explicit reconciliation ownership before proceeding.

## 11.6. Historical save version cannot be migrated unambiguously

Do not fabricate migration.

Decide compatibility cutoff and document unsupported version.

## 11.7. Determinism requires renderer/backend bit identity

Re-evaluate deterministic domain.

Do not accidentally make graphics backend state part of semantic contract.

## 11.8. Wall-clock budget affects semantic commit order

Treat as confirmed determinism risk.

Add explicit deterministic budget/commit policy.

## 11.9. Scanner produces unmanageable false positives

Do not whitelist broad directories blindly.

Improve symbol/context classification while preserving fail-closed coverage.

## 11.10. A validator passes by auto-repairing data

Forbidden.

`--check` must never mutate canonical state.

Generator and validator remain separate operations.

---

---

# 12. Concrete new test targets planned

Names are planned defaults.

If a target with equivalent responsibility already exists by execution time, reuse it rather than duplicate.

## 12.1. Existing targets to extend

```text
EpidemicGameFrameworkSaveGameTests
EpidemicGameFrameworkCoreIntegrationTests
EpidemicRuntimePersistenceTests
EpidemicGameFrameworkRuntimeBridgeTests
```

## 12.2. New Truth targets

Recommended:

```text
EpidemicPersistenceWireTruthTests
EpidemicWholeEngineRestoreTruthTests
EpidemicDeterminismTruthTests
EpidemicDeterminismSaveLoadTruthTests
```

Responsibilities:

`EpidemicPersistenceWireTruthTests`:
wire + storage + process reopen.

`EpidemicWholeEngineRestoreTruthTests`:
fresh composition restore and reconciliation.

`EpidemicDeterminismTruthTests`:
runner D1/D2 control scenarios.

`EpidemicDeterminismSaveLoadTruthTests`:
control versus save/load equivalence.

Do not create one target per tiny requirement.

---

---

# 13. Exact CMake integration policy

## 13.1. Existing module tests

Add labels with `set_tests_properties` after `epidemic_add_test`.

Example shape:

```cmake
set_tests_properties(
    EpidemicGameFrameworkSaveGameTests
    PROPERTIES LABELS "persistence-local"
)
```

Preserve existing labels if any.

## 13.2. Truth helper

Current `epidemic_add_truth_test` applies `truth`.

For additional labels:

```cmake
set_tests_properties(
    EpidemicWholeEngineRestoreTruthTests
    PROPERTIES LABELS "truth;persistence-whole-engine"
)
```

Be careful:

A later `set_tests_properties(... LABELS ...)` replaces label set.

Always include `truth` explicitly.

## 13.3. Timeouts

Assign explicit timeout to new whole-engine/process tests.

Do not rely on global default where scenario legitimately needs more time.

Keep under freeze timeout budget.

---

---

# 14. Evidence and validator quality rules

## 14.1. Forbidden evidence

Cannot be sole evidence:

`#include`.
`using`.
Type declaration.
Service object construction.
`return 0`.
`main()`.
Generic smoke pass.
File existence.
Comment.
Rationale without executable behavior where behavior is required.

## 14.2. Required evidence for behavior

Example:

```json
{
  "test_target": "EpidemicWholeEngineRestoreTruthTests",
  "test_symbol": "RestoreRebuildsRuntimeBridgeWithoutReusingOldHandles",
  "assertion": "old runtime observation is rejected and new mapping is consistent"
}
```

## 14.3. Validator self-test rule

Every newly introduced acceptance rule requires at least one negative fixture proving it can fail.

A validator with only happy-path tests is not accepted.

---

---

# Exit checklist for this part

## Deterministic runner

[x] Deterministic state coverage complete.
[x] Canonical digest.
[x] Semantic event trace.
[x] Reproduction metadata.
[x] Fresh-process support.


Qualification: M5.5.1–M5.5.15 PASS. Current Debug mini D1/D2 gate: `_goal5_handoff/current_stage55_debug_junit.xml`. Registry: 504 reviewed states, 152 included, 49 owners. Evidence gate rejects out-of-case assertions (failing regression then scoped-body fix). Stage 5.7 exact-tree qualification remains separate.
