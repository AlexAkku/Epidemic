# Milestone 5: пошаговый execution plan
## Persistence and Determinism Qualification

Дата: 2026-10-01

Основа: `Milestone_5_Persistence_Determinism_Qualification_Plan_2026-10-01_v2.md`

Статус: конкретный последовательный план выполнения Milestone 5.

Этот документ не заменяет архитектурный план v2. Он задаёт порядок исполнения, конкретные артефакты, файлы, validators, tests, stop conditions и правила внесения production fixes.

---

# 0. Проверенный admission baseline и обязательные ограничения

## 0.1. Подтверждённые факты текущего дерева

Перед составлением этого execution plan повторно проверен baseline `Epidemic Engine 01-10-26-1`.

Подтверждено:

1. `EngineBase`, `EngineRuntime`, `EngineFramework` сохраняют установленное layering.
2. `SaveGame` core находится в:
   `EngineFramework/GameplayWorldStateOwners/SaveGame`.
3. `SaveGame` core уже предоставляет:
   `ISaveBarrier`,
   `ISaveParticipant`,
   `ISaveMigration`,
   `SaveGameOrchestrator`,
   `SaveGameImage`,
   `SaveGameManifest`,
   `SaveSection`,
   `ContentCompatibilityPolicy`.
4. `SaveGameOrchestrator` уже поддерживает:
   participant registration,
   dependency order,
   schema version,
   migration,
   validation,
   staging,
   allocation-free/noexcept final commit,
   required/optional participant semantics,
   resource limits.
5. На admission baseline production implementations `ISaveParticipant` обнаружено две:
   `ScheduledTriggerDispatcherSaveParticipant`,
   `NarrativeExternalConsequenceSaveParticipant`.
6. `ISaveBarrier` production implementation на admission baseline не обнаружена. Текущие implementations находятся только в tests.
7. Runtime persistence plane находится в:
   `EngineRuntime/Persistence`.
8. `IPersistenceBackend` существует.
9. Production backend на admission baseline:
   `InMemoryPersistenceBackend`.
10. Durable filesystem/external persistence backend для whole save package не обнаружен.
11. `EngineFramework/IntegrationLayer/Integration` уже связывает:
    `SaveGame`,
    Framework Time,
    Runtime Time.
12. Текущий `ArchitectureFreeze.cmake` разрешает Integration-layer Runtime dependency только для пары:
    `EpidemicGameFrameworkCoreIntegration -> EpidemicRuntimeTime`.
    Прямой `CoreIntegration -> RuntimePersistence` на admission baseline запрещён.
13. Поэтому существующий `Integration` является default candidate placement для Goal 5 coordination logic, но не даёт автоматического права добавить Runtime Persistence dependency.
14. Предпочтение: узкий injected port/interface, который сохраняет frozen layering и позволяет composition предоставить Runtime persistence operations.
15. Если direct Runtime Persistence dependency действительно необходима и narrow port не подходит, расширение architecture allowlist выполняется только как отдельный reviewed architecture delta с positive/negative architecture fixtures.
16. Создание нового Framework production module не является частью default plan, потому что это изменит production-unit inventory и потребует отдельного local-ready admission.
14. `Tests/Truth` уже является местом для system-level qualification.
15. Текущие Truth labels:
    `truth`,
    `truth;load`,
    `truth;smoke`.
16. Текущий build runner:
    `build.ps1`.
17. Полная Windows matrix запускается:
    `.\build.ps1 -Mode Matrix -Configuration Both`.
18. Canonical freeze validators находятся в `docs/freeze`.
19. CI source of truth не должен зависеть от Milestone/roadmap Markdown.
20. Goal 5 должен расширять существующий `freeze-contract`, а не создавать второй независимый validation pipeline.

Если к началу фактического выполнения код изменился, эти числа и конкретные реализации перепроверяются автоматически. Нельзя подгонять discovery под этот документ.

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

# 2. Stage 0: Milestone 5 admission

## M5.0.1. Создать чистый рабочий baseline

Действие:

1. Взять current Goal 4 completed tree.
2. Убедиться, что последняя note-independence/closure delta уже интегрирована либо эквивалентные изменения присутствуют.
3. Удалить build directories или использовать `-Clean`.

Проверка:

```powershell
git status --short
```

Ожидание:

Рабочее дерево известно и контролируемо.

## M5.0.2. Проверить независимость freeze contract от рабочих заметок

Выполнить поиск:

```powershell
Get-ChildItem docs/freeze,.github,cmake -Recurse -File |
    Select-String -Pattern 'Milestone 4\.md|Milestone 5\.md|work-plan\.md'
```

Допустимо:

Документационные ссылки, которые не участвуют в executable CI.

Недопустимо:

Validator или workflow читает рабочий roadmap как machine source of truth.

При failure:

Сначала исправить freeze pipeline. Milestone 5 не начинать.

## M5.0.3. Прогнать существующие validators

Запустить:

```powershell
.\build.ps1 -Mode Validators
```

Дополнительно явно:

```powershell
python docs/freeze/ci_gate_contract.py --check
python docs/freeze/ci_gate_contract.py --self-test
cmake -P cmake/ArchitectureFreezeSelfTest.cmake
```

Stop condition:

Любой red gate блокирует Goal 5.

## M5.0.4. Прогнать admission Windows matrix

Запустить:

```powershell
.\build.ps1 -Mode Matrix -Configuration Both -Clean
```

Этот шаг должен включить:

Base Debug.
Base Release.
Runtime Debug.
Runtime Release.
Full Debug.
Full Release.
Warnings as errors.
Public-header self-containment.
Exact CTest manifest.
Tests.

Записать:

Git SHA.
Test counts.
Compiler version.
CMake version.
Python version.
Current public header/API count.
Current production module count.

Нельзя hard-code counts как будущий validator contract.

## M5.0.5. Записать Goal 5 admission file

Создать machine-readable:

```text
docs/freeze/goal5_admission.json
```

Поля минимум:

```json
{
  "source_sha": "...",
  "production_module_count": 0,
  "public_header_count": 0,
  "public_api_count": 0,
  "profiles": {},
  "toolchain": {},
  "goal4_contract": "PASS"
}
```

Добавить validator:

```text
docs/freeze/goal5_admission.py
```

Он проверяет:

1. Required fields.
2. SHA format.
3. Production discovery count matches current tree.
4. Current Goal 4/local-ready contract PASS.
5. No hard dependency on roadmap Markdown.

## M5.0.6. Self-test admission validator

Negative fixtures:

1. Missing source SHA.
2. Wrong production count.
3. Missing Goal 4 completion.
4. Unknown profile.
5. Duplicate profile.
6. Missing toolchain field.

Только после PASS перейти к 5.1.

---

# 3. Stage 5.1: Persistence ownership map and save contract

# 5.1.A. Построить автоматический candidate inventory

## M5.1.1. Реализовать production module discovery reuse

Не писать второй module scanner.

Использовать существующий production discovery logic из Goal 1–4, если он доступен как importable helper.

Если текущий helper невозможно безопасно импортировать:

1. Вынести общий discovery primitive без изменения semantics.
2. Подключить к старым validators.
3. Доказать zero diff старых outputs.
4. Только затем использовать в Goal 5.

Цель:

Goal 5 видит автоматически найденный production inventory.

## M5.1.2. Реализовать `goal5_persistence_inventory.py`

Scanner работает только по:

```text
EngineBase production source/header
EngineRuntime production source/header
EngineFramework production source/header
existing module dossiers
existing local_ready_ledger
```

Исключить:

```text
Tests
DevelopmentInfrastructure/Tests
build
third-party
generated fixtures
Deltas
archives
working notes
```

## M5.1.3. Найти snapshot-like API candidates

Scanner должен искать минимум symbols/patterns:

```text
CaptureSnapshot
RestoreSnapshot
Checkpoint
CaptureCheckpoint
RestoreCheckpoint
SaveParticipant
ISaveParticipant
PersistenceSnapshot
IPersistenceBackend
SaveTransaction
Journal
Cursor
Reservation
Outbox
Reconciliation
RandomSequenceSnapshot
IdGenerator snapshot/state
```

Scanner сохраняет:

Qualified symbol.
Module.
Path.
Kind.
Public/private visibility.
Detected related methods.
Detected state type.

## M5.1.4. Найти durable-looking state candidates

Автоматически искать public/internal structs/classes, содержащие fields с именами/типами, характерными для persistence:

```text
revision
generation
sequence
cursor
epoch
pending
reservation
outbox
checkpoint
tombstone
journal
random state
seed
stream
```

Это candidates.

Scanner не имеет права автоматически помечать их persistent.

## M5.1.5. Найти опасные durable-field candidates

Проверить candidate state на:

Pointer types.
Raw pointers.
Function/callback objects.
Thread IDs.
Runtime handles.
Backend handles.
Allocator objects.
Wall-clock timestamps.
Process-local tokens.
Opaque IDs без remapping policy.

Output:

`risk_flags`.

Нахождение risk flag не равно defect.

## M5.1.6. Сопоставить с existing Goal 3/4 evidence

Для каждого candidate scanner пытается найти:

Module dossier.
Local-ready persistence criteria.
Existing test target.
Existing test evidence.
Existing snapshot contract.

Сгенерировать:

```text
docs/freeze/goal5_persistence_candidates.json
```

Sort order:

```text
module
candidate kind
qualified symbol
```

Output должен быть byte-stable при повторном run.

## M5.1.7. Сделать self-tests scanner

Fixtures минимум:

1. Real persistent owner.
2. Read-only snapshot.
3. Checkpoint.
4. Pointer field.
5. Runtime handle field.
6. Journal cursor.
7. Random stream state.
8. False positive in test directory.
9. False positive in generated fixture.
10. Duplicate symbol.

Scanner должен:

Find production candidates.
Ignore excluded scopes.
Produce stable IDs.
Never assign semantic PASS itself.

## M5.1.8. Первый discovery run

Запустить:

```powershell
python docs/freeze/goal5_persistence_inventory.py --record
python docs/freeze/goal5_persistence_inventory.py --check
python docs/freeze/goal5_persistence_inventory.py --self-test
```

Сохранить candidate count по layer/module.

Не интерпретировать count как acceptance criterion.

---

# 5.1.B. Создать reviewed persistence contract

## M5.1.9. Создать `goal5_persistence_contract.json`

Для каждого candidate заполнить:

```text
candidate_id
module
qualified_symbol
persistence_role
save_plane
authoritative_owner
capture_contract
restore_contract
participant_id
durable_schema
migration_policy
dependencies
transient_rebuild
reconciliation
deterministic_relevance
digest_policy
evidence
review_status
rationale
```

Разрешённые `persistence_role`:

```text
PERSISTENT_OWNER
PERSISTENT_CHECKPOINT
RECONSTRUCTED_TRANSIENT
READ_ONLY_SNAPSHOT
NON_PERSISTENT_OWNER
```

Разрешённые `save_plane`:

```text
FRAMEWORK_SAVEGAME
RUNTIME_PERSISTENCE
EXTERNAL
NONE
```

Review status:

```text
DISCOVERED
REVIEWED
BLOCKED
```

## M5.1.10. Классифицировать module-level persistence applicability

Для каждого discovered production module создать module record:

```text
module
applicability
reason
candidate_ids
```

Applicability:

```text
APPLICABLE
NOT_APPLICABLE
BLOCKED
```

Нельзя автоматически ставить `NOT_APPLICABLE` только потому, что scanner не нашёл `CaptureSnapshot`.

Нужно проверить module dossier и owned state.

## M5.1.11. Классифицировать Base

Ожидаемая модель для большинства Base modules:

`RECONSTRUCTED_TRANSIENT` или `NON_PERSISTENT_OWNER`.

Но это не предзаполненный PASS.

Для каждого Base module проверить:

1. Есть ли state, который влияет на future semantic gameplay.
2. Должен ли он переживать full save/load.
3. Если нет, почему fresh composition является правильным restore behavior.

## M5.1.12. Классифицировать Runtime

Особое внимание:

```text
Time
Persistence
Resources
Streaming
World
Simulation
Scene
Physics
Navigation
Animation
Audio
Environment
Renderer
Support
```

Не путать runtime operational snapshot с durable semantic persistence.

`Runtime Persistence` state относится к `save_plane=RUNTIME_PERSISTENCE`.

`Time` checkpoint проверяется как deterministic continuation state.

## M5.1.13. Классифицировать Framework state owners

Использовать существующее Goal 4 evidence.

Для каждого state owner:

1. Определить persistent/non-persistent role.
2. Проверить generator/revision/journal persistence.
3. Указать save plane.
4. Если `FRAMEWORK_SAVEGAME`, отметить participant coverage gap, если participant ещё нет.
5. Не писать participant на этом этапе.

## M5.1.14. Классифицировать Framework integration checkpoints

Особое внимание:

Pending deliveries.
Reservations.
Outboxes.
Durable cursors.
Accepted-work checkpoints.
Reconciliation records.

Определить:

Persistent checkpoint.
Reconstructable transient.
External durable state.

Каждое решение должно иметь rationale.

## M5.1.15. Классифицировать RuntimeBridge state

Проверить forward/reverse mappings.

Expected contract, если код подтверждает:

```text
persistence_role = RECONSTRUCTED_TRANSIENT
save_plane = NONE
```

Проверить отдельно semantic IDs, которые создают mappings.

Добавить explicit invariant:

Old runtime handles are not durable reusable identity.

Если это не доказано existing tests:

создать gap для 5.4.

## M5.1.16. Создать preliminary determinism decisions

Для каждого persistent candidate:

```text
deterministic_relevance =
    YES
    NO
    DERIVED
    UNKNOWN
```

К выходу 5.1 `UNKNOWN` запрещён.

Если `YES`:

digest policy может быть preliminary:

```text
INCLUDE
DERIVED_AND_VERIFIED
DEFER_TO_5_5
```

## M5.1.17. Реализовать `goal5_persistence_contract.py`

Validator проверяет:

1. Every discovered production module represented.
2. Every candidate represented exactly once.
3. No unknown candidate ID.
4. No duplicate candidate ID.
5. No unknown role.
6. No unknown save plane.
7. Persistent owner has authoritative owner.
8. Checkpoint has owner.
9. `FRAMEWORK_SAVEGAME` state has participant mapping or explicit 5.2 gap.
10. `RUNTIME_PERSISTENCE` state maps to runtime persistence contract.
11. `RECONSTRUCTED_TRANSIENT` has rebuild rationale.
12. `READ_ONLY_SNAPSHOT` has reason why it is not durable.
13. Dangerous field flags have reviewed decision.
14. Deterministic-relevant state has preliminary digest policy.
15. Dependencies reference known candidate/module/participant IDs.
16. No `TODO`, `UNKNOWN`, empty rationale on reviewed N/A-equivalent decisions.
17. Evidence target exists in canonical CTest manifest if evidence is executable.

## M5.1.18. Self-test persistence contract validator

Negative fixtures:

1. Missing candidate.
2. Duplicate owner.
3. Persistent owner without save plane.
4. Runtime handle marked durable without rationale.
5. Missing dependency.
6. Unknown module.
7. Auto-generated generic evidence.
8. Persistent checkpoint without owner.
9. Deterministic state without digest decision.
10. Unknown classification.
11. Stale test target.

## M5.1.19. Сгенерировать gap report

Generator:

```text
docs/freeze/goal5_persistence_gaps.json
```

Gap categories:

```text
MISSING_PARTICIPANT
MISSING_SCHEMA_POLICY
MISSING_DURABLE_STORAGE
MISSING_DEPENDENCY_DECISION
MISSING_RECONCILIATION
MISSING_TEST_EVIDENCE
MISSING_DETERMINISM_POLICY
SUSPECT_TRANSIENT_IDENTITY
CONFIRMED_DEFECT
```

Gap report генерируется из reviewed contract.

Его нельзя редактировать вручную.

## M5.1.20. Закрыть 5.1 gate

Запустить:

```powershell
python docs/freeze/goal5_persistence_inventory.py --check
python docs/freeze/goal5_persistence_inventory.py --self-test
python docs/freeze/goal5_persistence_contract.py --check
python docs/freeze/goal5_persistence_contract.py --self-test
.\build.ps1 -Mode Validators
```

Exit:

```text
100% discovered production modules reviewed
100% persistence candidates reviewed
UNCLASSIFIED = 0
UNKNOWN = 0
candidate inventory stable
gap report generated
no production behavior changed without regression
```

Сделать checkpoint.

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

# 8. Stage 5.6: Determinism audit and scenario matrix

# 5.6.A. Candidate scanner

## M5.6.1. Реализовать `goal5_determinism_inventory.py`

Scope only production code.

Find:

```text
unordered_map
unordered_set
random_device
steady_clock
system_clock
high_resolution_clock
pointer/address hashing
reinterpret_cast to integer around ordering/hash
thread id
async completion ordering
same timestamp schedules
floating comparisons influencing semantic branches
parallel reductions
serialization from unordered containers
```

Scanner output:

```text
docs/freeze/goal5_determinism_candidates.json
```

## M5.6.2. Reduce false positives structurally

Scanner records:

Module.
Function/symbol.
Pattern.
Context kind.
Whether path belongs deterministic relevant owner.

Do not silently drop findings by regex exception without reviewed rule.

## M5.6.3. Self-test scanner

Positive fixtures for each class.

Negative fixtures:

Tests directory.
Diagnostics-only code.
Comments/string literals.
Generated files.

---

# 5.6.B. Reviewed determinism registry

## M5.6.4. Classify each candidate

Allowed decisions:

```text
DETERMINISTIC_BY_CONSTRUCTION
CANONICALIZED
EXPLICIT_INPUT
BUDGET_NON_SEMANTIC
TOLERANCE_DEFINED
EXCLUDED_NON_SEMANTIC
DEFECT
```

Each reviewed record:

```text
candidate_id
decision
rationale
contract
evidence
review_status
```

## M5.6.5. Review unordered containers

For every candidate that can influence observable order:

Prove sort/canonicalization.
Or prove order unobservable.
Or mark defect.

Add regression if defect.

## M5.6.6. Review randomness

Search must end with:

All semantic random streams explicit.
No ambient random source in semantic domain.
Persistent streams restored.
Independent streams isolated.

Any `random_device` in semantic path is blocker unless explicit nonsemantic rationale.

## M5.6.7. Review wall-clock budgets

Specifically inspect current Runtime:

Resources.
Navigation.
Streaming.

For each:

Determine whether elapsed real time changes:

Only amount completed this frame.
Or committed semantic result/order.

Choose:

`BUDGET_NON_SEMANTIC`.
`EXPLICIT_INPUT`.
Deterministic work-count mode.
Defect.

Add deterministic profile/seam if needed.

## M5.6.8. Review same-time ordering

Time scheduler.
Simulation tasks.
Event queues.
Integration delivery queues.

Tie breaker must be explicit where order matters.

## M5.6.9. Review floating semantic branches

For each relevant path:

Fixed/integer already.
Canonical quantization.
Tolerance.
Excluded nonsemantic.
Defect.

Cross-toolchain bit identity is not automatically required.

## M5.6.10. Implement `goal5_determinism_contract.py`

Check:

1. Candidate inventory exact.
2. Every candidate reviewed.
3. No unknown decision.
4. Evidence target valid.
5. Deterministic owner digest covered.
6. Wall-clock candidate has policy.
7. Random candidate has stream policy.
8. Observable unordered source canonicalized.
9. Floating relevant path has policy.

Self-tests required.

---

# 5.6.C. Scenario matrix

## M5.6.11. Scenario A: control continuation

Build nontrivial semantic state.

Run fixed ordered inputs.

Run 10 times same executable.

Digest/trace identical.

## M5.6.12. Scenario B: save/load equivalence

Control:

Run without reload.

Variant:

At fixed step:
save,
destroy,
fresh composition,
restore,
reconcile,
continue.

Compare final semantic digest.

Compare normalized semantic event trace.

Expected equivalent according to contract.

## M5.6.13. Scenario C: streaming interruption

Run:

Materialize region.
Stream out.
Advance work.
Stream in.
Continue.

Repeat with same inputs.

Check stable semantic result and correct transient rebuild.

## M5.6.14. Scenario D: deferred/budget exhaustion

Force small deterministic work budget.

Same total work split across several ticks.

Compare against larger-budget equivalent if semantics promise partition invariance.

If only eventual equivalence promised:

compare at common completion point.

## M5.6.15. Scenario E: combat/effects

Include:

Ability/resource reservation.
Combat commit.
Effect/condition.
Terminal outcome.
Loot/reward.
Progression.

Verify exactly-once semantic trace.

## M5.6.16. Scenario F: perception/knowledge/AI

Include:

Observation.
Knowledge update.
Target disappearance/change.
AI replan.
Intent commit.

Stable trace/order.

## M5.6.17. Scenario G: economy/process/resources

Include:

Reservation.
Process.
Resource production.
Commit/retry.

No duplicate economic/resource side effect.

## M5.6.18. Scenario H: runtime materialization

Include:

Semantic object.
RuntimeBridge materialization.
Save/reload.
New runtime handle.
Stale old observation.
Retry materialization.

Semantic digest must not depend on old runtime identity.

## M5.6.19. Add save-load determinism target

Create:

```text
EpidemicDeterminismSaveLoadTruthTests
```

Labels:

```text
truth
determinism
determinism-save-load
persistence-whole-engine
```

It may reuse runner library.

## M5.6.20. D1 qualification

Each mandatory scenario:

10 same-build repeated runs.

Use:

```powershell
ctest --test-dir build/ae2/full-debug -C Debug -L determinism --repeat until-fail:10 --output-on-failure
```

Where CTest semantics fit.

Otherwise scenario runner performs controlled loop internally and prints failing seed/run.

## M5.6.21. D2 qualification

Run required core scenarios as fresh processes multiple times.

Compare emitted result JSON.

Minimum:

Control.
Save/load.
Runtime materialization.

## M5.6.22. Optional D3 detector

Run Debug and Release.

Compare semantic result.

If mismatch:

Investigate.

D3 becomes formal blocker only if compatibility contract says so.

At minimum unexplained mismatch cannot be silently ignored.

## M5.6.23. Cross-compiler detector

ClangCL can run selected headless deterministic scenario if practical.

By default this is detector, not frozen D4 promise.

Any difference is investigated for UB/nondeterminism.

## M5.6.24. Close 5.6

Run:

```powershell
python docs/freeze/goal5_determinism_inventory.py --check
python docs/freeze/goal5_determinism_inventory.py --self-test
python docs/freeze/goal5_determinism_contract.py --check
python docs/freeze/goal5_determinism_contract.py --self-test
python docs/freeze/goal5_evidence_quality.py --check
python docs/freeze/goal5_evidence_quality.py --self-test
```

Run all scenario labels.

Exit:

```text
determinism candidates reviewed = 100%
unknown = 0
D1 PASS
D2 PASS
save/load equivalent
required scenario matrix PASS
all confirmed defects have regressions
```

Checkpoint.

---

# 9. Stage 5.7: Final Milestone 5 qualification

# 5.7.A. Build final ledger

## M5.7.1. Создать `goal5_ledger.json`

Generated only.

For every discovered production module:

```text
module
persistence_applicability
persistence_status
determinism_applicability
determinism_status
candidate_ids
participant_ids
scenario_evidence
defect_ids
```

Allowed result:

```text
PASS
N/A
BLOCKED
```

`N/A` requires reviewed rationale in source contract.

## M5.7.2. Создать `goal5_contract.py`

Aggregator checks all prior Goal 5 contracts.

It must not duplicate deep logic.

It verifies:

1. Every lower validator PASS.
2. No BLOCKED.
3. No unclassified candidate.
4. No gap report entries left except explicitly allowed future-goal items.
5. Whole-engine tests registered.
6. Required CTest labels exist.
7. Required evidence targets exist.
8. Ledger generated from contracts.
9. Ledger idempotent.
10. No roadmap Markdown dependency.

## M5.7.3. Add Goal 5 adversarial self-test

One self-test entry point should include broken fixtures for:

Missing persistent owner.
Orphan participant.
Migration gap.
Mismatched save generation.
Restore dependency cycle.
Transient runtime identity persisted.
Missing digest contributor.
Unreviewed `steady_clock`.
Unordered observable output.
Generic evidence.
Missing test target.
Fake PASS in ledger.

---

# 5.7.B. Integrate with current freeze-contract

## M5.7.4. Update `ci_gate_contract.py`

Semantic CI validator must require Goal 5 checks once Goal 5 is ready for enforcement.

Expected workflow commands:

```powershell
python docs/freeze/goal5_admission.py --check
python docs/freeze/goal5_persistence_inventory.py --check
python docs/freeze/goal5_persistence_contract.py --check
python docs/freeze/goal5_participant_contract.py --check
python docs/freeze/goal5_schema_contract.py --check
python docs/freeze/goal5_save_generation_contract.py --check
python docs/freeze/goal5_restore_contract.py --check
python docs/freeze/goal5_determinism_inventory.py --check
python docs/freeze/goal5_determinism_contract.py --check
python docs/freeze/goal5_evidence_quality.py --check
python docs/freeze/goal5_contract.py --check
```

Self-tests also required.

## M5.7.5. Update GitHub workflow

Add Goal 5 validators to existing `freeze-contract` job.

Do not create unrelated workflow unless timeout/ownership justifies later split.

System tests remain in Full profiles through CTest.

## M5.7.6. Update `build.ps1` validator list

Goal 5 validators and self-tests become part of:

```powershell
.\build.ps1 -Mode Validators
```

No hidden manual command required for local admission.

---

# 5.7.C. CTest taxonomy

## M5.7.7. Verify labels

Required labels after Goal 5:

```text
persistence-local
persistence-wire
persistence-storage
persistence-whole-engine
determinism
determinism-save-load
goal5-regression
```

Not every old test needs a new label.

New Goal 5 suites must have explicit labels.

## M5.7.8. Update `ctest_manifest`

Record new tests only after names/labels/timeouts stable.

Use normal manifest generation procedure.

Then:

```powershell
python docs/freeze/ctest_manifest.py --check
python docs/freeze/ctest_manifest.py --self-test
```

No hand editing canonical CTest JSON.

---

# 5.7.D. Final exact-tree qualification

## M5.7.9. Clean validators run

```powershell
.\build.ps1 -Mode Validators -Clean
```

Expected:

All old freeze validators green.
All Goal 5 validators green.
All self-tests green.

## M5.7.10. Clean full Windows matrix

```powershell
.\build.ps1 -Mode Matrix -Configuration Both -Clean
```

Expected:

Base Debug PASS.
Base Release PASS.
Runtime Debug PASS.
Runtime Release PASS.
Full Debug PASS.
Full Release PASS.
Public header self-containment PASS.
Exact CTest manifests PASS.
Warnings as errors PASS.

## M5.7.11. Goal 5 focused Debug rerun

```powershell
ctest --test-dir build/ae2/full-debug -C Debug -L persistence-wire --output-on-failure
ctest --test-dir build/ae2/full-debug -C Debug -L persistence-storage --output-on-failure
ctest --test-dir build/ae2/full-debug -C Debug -L persistence-whole-engine --output-on-failure
ctest --test-dir build/ae2/full-debug -C Debug -L determinism --output-on-failure
ctest --test-dir build/ae2/full-debug -C Debug -L determinism-save-load --output-on-failure
```

## M5.7.12. Goal 5 focused Release rerun

Equivalent commands against `full-release`.

## M5.7.13. Repetition

Determinism D1:

At least 10 runs required scenarios.

Whole restore:

At least 3 consecutive runs.

Storage process-boundary smoke:

At least 3 consecutive fresh-process runs.

This is Milestone 5 qualification, not full Goal 9 flakiness campaign.

## M5.7.14. ClangCL portability check

Existing required public-header ClangCL remains.

Additionally, if deterministic headless target builds with ClangCL without platform expansion, run it as detector.

A mismatch must be investigated for UB or hidden nondeterminism.

It is not D4 blocker unless D4 explicitly adopted.

## M5.7.15. Verify generators idempotent

Run every Goal 5 generator once.

Capture hashes только generated Goal 5 canonical artifacts, например:

```powershell
$before = Get-ChildItem docs/freeze/goal5_*.json -File |
    Sort-Object FullName |
    ForEach-Object { "$($_.FullName)|$((Get-FileHash $_.FullName -Algorithm SHA256).Hash)" }
```

Run every Goal 5 generator второй раз без изменения source/contracts.

Capture `$after` тем же способом и сравнить:

```powershell
Compare-Object $before $after
```

Expected output: empty.

Дополнительно убедиться, что второй generation run сам сообщает zero rebases/repairs, если generator ведёт такие counters.

Нельзя использовать общий `git diff --exit-code` как единственную проверку idempotence, потому что в рабочем tree могут находиться legitimate production changes текущего milestone.

No hidden rebasing/repair.

## M5.7.16. Verify no note dependency

Search executable CI scopes again.

No Milestone/roadmap Markdown as source of truth.

## M5.7.17. Verify no remaining Goal 5 gaps

Check generated gap report:

```text
MISSING_PARTICIPANT = 0
MISSING_SCHEMA_POLICY = 0
MISSING_DURABLE_STORAGE = 0
MISSING_DEPENDENCY_DECISION = 0
MISSING_RECONCILIATION = 0
MISSING_TEST_EVIDENCE = 0
MISSING_DETERMINISM_POLICY = 0
SUSPECT_TRANSIENT_IDENTITY = 0
CONFIRMED_DEFECT unresolved = 0
```

## M5.7.18. Verify defect registry

Every Goal 5 confirmed defect:

Stable ID.
REVIEWED.
Reproducer.
Permanent regression.
Valid CTest target.
Valid test symbol.
No unresolved P0/P1.

## M5.7.19. Generate final Goal 5 ledger

Run generator.

Expected:

```text
100% discovered production modules have persistence decision
100% discovered production modules have determinism decision
BLOCKED = 0
UNCLASSIFIED = 0
```

## M5.7.20. Final gate

```powershell
python docs/freeze/goal5_contract.py --check
python docs/freeze/goal5_contract.py --self-test
```

Only if PASS:

Milestone 5 can be marked complete.

---

# 10. Expected production changes by stage

This section prevents scope drift.

## 10.1. 5.1

Expected production C++ changes:

Normally zero.

Allowed only if scanner exposes confirmed existing defect.

Main changes:

Tooling.
JSON contracts.
Validators.
Self-tests.

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

## 10.4. 5.4

Expected production changes:

Restore coordinator/reconciliation state if missing.
RuntimeBridge reconciliation hooks only if existing contract insufficient.

High risk:

Postcommit rollback illusion.
Runtime identity reuse.
Simulation resume too early.

## 10.5. 5.5

Expected production changes:

Mostly test/qualification infrastructure.

Avoid adding debug-only public methods solely for digest.

If introspection insufficient:

prefer internal friend/test adapters or existing snapshot/query APIs.

## 10.6. 5.6

Expected production changes:

Only confirmed determinism defects.

Examples:

Explicit sort.
Tie breaker.
Explicit random stream.
Deterministic budget mode.
Canonical quantization.

## 10.7. 5.7

Expected production changes:

Zero.

Only wiring validators/CI/manifests and fixing qualification regressions.

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

# 15. Final Milestone 5 no-debt checklist

Milestone 5 cannot close until every item is true.

## Persistence inventory

[ ] Production discovery complete.
[ ] Every production module persistence applicability reviewed.
[ ] Every snapshot/checkpoint candidate classified.
[ ] No unknown persistence candidate.
[ ] No duplicate authoritative owner.
[ ] Runtime transient identity not durable.

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

## Save generation

[ ] One logical save point.
[ ] Framework SaveGame included.
[ ] Runtime Persistence included.
[ ] Time included.
[ ] Pending accepted work has save policy.
[ ] Failed save never publishes valid-looking incomplete package.

## Restore

[ ] Fresh composition restore.
[ ] Production dependency order.
[ ] Derived indexes valid.
[ ] RuntimeBridge rebuilt.
[ ] Old transient handles rejected.
[ ] Precommit failures atomic.
[ ] Postcommit reconciliation retry-safe.
[ ] Simulation resumes only when ready.

## Deterministic runner

[ ] Deterministic state coverage complete.
[ ] Canonical digest.
[ ] Semantic event trace.
[ ] Reproduction metadata.
[ ] Fresh-process support.

## Determinism audit

[ ] All candidates reviewed.
[ ] Unordered semantic output canonicalized.
[ ] Randomness explicit.
[ ] Wall-clock budgets reviewed.
[ ] Same-time ordering explicit.
[ ] Floating semantic policy explicit.

## Required scenarios

[ ] Control continuation.
[ ] Save/load equivalence.
[ ] Streaming.
[ ] Deferred/budget.
[ ] Combat/effects.
[ ] Perception/knowledge/AI.
[ ] Economy/process/resources.
[ ] Runtime materialization.

## Qualification

[ ] D1 PASS.
[ ] D2 PASS.
[ ] Goal 5 validators PASS.
[ ] Goal 5 self-tests PASS.
[ ] Full Windows Debug/Release matrix PASS.
[ ] Public-header self-containment PASS.
[ ] Exact CTest manifests PASS.
[ ] Goal 5 generators idempotent.
[ ] Gap report empty.
[ ] Unresolved Goal 5 P0/P1 = 0.
[ ] Unresolved persistence/determinism code debt = 0.

---

# 16. Milestone 5 completion state

Only after M5.7.20 passes record:

```text
PERSISTENCE_QUALIFIED
DETERMINISM_QUALIFIED
GOAL5_COMPLETE
```

Do not assign:

```text
SYSTEM_READY
FREEZE_READY
FROZEN
```

Those statuses require later goals.

After `GOAL5_COMPLETE`, next allowed campaign step is Goal 6:

memory/lifetime and concurrency/async qualification.

---

# 17. Final execution order, condensed

This is the literal serial order.

```text
M5.0   Admission
  |
  v
M5.1   Discover persistence surface
  |
  v
M5.1   Review/classify all candidates
  |
  v
M5.1   Validate persistence map
  |
  v
M5.2   Build participant coverage graph
  |
  v
M5.2   Implement missing participants
  |
  v
M5.2   Freeze schema/migration policy
  |
  v
M5.2   Implement whole-image codec
  |
  v
M5.2   Implement durable storage boundary
  |
  v
M5.2   Qualify wire/storage/reopen
  |
  v
M5.3   Classify save-sensitive pending work
  |
  v
M5.3   Define consistent-cut mechanism
  |
  v
M5.3   Implement cross-plane save coordination
  |
  v
M5.3   Fault-test atomic save generation
  |
  v
M5.4   Freeze restore phase/dependency contract
  |
  v
M5.4   Build whole-engine restore harness
  |
  v
M5.4   Qualify RuntimeBridge rematerialization
  |
  v
M5.4   Qualify reconciliation failures/retry
  |
  v
M5.5   Define semantic deterministic domain
  |
  v
M5.5   Build canonical digest and event trace
  |
  v
M5.5   Build deterministic scenario runner
  |
  v
M5.5   Prove runner with adversarial self-tests
  |
  v
M5.6   Discover nondeterminism candidates
  |
  v
M5.6   Review every candidate
  |
  v
M5.6   Fix confirmed determinism defects
  |
  v
M5.6   Run scenarios A-H
  |
  v
M5.6   D1/D2 repetition
  |
  v
M5.7   Generate Goal 5 ledger
  |
  v
M5.7   Integrate validators into freeze-contract
  |
  v
M5.7   Update CTest manifest
  |
  v
M5.7   Clean Windows matrix
  |
  v
M5.7   Focused persistence/determinism reruns
  |
  v
M5.7   Idempotence/no-gap/no-debt checks
  |
  v
GOAL5_COMPLETE
```

No phase is parallel.

No later phase starts on a red earlier gate.

No final PASS is based on a roadmap Markdown file.

No production fix is accepted without executable regression.
