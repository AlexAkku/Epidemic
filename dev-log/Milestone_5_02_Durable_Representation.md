# Part 2/7 — 5.2 Durable representation

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

### P2-A — Participant coverage graph
Requirements: `M5.2.1–M5.2.4`.

Цель: полное exact coverage durable Framework state participant graph без overlap/orphans/cycles.

### P2-B — Missing participant implementation
Requirements: `M5.2.5–M5.2.7`.

Цель: ownership-local adapters и executable participant behavior, после чего registry может быть frozen.

### P2-C — Schema/version compatibility
Requirements: `M5.2.8–M5.2.11`.

Цель: finite participant version contract, migration graph, historical fixtures и profile semantics.

### P2-D — Whole-image wire codec
Requirements: `M5.2.12–M5.2.15`.

Цель: explicit versioned/bounded deterministic wire representation и compatibility/golden evidence.

### P2-E — Durable storage/reopen
Requirements: `M5.2.16–M5.2.20`.

Цель: production storage boundary, deterministic backend evidence и fresh-process reopen semantics.

### P2-F — Durable representation gate
Requirement: `M5.2.21`.

Цель: закрыть 5.2 только после exact participant/schema/wire/storage qualification без orphan state.

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

# 4. Stage 5.2: Save participant, wire format and durable storage integration

# 5.2.A. Participant coverage

## M5.2.1. Построить participant coverage matrix

Из 5.1 получить все candidates:

```text
persistence_role in {PERSISTENT_OWNER, PERSISTENT_CHECKPOINT}
save_plane = FRAMEWORK_SAVEGAME
```

Создать:

```text
docs/freeze/goal5_participant_contract.json
```

Каждый persistent candidate должен быть покрыт participant ID.

Participant может покрывать:

1. Один owner.
2. Несколько tightly-coupled state records одного ownership boundary.

Запрещено:

1. Два participants владеют одним authoritative state.
2. Aggregate participant скрывает unrelated owners без явной причины.
3. Participant становится вторым authoritative owner.

## M5.2.2. Определить participant granularity

Для каждого gap выбрать:

```text
EXISTING_PARTICIPANT
NEW_PARTICIPANT
AGGREGATED_PARTICIPANT
RUNTIME_PERSISTENCE
EXTERNAL_DURABLE
```

Decision фиксируется в contract.

Не начинать coding до полного mapping.

## M5.2.3. Определить participant dependencies

Использовать semantic restore dependencies.

Не копировать include graph механически.

Для каждого participant:

```text
id
requirement
schema_version
dependencies
covered_candidate_ids
```

Прогнать graph validator ещё до реализации.

## M5.2.4. Реализовать `goal5_participant_contract.py`

Проверки:

1. All Framework durable candidates covered.
2. Coverage exactly one durable mechanism.
3. Participant IDs unique.
4. Dependencies known.
5. No cycles.
6. No uncovered candidate.
7. No participant without state.
8. No overlap authoritative coverage.
9. Required/optional semantics explicit.

Self-test:

Missing coverage.
Duplicate coverage.
Cycle.
Unknown dependency.
Orphan participant.
Overlapping owner.

---

# 5.2.B. Реализовать missing participants

## M5.2.5. Добавлять participants только ownership-locally или в existing integration adapters

Правило размещения:

1. Если participant сохраняет state одного owner и не требует peer module dependencies, adapter может находиться рядом с owner или в existing integration module according to architecture.
2. Если participant объединяет integration checkpoint, размещать в соответствующем IntegrationLayer module.
3. SaveGame core не должен линковаться на gameplay majors.
4. Не нарушать `_epidemic_save_game_forbidden_targets`.
5. Не добавлять peer dependencies ради удобства.

После каждого нового participant:

```powershell
cmake configure/build affected target
run affected module tests
python docs/freeze/architecture_ownership.py --check
```

## M5.2.6. Для каждого new participant сначала добавить tests

Минимальный test set:

1. Stable `Id()`.
2. `SchemaVersion()`.
3. `Dependencies()`.
4. Non-empty capture.
5. Empty capture where applicable.
6. Validate valid payload.
7. Reject corrupted payload.
8. Stage without mutating live state.
9. Commit publishes staged state.
10. Allocation/failure before commit preserves live state.
11. Generator/revision/cursor continuity if applicable.

Only then implement participant.

## M5.2.7. Freeze participant registry only after composition complete

Composition code должен:

1. Register all participants.
2. Register migrations.
3. Validate participant contract.
4. Freeze registry.
5. Reject late registration.

Добавить integration regression на exact participant set.

---

# 5.2.C. Schema/version contract

## M5.2.8. Создать `goal5_schema_contract.json`

Для каждого participant:

```text
participant_id
current_version
minimum_supported_version
supported_versions
migration_edges
compatibility_profiles
fixture_ids
```

Если current version 1 и historical support отсутствует:

```text
migration_policy = NO_PRIOR_SUPPORTED_VERSION
```

Не создавать фиктивную version 0.

## M5.2.9. Реализовать `goal5_schema_contract.py`

Проверить:

1. Current version > 0.
2. Supported range coherent.
3. No migration gap.
4. No migration cycle.
5. Migration target eventually reaches current.
6. Historical supported versions have fixtures.
7. Version-1-only participant may use `NO_PRIOR_SUPPORTED_VERSION`.
8. Future version rejected under strict profiles.
9. Profile policy explicit.

Self-tests обязательны.

## M5.2.10. Создать compatibility fixture directory

Рекомендуемый path:

```text
Tests/Fixtures/SaveGame/
```

Structure:

```text
manifest.json
participant/<id>/v1.bin
participant/<id>/v2.bin
whole/<format-version>/...
```

Fixture content считается source-controlled test contract.

Нельзя перегенерировать historical fixture автоматически во время normal CI.

## M5.2.11. Проверить все существующие compatibility profiles

Отдельные tests для:

```text
ExactRequired
CompatibleVersion
MigrationRequired
BestEffort
DeveloperMode
```

Production strict qualification:

`ExactRequired`.
`CompatibleVersion`.
`MigrationRequired` where applicable.

`BestEffort` и `DeveloperMode` тестируются как permissive behavior, но не используются для strict persistence PASS.

---

# 5.2.D. Whole SaveGameImage wire codec

## M5.2.12. Зафиксировать codec requirements до coding

Нужно принять decision record в machine-readable schema contract.

Требования:

1. Magic/format identity.
2. Save format version.
3. Fixed integer encoding.
4. Explicit enum encoding.
5. Length-prefix bounds.
6. Participant IDs.
7. Participant schema versions.
8. Manifest.
9. Sections.
10. Hash/checksum policy.
11. Deterministic ordering.
12. Reject trailing/invalid structure according to contract.
13. No native object memory dumps.
14. No pointer/STL layout dependency.

Формат может быть custom binary или другой explicit format.

Выбор формата не должен менять semantic API `SaveGameImage`.

## M5.2.13. Реализовать codec внутри SaveGame boundary

Предпочтительно:

```text
EngineFramework/GameplayWorldStateOwners/SaveGame
```

SaveGame core может владеть encode/decode representation.

Он не должен выполнять filesystem I/O.

Если public API не требуется приложению напрямую:

держать implementation surface минимальной.

Если нужен public codec contract:

public API delta проходит existing public surface review.

## M5.2.14. Добавить codec tests в `EpidemicGameFrameworkSaveGameTests`

Обязательные tests:

1. Empty image roundtrip.
2. Multi-participant roundtrip.
3. Stable byte output for same image.
4. Corrupted magic.
5. Unsupported format version.
6. Truncated header.
7. Truncated participant table.
8. Truncated payload.
9. Invalid size.
10. Duplicate participant.
11. Hash mismatch.
12. Unknown required participant semantics.
13. Optional unknown participant semantics by profile.
14. Trailing bytes policy.
15. Resource limit overflow.
16. Malformed dependency list.
17. Deterministic participant/section ordering.

## M5.2.15. Создать codec golden fixtures

Для wire format itself создать small golden fixtures.

Проверить:

Decode historical fixture.
Re-encode according to current canonical format.
Semantic equality.

Не требовать byte-equality после migration, если contract этого не обещает.

---

# 5.2.E. Durable storage boundary

## M5.2.16. Ввести storage abstraction без OS dependency в SaveGame core

Если storage interface отсутствует, добавить minimal interface, например conceptually:

```text
ISavePackageStore
Load
Commit
```

Точные names определяются implementation review.

Interface должен выражать semantic operations, не filesystem primitives.

SaveGame core не должен зависеть от Windows API.

## M5.2.17. Разместить production filesystem implementation вне SaveGame core

Default placement:

existing `EngineFramework/IntegrationLayer/Integration` либо composition-support implementation, если dependency set остаётся coherent.

Не добавлять новый production module по умолчанию.

Implementation должен использовать explicit atomic replacement policy.

На Windows проверить:

Temporary file.
Flush policy if contract promises durability.
Rename/replace behavior.
Failure cleanup.
Old valid save preservation.

## M5.2.18. Добавить deterministic in-memory storage test backend

Test backend должен поддерживать controlled failures:

Fail open.
Fail write.
Fail commit/replace.
Return truncated bytes.
Return corrupted bytes.
Simulate old valid generation.

Не использовать global allocation override.

## M5.2.19. Добавить storage tests

System-level target:

можно добавить `EpidemicPersistenceWireTruthTests` в `Tests/Truth`.

Labels:

```text
truth
persistence-wire
persistence-storage
```

Scenarios:

1. Encode, commit, reopen, decode.
2. Failure before commit preserves old save.
3. Failure during temp write.
4. Failure during replace.
5. Corrupted stored save.
6. Missing save.
7. Oversized save.
8. Multiple sequential generations.
9. Fresh process reopen smoke.

## M5.2.20. Process-boundary smoke

Добавить отдельный helper executable или test mode.

Phase A:

Write save to temp test directory and exit.

Phase B:

New process opens file, validates and restores minimal known state.

CTest fixture or wrapper PowerShell/Python may coordinate two process invocations.

Не использовать static memory between phases.

## M5.2.21. Закрыть 5.2 validators

Запустить:

```powershell
python docs/freeze/goal5_participant_contract.py --check
python docs/freeze/goal5_participant_contract.py --self-test
python docs/freeze/goal5_schema_contract.py --check
python docs/freeze/goal5_schema_contract.py --self-test
ctest --test-dir build/ae2/full-debug -C Debug -L persistence-wire --output-on-failure
ctest --test-dir build/ae2/full-debug -C Debug -L persistence-storage --output-on-failure
```

Также:

```powershell
.\build.ps1 -Mode Validators
```

Exit:

```text
all durable Framework state covered
schema gaps = 0
orphan participant = 0
wire codec qualified
durable storage reopen qualified
historical fixtures complete for supported versions
```

Checkpoint.

---

---

# Stage-specific scope guard

## 10.2. 5.2

Expected production changes:

Save participants.
Migration implementations where real historical schema requires.
SaveGame image codec.
Storage abstraction.
Production storage implementation.
Minimal composition wiring.

High risk:

Public SaveGame API delta.
Participant dependency mistakes.
Wire compatibility.

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

## Participant coverage

[ ] Every Framework persistent owner/checkpoint covered.
[ ] Runtime persistence plane mapped.
[ ] No overlapping participant ownership.
[ ] Participant dependencies acyclic.
[ ] Required/optional semantics reviewed.


## Schema and wire

[ ] Current schema known for every participant.
[ ] Historical support policy explicit.
[ ] No artificial migration fixture.
[ ] Supported historical versions have fixtures.
[ ] Wire format bounded/versioned.
[ ] Corruption/truncation rejected.


## Storage

[ ] Production durable storage path exists.
[ ] Failed write preserves last valid save.
[ ] Fresh-process reopen works.
[ ] Storage errors controlled.

