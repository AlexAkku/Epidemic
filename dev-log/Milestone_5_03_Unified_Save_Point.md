# Part 3/7 — 5.3 Unified save point

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

### P3-A — Consistent-cut design
Requirements: `M5.3.1–M5.3.6`.

Цель: finite inventory save-sensitive activity + reviewed consistent-cut/generation architecture без layering regression.

### P3-B — Cross-plane coordination
Requirements: `M5.3.7–M5.3.11`.

Цель: production coordination Framework/Runtime Time/Runtime Persistence/participants под одним generation contract и fail-closed validator.

### P3-C — Save fault/accepted-work qualification
Requirements: `M5.3.12–M5.3.16`.

Цель: controlled failure seams, mutation/accepted-work invariants, retry/release behavior и закрытый 5.3 gate.

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

# 5. Stage 5.3: Unified save barrier and atomic save generation

# 5.3.A. Сначала описать consistent-cut contract

## M5.3.1. Построить inventory save-sensitive activities

На основе 5.1 собрать categories:

Authoritative mutations.
Accepted external work.
Pending queues.
Deferred work.
Runtime persistence transaction.
Runtime Time state.
Framework Time state.
Streaming transitions.
RuntimeBridge materialization.
Integration reconciliation.

Для каждой category решить:

```text
QUIESCE
SNAPSHOT_ISOLATED
DURABLE_PENDING
CANCEL_BEFORE_ACCEPT
BLOCK_SAVE
NON_SEMANTIC
```

Записать:

```text
docs/freeze/goal5_save_generation_contract.json
```

## M5.3.2. Выбрать consistent-cut mechanism

Не кодировать заранее глобальную pause.

Сначала определить, какой механизм минимально подходит текущей архитектуре.

Разрешено:

1. Quiescence barrier.
2. Epoch/generation barrier.
3. Immutable snapshot isolation.
4. Hybrid.

Decision должен ответить:

1. Когда capture point считается установленным.
2. Какие mutations разрешены после point.
3. Как pending accepted work относится к generation.
4. Как Time связан с generation.
5. Как Runtime Persistence связан с generation.
6. Как Framework participant payloads связаны с generation.

## M5.3.3. Определить `SaveGenerationId`

Если existing types не дают stable cross-plane identity:

добавить минимальный type в integration/save coordination contract.

Он должен:

Stable compare.
Not pointer-derived.
Not wall-clock-derived unless wall time is only metadata.
Be serialized in whole save package.
Be validated on restore.

## M5.3.4. Не помещать cross-plane coordinator в Runtime

Default candidate placement:

```text
EngineFramework/IntegrationLayer/Integration
```

Причина:

1. Module already integrates SaveGame and Runtime Time.
2. Framework is the upper layer.
3. Runtime must not depend on Framework.

Критическое admission ограничение:

Текущий `ArchitectureFreeze.cmake` allowlist разрешает только:

```text
EpidemicGameFrameworkCoreIntegration:EpidemicRuntimeTime
```

Поэтому direct dependency:

```text
EpidemicGameFrameworkCoreIntegration -> EpidemicRuntimePersistence
```

сейчас является запрещённой.

Порядок решения:

1. Сначала попытаться определить narrow persistence coordination port на Framework/integration boundary.
2. Runtime-specific adapter/implementation должен оставаться в разрешённой composition boundary.
3. Проверить, можно ли передать Runtime persistence operations через injected interface без нового direct Framework-to-Runtime major dependency.
4. Только если это невозможно без дублирования ownership или ложного abstraction, оформить explicit architecture change proposal.
5. Architecture change proposal должен перечислить exact target pair, include prefix и причину.
6. Добавить positive fixture, подтверждающий только новую разрешённую boundary.
7. Добавить negative fixtures, подтверждающие, что остальные Runtime dependencies по-прежнему запрещены.
8. Обновить `ArchitectureFreeze.cmake`, `ArchitectureFreezeSelfTest.cmake` и canonical architecture evidence.
9. Прогнать полный architecture/freeze gate.
10. Только после этого использовать новую direct dependency.

Нельзя тихо расширять allowlist как обычную CMake правку.

## M5.3.5. Проверить architecture after coordination design

До production implementation выполнить:

```powershell
python docs/freeze/architecture_ownership.py --check
python docs/freeze/architecture_ownership.py --self-test
cmake -P cmake/ArchitectureFreezeSelfTest.cmake
```

Если planned dependency не проходит текущий gate:

это design stop, а не повод временно выключить architecture checks.

## M5.3.6. Не создавать новый production module без stop/review

Если existing Integration becomes incoherent:

остановить execution.

Отдельно решить вопрос нового module.

Если новый module принят:

1. Production module count changes.
2. Goal 1 discovery must see it.
3. Module dossier required.
4. Local-ready 37 criteria required.
5. Public surface reviewed.
6. CTest target required.

Это отдельный mini-admission, не скрытый step.

---

# 5.3.B. Production barrier/coordination

## M5.3.7. Реализовать production coordination mechanism

Если выбран `ISaveBarrier`:

добавить production implementation в Integration layer.

Если выбран другой mechanism:

не удалять `ISaveBarrier` без отдельного public contract review.

Coordinator должен предоставлять SaveGame orchestrator достаточный lease/epoch contract.

## M5.3.8. Связать Runtime Time

Capture должен фиксировать Time checkpoint той же generation.

Проверить:

Fractional remainder.
Scale/pause state if persistence contract requires.
No wall-clock state reused as gameplay deterministic state.

## M5.3.9. Связать Runtime Persistence

Runtime Persistence snapshot/transaction получает same logical generation metadata.

Если текущий `PersistenceSnapshot` не имеет generation field:

не добавлять blindly.

Сначала определить, где generation лучше хранить:

whole-package manifest.
Runtime persistence envelope.
Both with validation.

Нужен один source of truth и explicit consistency check.

## M5.3.10. Связать Framework participants

`SaveContext` может быть расширен только если generation нужен participant semantics.

Если generation нужен только package coordinator:

не раздувать public participant API.

Минимизировать public surface delta.

## M5.3.11. Добавить save generation validator

`goal5_save_generation_contract.py` проверяет:

1. All save planes represented.
2. One generation source.
3. Time policy present.
4. Runtime Persistence policy present.
5. Framework SaveGame policy present.
6. Every pending category has save policy.
7. No unknown async boundary.
8. No Runtime -> Framework dependency.
9. No duplicated authoritative generation state.

Self-tests:

Mismatched generation.
Missing plane.
Unknown pending category.
Illegal dependency.
Duplicate generation owner.

---

# 5.3.C. Failure qualification

## M5.3.12. Создать controlled fault seams

Fault seams только narrow/private.

Required logical fault points:

Barrier acquire.
Framework capture.
Runtime persistence capture.
Time checkpoint.
Package encode.
Storage commit.
Barrier release bookkeeping where fallible.

Не использовать process-global allocation injection.

## M5.3.13. Добавить save coordination tests

Target:

existing `EpidemicGameFrameworkCoreIntegrationTests` для module-level coordination.

New Truth target for whole system may be introduced later in 5.4.

Tests:

1. Idle save.
2. Pending deferred work.
3. Accepted external work.
4. Streaming transition.
5. Budget-deferred work.
6. Barrier acquisition failure.
7. Participant capture failure.
8. Runtime Persistence failure.
9. Time checkpoint failure.
10. Encode failure.
11. Storage failure.
12. Retry after failure.
13. Barrier released after failure.
14. No valid package published on failed save.
15. Previous valid package retained.

## M5.3.14. Проверить no hidden mutation during capture

Add adversarial fake participant/service that attempts prohibited mutation during capture.

Expected:

Rejected or isolated according to contract.

Test must prove mechanism, not just documentation.

## M5.3.15. Проверить accepted-work semantics

Для каждого `DURABLE_PENDING` category:

1. Create accepted work.
2. Stop before terminal publication.
3. Save.
4. Restore.
5. Reconcile.
6. Verify side effect exactly once.

## M5.3.16. Закрыть 5.3 gate

Commands:

```powershell
python docs/freeze/goal5_save_generation_contract.py --check
python docs/freeze/goal5_save_generation_contract.py --self-test
python docs/freeze/architecture_ownership.py --check
cmake -P cmake/ArchitectureFreezeSelfTest.cmake
```

Run affected tests.

Exit:

```text
one logical save point
all persistence planes consistent
all pending categories classified
save failure never publishes invalid package
architecture layering unchanged
```

Checkpoint.

---

---

# Stage-specific scope guard

## 10.3. 5.3

Expected production changes:

Save coordination.
Integration layer.
Runtime Persistence coordination.
Time/save generation wiring.
Narrow fault seams.

High risk:

Architecture layering.
Accepted work.
Half-save generation.

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

## Save generation

[ ] One logical save point.
[ ] Framework SaveGame included.
[ ] Runtime Persistence included.
[ ] Time included.
[ ] Pending accepted work has save policy.
[ ] Failed save never publishes valid-looking incomplete package.

