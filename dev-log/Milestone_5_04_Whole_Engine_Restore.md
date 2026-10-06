# Part 4/7 — 5.4 Whole-engine restore

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

### P4-A — Restore contract
Requirements: `M5.4.1–M5.4.3`.

Цель: production-driven restore phases/dependency graph и validator.

### P4-B — Fresh-composition happy path
Requirements: `M5.4.4–M5.4.13`.

Цель: reusable whole-engine harness, non-trivial state, production save, destruction, fresh composition, restore, identity rematerialization и causal continuation.

### P4-C — Restore failure matrix
Requirements: `M5.4.14–M5.4.21`.

Цель: finite phase-by-phase failure qualification включая process-boundary storage smoke.

### P4-D — Whole-engine restore gate
Requirement: `M5.4.22`.

Цель: 5.4 PASS только если happy path и complete failure matrix machine-proven.

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

# 6. Stage 5.4: Whole-engine ordered restore and reconciliation

# 5.4.A. Restore contract

## M5.4.1. Создать restore phase model

Machine-readable:

```text
docs/freeze/goal5_restore_contract.json
```

Phases:

```text
LOAD
VALIDATE
STAGE
COMMIT_SEMANTIC
REBUILD_TRANSIENT
RECONCILE_EXTERNAL
MATERIALIZE_RUNTIME
RESUME
```

Если implementation использует другие names:

contract mapping должен показывать equivalent semantics.

## M5.4.2. Построить restore dependency graph

Source:

Participant dependencies.
Runtime persistence dependencies.
Time dependency.
Integration checkpoints.
RuntimeBridge reconciliation.

Graph должен быть production-derived.

Test harness не должен иметь отдельный hard-coded restore order.

## M5.4.3. Реализовать `goal5_restore_contract.py`

Проверить:

1. Every persistent candidate covered.
2. Every participant in graph.
3. Runtime persistence plane included.
4. Time included.
5. Dependencies known.
6. No cycle.
7. Precommit fallible work separated.
8. Postcommit reconciliation policy exists.
9. Resume guard exists.
10. RuntimeBridge rebuild policy exists.

Self-tests обязательны.

---

# 5.4.B. Whole-engine harness

## M5.4.4. Добавить system target

Создать в `Tests/Truth`:

```text
EpidemicWholeEngineRestoreTruthTests
```

Label:

```text
truth
persistence-whole-engine
```

Target links only required production modules.

Do not create test-only alternative orchestration semantics.

## M5.4.5. Реализовать reusable test composition builder

Builder должен создавать fresh:

EngineBase headless composition.
Runtime support/services required by scenario.
Framework services.
Integration adapters.
SaveGame participants.
Save coordinator.
RuntimeBridge.

Builder test utility не является production owner.

## M5.4.6. Реализовать non-trivial seed scenario

Минимальное state:

1. Gameplay time advanced.
2. Several entities.
3. At least one generator/revision advanced.
4. Inventory/progression or other durable gameplay state.
5. Pending integration checkpoint.
6. Runtime persistent record.
7. At least one materialized Runtime representation.

## M5.4.7. Выполнить production save

Harness обязан пройти:

Production coordination.
SaveGame orchestrator.
Runtime persistence plane.
Codec.
Storage backend.

Не разрешается напрямую складывать snapshots в test struct.

## M5.4.8. Полностью уничтожить composition

Explicit destruct:

Framework instances.
Runtime instances.
Base composition where test architecture allows.

Очистить all test-owned handles.

No object from old composition may be passed into restore.

## M5.4.9. Создать fresh composition

New instances.
New runtime handles.
New allocations.

Load storage package.

## M5.4.10. Выполнить production restore

Path:

Load.
Decode.
Validate.
Stage.
Semantic commit.
Transient rebuild.
Reconcile.
Materialize.
Resume.

## M5.4.11. Проверить authoritative state

Compare semantic state:

IDs.
Generations.
Revisions.
Journals/cursors where persistent.
Pending durable records.
Time.
Random state where applicable.

Не сравнивать pointers.

## M5.4.12. Проверить RuntimeBridge identity

Assertions:

1. Semantic object ID preserved.
2. Old runtime handle invalid/not reused as identity.
3. New runtime representation created.
4. Forward/reverse maps consistent.
5. Stale old observation rejected.
6. Retry does not duplicate representation.

## M5.4.13. Продолжить causal scenario

После restore выполнить further semantic operations.

Проверить:

No missing dependency.
No stale cursor.
No duplicated side effect.
Time progresses.
Simulation resumes only after ready.

---

# 5.4.C. Failure matrix

## M5.4.14. LOAD failure

Cases:

Missing file.
Corrupt package.
Truncated package.
Unsupported save format.

Expected:

No live semantic mutation.

## M5.4.15. VALIDATE failure

Inject participant validation failure.

Expected:

No live state changed.
No Runtime materialization.
Resume not entered.

## M5.4.16. STAGE failure

Inject controlled stage failure.

Expected:

No live state changed.
All prepared temporary state released.

## M5.4.17. Semantic commit preparation failure

All fallible work must finish before final no-throw commit.

Test attempts allocation/fault before commit boundary.

Expected:

Commit not started.

## M5.4.18. RECONCILE_EXTERNAL failure

Inject backend/provider failure after semantic restore.

Expected:

Semantic state remains restored.
Engine enters reconciliation-required state.
Simulation does not resume.
Retry succeeds without duplicate external side effect.

## M5.4.19. MATERIALIZE_RUNTIME failure

Expected:

Semantic state retained.
No duplicate runtime object.
Retry safe.
Old runtime identity not accepted.

## M5.4.20. RESUME guard

Try tick/simulation before reconciliation complete.

Expected:

Controlled rejection/no-op according to contract.

## M5.4.21. Fresh-process storage smoke

Run save writer process.
Exit.
Run restore process.
Validate known semantic digest.

This smoke only needs representative state.

Whole suite does not need process spawn for every case.

## M5.4.22. Закрыть 5.4 gate

Run:

```powershell
python docs/freeze/goal5_restore_contract.py --check
python docs/freeze/goal5_restore_contract.py --self-test
ctest --test-dir build/ae2/full-debug -C Debug -L persistence-whole-engine --output-on-failure
```

Then full affected Debug.

Exit:

```text
fresh-composition restore PASS
process-boundary reopen PASS
precommit failure atomicity PASS
postcommit reconciliation PASS
RuntimeBridge rebuild PASS
continuation PASS
```

Checkpoint.

---

---

# Stage-specific scope guard

## 10.4. 5.4

Expected production changes:

Restore coordinator/reconciliation state if missing.
RuntimeBridge reconciliation hooks only if existing contract insufficient.

High risk:

Postcommit rollback illusion.
Runtime identity reuse.
Simulation resume too early.

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

## Restore

[x] Fresh composition restore.
[x] Production dependency order.
[x] Derived indexes valid.
[x] RuntimeBridge rebuilt.
[x] Old transient handles rejected.
[x] Precommit failures atomic.
[x] Postcommit reconciliation retry-safe.
[x] Simulation resumes only when ready.

Qualification 2026-10-05: M5.4.1–M5.4.22 PASS. Full Debug 116/116 and Release whole/process runtime PASS. Evidence: `docs/freeze/goal5_stage54_execution.md`, `_goal5_handoff/current_stage54_full_debug_report.json`, `_goal5_handoff/reconciliation_review.json`. Final exact-tree MS5 qualification remains stage 5.7.

