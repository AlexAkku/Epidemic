# Part 1/7 — Admission + 5.1 Persistence ownership map

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

### P0-A — Admission tree integrity
Requirements: `M5.0.1–M5.0.2`.

Цель: exact working baseline и note-independent freeze pipeline. Packet не меняет Goal 5 behavior.

### P0-B — Admission qualification
Requirements: `M5.0.3–M5.0.6`.

Цель: действующие validators/matrix/admission record/self-test. Выходом является machine-readable admission baseline, а не terminal transcript.

### P1-A — Persistence discovery surface
Requirements: `M5.1.1–M5.1.8`.

Цель: deterministic finite candidate inventory. Discovery фиксирует факты/риски, но не semantic PASS.

### P1-B — Persistence semantic classification
Requirements: `M5.1.9–M5.1.16`.

Цель: reviewed module/candidate lifecycle decisions для всей finite surface. Review выполняется ownership-local slices; `UNCLASSIFIED`/`UNKNOWN` запрещены на выходе.

### P1-C — Persistence contract/gaps/gate
Requirements: `M5.1.17–M5.1.20`.

Цель: fail-closed validator+self-test, generated gap set и закрытый 5.1 gate.

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

---

# Stage-specific scope guard

## 10.1. 5.1

Expected production C++ changes:

Normally zero.

Allowed only if scanner exposes confirmed existing defect.

Main changes:

Tooling.
JSON contracts.
Validators.
Self-tests.

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

## Persistence inventory

[ ] Production discovery complete.
[ ] Every production module persistence applicability reviewed.
[ ] Every snapshot/checkpoint candidate classified.
[ ] No unknown persistence candidate.
[ ] No duplicate authoritative owner.
[ ] Runtime transient identity not durable.

