# Milestone 5 v4 — full-fidelity pipeline edition

Дата pipeline-ревизии: 2026-10-05

Этот документ сохраняет полный архитектурный контракт Milestone 5 из версии 2026-10-01. Ни одно semantic requirement, no-debt condition, failure class, persistence/determinism obligation или exit criterion не удалено. Pipeline-ревизия меняет только способ организации работы вокруг требований.

## P0. Как использовать этот документ в локальном analysis pipeline

### P0.1. Source of truth

Ниже исходный архитектурный план остаётся authoritative specification Milestone 5. Machine-readable contracts и executable evidence остаются authoritative для фактического PASS состояния дерева. Pipeline rules не могут ослаблять требования плана.

### P0.2. Bounded work packets

Исполнение не формулируется как broad instruction «проверь весь subsystem». Любая широкая задача сначала сводится к finite inventory или canonical record set. Затем semantic review, implementation и verification выполняются bounded packets по стабильным IDs.

Каждый packet имеет пять обязательных свойств:

1. `Objective` — одно утверждение, которое станет PASS/BLOCKED.
2. `Finite surface` — конечный набор module/candidate/participant/scenario/defect IDs.
3. `Allowed change scope` — минимальная ownership-local зона.
4. `Machine proof` — validator/test/evidence, которые доказывают утверждение.
5. `Handoff state` — короткий набор фактов, достаточный следующему packet.

### P0.3. Separation of semantic and mechanical work

Semantic decisions выполняются последовательно одним reasoning stream. Независимые build/test/static/validator процессы могут выполняться параллельно как local OS processes, если они не меняют общие artifacts и проверяют уже определённый change-set.

Mechanical PASS не является новой reasoning задачей. Mechanical FAIL открывает ровно одну следующую hypothesis по первой существенной причине.

### P0.4. Context discipline

Pipeline должен переносить в model-visible context не repository surface, а evidence package. По умолчанию достаточно нескольких точных source ranges/contract records. Broad inventory остаётся machine-readable и не должен целиком пересказываться в reasoning context.

### P0.5. No semantic compression

Оптимизация токенов не разрешает:

- объединять разные requirements в один generic PASS;
- заменять candidate-by-candidate review выборкой примеров;
- удалять negative fixtures или failure matrix cases;
- заменять executable behavior наличием symbol/manifest записи;
- пропускать historical/versioning/storage/process-boundary obligations;
- ослаблять final exact-tree qualification.

### P0.6. Resume and compaction

После каждого packet сохраняется короткое structured state: текущий requirement range, completed packet IDs, working files, confirmed facts, blockers и open questions. При resume уже закрытые packets не исследуются заново, если их inputs не изменились.

---

# Полный архитектурный план (semantic requirements preserved verbatim)

# Milestone 5: системная квалификация persistence и determinism

Дата планирования: 2026-10-01

Статус документа: план доработки и квалификации. Это не пошаговый execution plan.

Основа: текущее состояние движка после локального закрытия EngineBase, EngineRuntime и EngineFramework и пересмотр исходной цели 5 из общего work-plan.

## 1. Назначение Milestone 5

Milestone 5 должен доказать две системные гарантии движка.

Первая гарантия: состояние, которое по контракту обязано переживать сохранение и полную перезагрузку движка, действительно сохраняется, восстанавливается в правильном порядке, проходит versioning и migration policy, не смешивается с transient runtime state и после восстановления позволяет безопасно продолжить тот же causal scenario.

Вторая гарантия: deterministic semantic domain движка при одинаковом исходном состоянии и одинаковом упорядоченном наборе входов приходит к одному и тому же семантическому результату. Save/load, streaming, budgeted work и восстановление runtime representation не должны незаметно менять дальнейшее поведение.

Milestone 5 не должен повторять локальный аудит 78 production-модулей. Локальная correctness, failure atomicity и большая часть module-local snapshot/restore evidence уже закреплены предыдущими milestones. Новый этап работает на системном уровне и объединяет существующие локальные контракты в один проверяемый whole-engine contract.

Цель Milestone 5 формулируется так:

> Доказать, что весь authoritative persistent state движка полностью классифицирован, сохраняется через единый versioned save contract, восстанавливается в fresh engine composition без reuse transient identity и после restore даёт deterministic semantic continuation, эквивалентный control run в пределах явно определённого deterministic contract.

## 2. Что меняется относительно старой Goal 5

Исходная цель 5 правильно определяла основные направления: persistence map, local restore checks, whole-engine restore, deterministic runner, determinism matrix и repeated deterministic scenarios.

После Goals 3 и 4 часть этих требований уже закрыта локально. Поэтому Milestone 5 не должен заново заставлять каждый модуль проходить одинаковый набор roundtrip и corruption tests. Вместо этого требуется gap analysis относительно уже существующего evidence.

Дополнительно текущая архитектура требует трёх вещей, которые старый план описывал недостаточно точно.

Первая: необходимо различать authoritative persistent state, persistent checkpoints, reconstructed transient state и read-only snapshots. Наличие `CaptureSnapshot()` само по себе не делает объект участником save game.

Вторая: Framework SaveGame и Runtime Persistence являются двумя persistence planes. Milestone 5 обязан доказать, что они относятся к одному logical save point и не могут сформировать системно невозможную комбинацию двух по отдельности валидных snapshot.

Третья: in-memory snapshot и in-memory `SaveGameImage` недостаточны для системной квалификации. Goal 5 должен включать versioned wire representation, durable storage boundary и fresh-process restore model.

## 3. Принципы выполнения

### 3.1. Только последовательная работа

Milestone 5 выполняется последовательно. Следующий раздел не считается начатым, пока предыдущий не сформировал стабильный machine-readable contract и не прошёл свои validators.

Причина: все последующие этапы используют результаты предыдущих как source of truth. Parallel delta model, применявшаяся в Goal 4, здесь создаст риск несовместимых представлений persistence ownership, save ordering и deterministic state.

### 3.2. Machine-readable contract является источником истины

Markdown используется для объяснения архитектуры и результатов. CI не должен зависеть от рабочих заметок, roadmap или milestone documents.

Каждый системный контракт Milestone 5 должен иметь machine-readable canonical representation. Валидаторы проверяют именно его.

Документация может генерироваться из canonical contract или ссылаться на него. Удаление рабочего Markdown не должно менять результат CI.

### 3.3. Автоматическая проверка строится по схеме discovery -> reviewed contract -> validator -> executable evidence

Для каждого крупного класса требований применяются четыре слоя.

1. Discovery tool автоматически находит потенциальную поверхность в коде.
2. Reviewed registry классифицирует каждый найденный объект.
3. Validator доказывает полноту registry относительно discovery inventory и fail-closed отклоняет новые необработанные объекты.
4. Executable tests доказывают реальное поведение.

Автоматический scanner не имеет права сам объявлять semantic contract правильным. Он находит кандидатов. Решение о том, является ли состояние persistent, transient или checkpoint, должно быть зафиксировано reviewed decision.

### 3.4. Проверяется поведение, а не наличие символа

Нельзя считать persistence или determinism доказанными потому, что test source содержит имя API.

Evidence должен ссылаться минимум на:

* зарегистрированный test target;
* стабильный test case или test symbol;
* проверяемое contract assertion;
* machine-readable obligation, которую этот test закрывает.

Raw line number может храниться как дополнительная навигация. Он не должен быть единственной идентичностью evidence.

### 3.5. Валидаторы сами проходят adversarial self-tests

Каждый новый validator должен иметь negative fixtures минимум для следующих классов ошибки:

* пропущенный owner;
* неизвестная classification;
* duplicate identity;
* stale evidence;
* broken dependency;
* invalid version policy;
* transient identity, ошибочно помеченная persistent;
* persistent state без save participant;
* non-deterministic state без policy;
* попытка объявить PASS только через generic или setup evidence.

Положительный fixture обязателен отдельно.

### 3.6. Fix только после воспроизведения

Если Milestone 5 обнаруживает production defect, сначала создаётся минимальный regression или deterministic reproducer. После этого меняется production code.

Нельзя закрывать обнаруженный defect только изменением manifest или rationale.

### 3.7. Public contract меняется только при необходимости

Persistence qualification не является поводом для общего redesign API.

Если обнаруженный defect требует изменения public contract, affected local evidence предыдущих milestones пересматривается. Изменение должно быть минимальным и явно привязанным к defect.

### 3.8. No-debt означает отсутствие неизвестного состояния

Milestone 5 не считается закрытым, если остаётся хотя бы один из следующих классов неопределённости:

* snapshot-like type без classification;
* authoritative state без save policy;
* persistent checkpoint без owner;
* save participant без version policy;
* transient runtime identity, попавшая в durable save;
* deterministic state без digest policy;
* source of randomness без stream ownership;
* observable ordering без tie-breaking contract;
* restore dependency, существующая только в test harness;
* post-restore reconciliation без retry semantics;
* сохранение, которое можно проверить только вручную.

---

# 4. Текущие подтверждённые исходные факты

Перед началом исполнения план опирается на следующие факты текущего дерева, проверенные повторно на baseline 2026-10-01:

* canonical local-ready ledger содержит 49 modules, у которых `PERSIST-SNAPSHOT`, `PERSIST-VALIDATE`, `PERSIST-FAILURE` и `PERSIST-CONTINUITY` одновременно `PASS`; ещё 29 modules имеют по этим четырём критериям reviewed `N/A`; смешанных/неопределённых состояний нет;
* среди этих 49 modules 47 относятся к EngineFramework и 2 к EngineRuntime (`Persistence` и `Time`);
* Framework production surface содержит многочисленные `RestoreSnapshot`/checkpoint contracts, поэтому Goal 5 является системной интеграцией уже существующего local evidence, а не повторным module audit;
* production `ISaveParticipant` implementations на текущем baseline две: `ScheduledTriggerDispatcherSaveParticipant` и `NarrativeExternalConsequenceSaveParticipant`;
* `ISaveBarrier` существует как public contract, но production implementation в текущем дереве не обнаружена; реализации существуют только в tests;
* Runtime `IPersistenceBackend` имеет production `InMemoryPersistenceBackend`; durable filesystem/external backend в текущем дереве не обнаружен;
* `SaveGameImage` уже содержит versioned manifest, participant schema versions, dependencies, required/optional semantics, payload hashes и resource limits, но production byte/storage representation целого image отдельно не обнаружена;
* production code использует `steady_clock` в Runtime Resources, Navigation и Streaming для work/time budgets, поэтому determinism qualification обязана отделить semantic result от machine-speed-dependent progress;
* текущий Truth save/restore smoke вручную восстанавливает ограниченный набор Framework services и не является production whole-engine SaveGame restore.

Эти факты являются admission baseline для Milestone 5. Их должен подтвердить первый discovery run; если дерево к началу исполнения изменилось, machine-readable discovery имеет приоритет над числами этого документа.

---

# 5.1. Persistence ownership map and save contract

## 5.1.1. Цель этапа

Сформировать полную и проверяемую карту состояния движка, которое связано с persistence.

Карта должна отвечать не на вопрос «у кого есть snapshot», а на вопрос «какой жизненный цикл имеет это состояние и какое место оно занимает в whole-engine save/restore».

Этап не меняет production behavior, если discovery не выявит фактическую дыру. Основной результат этапа: canonical contract и автоматический контроль его полноты.

## 5.1.2. Классификация состояния

Persistence role и save plane являются двумя независимыми осями. Их нельзя смешивать в один enum: Runtime-owned persistent state остаётся `PERSISTENT_OWNER`, а принадлежность Runtime persistence plane фиксируется отдельным полем.

Каждый найденный state-bearing candidate должен принадлежать ровно одному `persistence_role`.

| `persistence_role` | Смысл |
|---|---|
| `PERSISTENT_OWNER` | Authoritative state, которое обязано переживать save/load |
| `PERSISTENT_CHECKPOINT` | Durable adapter, outbox, reservation, cursor или reconciliation state, влияющее на корректное продолжение |
| `RECONSTRUCTED_TRANSIENT` | State, которое после restore создаётся заново из authoritative state |
| `READ_ONLY_SNAPSHOT` | Detached observation, query, diagnostics или test snapshot, который не является save contract |
| `NON_PERSISTENT_OWNER` | Authoritative state, которое по design не обязано переживать save/load |

Отдельно фиксируется `save_plane`:

| `save_plane` | Смысл |
|---|---|
| `FRAMEWORK_SAVEGAME` | State входит в Framework SaveGame participant graph |
| `RUNTIME_PERSISTENCE` | State принадлежит Runtime Persistence plane |
| `EXTERNAL` | Durable state находится за engine boundary и участвует через reconciliation contract |
| `NONE` | Для transient/read-only/non-persistent state durable plane отсутствует |

Классификация должна быть исчерпывающей и fail-closed. Новый snapshot-like candidate после изменения кода обязан сделать validator красным, пока не будет review decision.

## 5.1.3. Что должно храниться в canonical persistence map

Для каждого candidate требуется machine-readable запись минимум со следующими полями:

| Поле | Назначение |
|---|---|
| `module` | Production freeze unit |
| `state_id` | Стабильная identity записи |
| `classification` | Один из классов persistence map |
| `authoritative_owner` | Кто имеет право изменять state |
| `capture_contract` | Какая операция фиксирует durable state |
| `restore_contract` | Какая операция или participant восстанавливает state |
| `save_plane` | Framework SaveGame, Runtime Persistence, external либо N/A |
| `participant_id` | Stable SaveParticipant identity, если он уже существует; иначе explicit gap для 5.2 |
| `durable_schema` | Durable schema identity/version, если durable boundary уже существует; иначе explicit gap для 5.2 |
| `migration_policy` | Current compatibility decision либо explicit gap для 5.2 |
| `dependencies` | Известные restore dependencies или reviewed dependency gap |
| `transient_rebuild` | Что должно быть создано заново после restore |
| `reconciliation` | Как восстанавливается связь с external/runtime state |
| `deterministic_relevance` | Влияет ли state на будущее semantic behavior |
| `digest_policy` | Входит ли state в semantic digest и каким образом |
| `evidence` | Reviewed test/contract references |
| `status` | `DISCOVERED`, `REVIEWED`, `BLOCKED` |

## 5.1.4. Автоматический discovery

Discovery должен собирать кандидатов из нескольких независимых источников.

Необходимо находить:

* `CaptureSnapshot`, `RestoreSnapshot`, `Checkpoint`, `SaveParticipant`;
* snapshot/checkpoint/public state types;
* Runtime Persistence records и backend interfaces;
* durable cursors, journals, reservations, outboxes и reconciliation queues;
* generators, revisions и random stream snapshots;
* RuntimeBridge mappings и другие identity mappings;
* serialization/archive entry points;
* module dossier persistence classification.

Scanner должен объединять результаты по stable symbol identity и выдавать candidate inventory.

Отдельно требуется detector потенциально опасных полей внутри durable structures:

* pointer;
* raw runtime handle;
* allocator identity;
* thread ID;
* wall-clock time;
* process-local callback/token;
* backend-specific opaque identity без documented remapping.

Нахождение такого поля не означает defect автоматически. Оно требует reviewed decision.

## 5.1.5. Что переиспользуется из Goals 3 и 4

Предыдущий local persistence evidence не выбрасывается.

Для каждого `PERSISTENT_OWNER` и `PERSISTENT_CHECKPOINT` canonical map должна уметь сослаться на уже существующие доказательства:

* roundtrip;
* invalid restore;
* revision/generator continuity;
* journal/cursor continuity;
* failure atomicity;
* stale identity handling;
* accepted external work;
* reconciliation.

Новый тест добавляется только если конкретная applicable obligation не закрыта существующим evidence.

Таким образом, 5.1 формирует gap matrix, а не повторный полный module audit.

## 5.1.6. Проверки корректности

Validator этапа должен доказать:

1. 100% автоматически обнаруженных production-модулей имеют module-level persistence applicability decision. На текущем baseline их 78; validator не должен hard-code это число.
2. Все автоматически найденные state candidates представлены в registry.
3. Все persistent records имеют authoritative owner.
4. Нет двух authoritative owners одного state.
5. Каждый persistent state имеет reviewed save-plane decision либо explicit unresolved durable-path gap, который обязан закрыть 5.2.
6. Каждый transient state имеет rebuild rationale.
7. Каждый deterministic-relevant state имеет preliminary digest decision либо explicit gap для 5.5.
8. Persistent state не содержит undocumented reusable runtime identity.
9. Известные restore dependencies представлены в registry; неизвестные зависимости не могут silently считаться отсутствующими.
10. Нет `UNCLASSIFIED`, `TODO`, `UNKNOWN` и auto-PASS решений.

## 5.1.7. Зона ответственности этапа

Разрешённая зона изменений:

* persistence discovery tooling;
* canonical persistence registry;
* validators и self-tests;
* persistence-related dossier metadata;
* минимальные regression tests, если discovery выявляет ранее неизвестный реальный defect.

Production code меняется только после подтверждённого defect.

## 5.1.8. Критерий выхода 5.1

Этап закрывается, когда:

* 100% discovered production modules классифицированы по persistence applicability;
* discovery inventory и reviewed registry совпадают;
* `UNCLASSIFIED = 0`;
* все persistent owners и checkpoints имеют owner и reviewed classification, а отсутствующие durable path/schema/dependency решения представлены как explicit gaps для следующих фаз;
* все snapshot-like types либо входят в persistence contract, либо явно классифицированы как transient/read-only/non-persistent;
* validator и adversarial self-tests зелёные;
* сформирован точный machine-readable gap list для 5.2 и preliminary determinism gaps для 5.5.

---

# 5.2. Save participant, wire format and durable storage integration

## 5.2.1. Цель этапа

Доказать, что каждый state, классифицированный в 5.1 как durable, имеет реальный путь от authoritative memory state до versioned durable representation и обратно.

Главный принцип: `CaptureSnapshot()` не является достаточным persistence contract.

Whole-engine save должен иметь законченный путь:

`authoritative state -> participant payload -> versioned save package -> durable storage -> fresh process -> validation -> restore`.

## 5.2.2. Save participant coverage

Каждый `PERSISTENT_OWNER` и `PERSISTENT_CHECKPOINT` должен быть покрыт одним из разрешённых механизмов:

1. Framework `ISaveParticipant`.
2. Runtime Persistence plane.
3. Явно документированный external durable owner с reconciliation contract.

Других вариантов быть не должно.

Validator должен fail-closed обнаруживать persistent state без durable path.

Для Framework participant graph требуется проверить:

* stable participant ID;
* unique participant identity;
* dependency list;
* current schema version;
* required/optional semantics;
* capture;
* validation;
* staging;
* commit;
* migration;
* payload limits;
* deterministic participant order.

## 5.2.3. Versioning model

Нельзя искусственно добавлять version field в каждую внутреннюю `FooSnapshot`, если snapshot является implementation detail.

Versioning contract должен существовать на durable boundary.

Основная единица compatibility:

`SaveParticipantId + SaveSchemaVersion + encoded payload`.

Для каждого participant определяется:

* current version;
* минимальная поддерживаемая historical version;
* migration chain;
* unsupported future version behavior;
* unsupported obsolete version behavior;
* missing optional field policy;
* unknown field/section policy, если формат это допускает.

Migration должна тестироваться на реальных historical fixtures либо на стабильно сохранённых compatibility fixtures только для реально поддерживаемых старых версий. Если participant существует только в schema version 1 и исторической поддерживаемой версии ещё нет, допускается reviewed `NO_PRIOR_SUPPORTED_VERSION`; создавать искусственную миграцию ради чекбокса запрещено.

## 5.2.4. Wire representation

Milestone 5 должен отделить semantic state от внутреннего layout C++ types.

Durable format не должен зависеть от:

* pointer layout;
* object padding;
* STL implementation layout;
* native vtable;
* process-local handle;
* raw memory dump.

Wire representation должна иметь:

* format identity;
* format version;
* bounded sizes;
* section/participant identity;
* schema version per participant;
* deterministic ordering там, где порядок семантически незначим;
* corruption detection policy;
* explicit integer/enum encoding rules.

Не требуется обещать вечную binary compatibility. Требуется versioned, проверяемая и мигрируемая persistence compatibility в пределах frozen contract.

## 5.2.5. Restore compatibility profiles

Текущий SaveGame contract уже различает `ExactRequired`, `CompatibleVersion`, `MigrationRequired`, `BestEffort` и `DeveloperMode`.

Milestone 5 должен явно определить, какие профили являются freeze-qualified production restore contracts.

`DeveloperMode` и permissive `BestEffort` не могут использоваться как доказательство строгой production restore correctness. Они могут иметь отдельные tests как development/recovery behavior.

Для каждого production-qualified profile требуется отдельная матрица допустимых metadata/schema mismatches, migration behavior и required/optional participant semantics.

## 5.2.6. Durable storage boundary

Необходимо определить production storage interface, который способен записать и снова открыть save package.

Этап должен квалифицировать минимум следующие свойства:

* complete write;
* failed write не уничтожает последний valid save;
* truncated/corrupted payload обнаруживается;
* write interruption имеет определённый outcome;
* reopen через новый engine process model;
* size limits;
* storage error propagation;
* retry policy;
* overwrite/replacement semantics.

Конкретный filesystem implementation может оставаться платформенным. Persistence contract должен быть testable через deterministic backend и отдельный platform smoke.

## 5.2.7. Автоматические инструменты

Нужен participant coverage validator, который сопоставляет persistence map с зарегистрированным participant graph и Runtime persistence plane.

Нужен schema validator, который проверяет:

* duplicate participant IDs;
* missing version;
* illegal dependency;
* dependency cycle;
* persistent owner без participant;
* participant без mapped state;
* migration gap;
* ambiguous required/optional semantics.

Нужен fixture inventory для historical save payloads. CI должен fail-closed при удалении поддерживаемого fixture без изменения compatibility policy.

## 5.2.8. Behavioral evidence

Обязательны тесты не только на encoder/decoder.

Нужны:

* non-empty participant payload roundtrip;
* zero/empty state;
* multiple create/update/remove cycles;
* version migration;
* unsupported version;
* missing required participant;
* missing optional participant;
* duplicated participant section;
* corrupted payload;
* truncated payload;
* limits exceeded;
* allocation failure до durable commit;
* storage failure;
* reopen в fresh engine instance.

## 5.2.9. Зона ответственности этапа

Разрешённая зона:

* SaveGame participant layer;
* Runtime Persistence integration;
* serialization/wire layer;
* durable storage abstraction;
* persistence validators;
* compatibility fixtures;
* persistence tests.

Не должны затрагиваться unrelated gameplay behavior и public API без подтверждённой необходимости.

## 5.2.10. Критерий выхода 5.2

Этап закрыт, когда:

* 100% persistent state из 5.1 имеет durable path;
* production participant graph покрывает Framework persistent state;
* Runtime persistent state имеет явно определённую роль в whole-engine save;
* wire format versioned и bounded;
* migration policy проверяется fixtures для всех реально поддерживаемых historical versions; version-1-only participants имеют reviewed `NO_PRIOR_SUPPORTED_VERSION`;
* durable storage roundtrip проходит;
* corrupted/truncated/incomplete save не принимается как valid;
* no-debt validator показывает 0 orphan persistent state и 0 orphan participants.

---

# 5.3. Unified save barrier and atomic save generation

## 5.3.1. Цель этапа

Доказать, что Framework SaveGame, Runtime Persistence, Time и durable checkpoints фиксируют один логический момент состояния.

Два independently valid snapshot не считаются whole-engine save, если они были сняты в разные causal moments.

## 5.3.2. Save generation contract

Whole-engine save должен иметь stable save generation или эквивалентную identity, которая связывает все persistence planes.

В одном save package должны быть согласованы минимум:

* Framework participant set;
* Runtime Persistence state;
* Runtime/Game Time checkpoint;
* deterministic random state, если он существует отдельно;
* durable integration checkpoints;
* migration/format metadata.

Validator должен обнаруживать package, где sections принадлежат несовместимым generation/revision.

Cross-plane coordinator должен находиться в архитектурно разрешённом composition/integration layer. Нельзя ради Goal 5 добавлять Runtime -> Framework dependency или заставлять низкоуровневый SaveGame core напрямую владеть Runtime services. Существующие architecture gates остаются обязательными на каждом шаге.

## 5.3.3. Save barrier semantics

Необходимо определить, какие действия разрешены в каждой фазе save.

Barrier contract должен явно отвечать:

* когда запрещается новая authoritative mutation либо каким образом snapshot isolation делает её безопасной;
* что происходит с уже accepted external work;
* что происходит с pending async completion;
* можно ли читать state во время capture;
* может ли Runtime materialization продолжаться и при каких epoch/generation правилах;
* должны ли Time/Simulation quiesce, либо каким другим механизмом доказывается consistent cut;
* когда mutation снова разрешается, если применялась quiescence;
* какой timeout/error policy у coordination mechanism.

Milestone 5 не предписывает обязательную глобальную паузу. Допустимы quiescence, epoch barrier, immutable snapshot isolation или эквивалентный механизм, если тестами доказан один consistent logical save point.

`ISaveBarrier` не должен оставаться test-only abstraction, если именно он выбран production coordination mechanism. Если архитектура выбирает другой эквивалентный механизм, canonical contract должен явно зафиксировать его и доказать те же invariants.

## 5.3.4. Accepted work

Особое внимание требуется операциям, которые уже были приняты внешним или нижним участником, но ещё не получили terminal local publication.

Barrier не имеет права:

* потерять accepted work;
* записать save до durable checkpoint такого work, если оно влияет на continuation;
* повторить side effect после restore;
* ожидать бесконечно без contract.

Для каждого такого boundary должен существовать один из вариантов:

* завершить до save;
* сохранить durable pending/reconciliation record;
* отменить до внешнего commit;
* отказать в save с контролируемой причиной.

## 5.3.5. Автоматическая проверка

Нужен save-barrier contract registry, сопоставленный с persistent checkpoints из 5.1.

Validator должен проверять:

* every pending durable category has barrier policy;
* no persistent participant captures outside generation;
* Runtime Persistence и Framework SaveGame share generation;
* Time checkpoint belongs to same generation;
* barrier phase transitions complete;
* no unknown async boundary.

Fault tests должны уметь останавливать save на каждой логической фазе и сравнивать pre/post state.

## 5.3.6. Behavioral scenarios

Нужно доказать:

* idle save;
* save при pending deferred work;
* save при accepted external work;
* save при streaming transition;
* save во время budget exhaustion/deferred queue;
* participant capture failure;
* Runtime Persistence failure;
* Time checkpoint failure;
* storage failure после capture;
* barrier release после failure;
* retry save после recoverable failure.

Главный invariant: неуспешный save не создаёт package, который выглядит valid, и не оставляет engine навсегда в quiesced state.

## 5.3.7. Зона ответственности этапа

Основная зона:

* save coordination;
* SaveGame orchestrator;
* Runtime Persistence coordination;
* Time/Simulation pause contract;
* integration checkpoints;
* barrier implementation;
* fault seams и tests.

Gameplay owners не должны получать новые обязанности, кроме минимально необходимого participation contract.

Milestone 5 квалифицирует только coherence save point и persistence-specific pending work. Общая thread-safety, races и callback lifetime остаются Goal 6 и не должны случайно расширять scope 5.3.

## 5.3.8. Критерий выхода 5.3

Этап закрыт, когда:

* существует production whole-engine save barrier;
* все persistence planes принадлежат одному logical save generation;
* pending/accepted work имеет explicit save policy;
* barrier failure/retry проверены;
* невозможно сформировать internally inconsistent valid package;
* validators и adversarial tests зелёные.

---

# 5.4. Whole-engine ordered restore and reconciliation

## 5.4.1. Цель этапа

Доказать восстановление не отдельных сервисов, а нового engine instance после полного уничтожения предыдущей composition.

Restore должен воспроизводить dependency order, не reuse transient identity и безопасно завершать reconciliation до возобновления simulation.

## 5.4.2. Restore phases

Whole-engine restore contract должен быть фазовым.

Рекомендуемая логическая модель:

1. `LOAD`
2. `VALIDATE`
3. `STAGE`
4. `COMMIT_SEMANTIC`
5. `REBUILD_TRANSIENT`
6. `RECONCILE_EXTERNAL`
7. `MATERIALIZE_RUNTIME`
8. `RESUME`

Точное количество внутренних фаз может отличаться. Важно разделить fallible pre-commit work и semantic commit.

До semantic commit restore может быть отменён без изменения live state.

После semantic commit нельзя притворяться, что accepted durable state можно просто откатить из-за Runtime backend failure. Такой failure переводит engine в explicit reconciliation-required state.

## 5.4.3. Dependency order

Порядок Framework participants должен определяться production participant dependencies, а не вручную в test harness.

Validator должен проверять:

* unique participant ID;
* missing dependency;
* dependency cycle;
* deterministic order при равноправных nodes;
* required participant set;
* restore order versus capture order contract.

## 5.4.4. Derived indexes

Milestone 5 не должен создавать global `RebuildAllIndexes()`.

Каждый authoritative owner отвечает за свои derived indexes внутри собственного staged restore contract.

Whole-engine harness проверяет invariants после restore, но не знает внутреннюю структуру indexes.

Так сохраняется ownership boundary.

## 5.4.5. RuntimeBridge и transient identity

RuntimeBridge mappings не должны сохраняться как reusable runtime identity.

После fresh boot:

* semantic Framework identity сохраняется;
* old Runtime handle считается invalid;
* Runtime object materializes заново;
* создаётся новая runtime identity;
* forward/reverse mapping согласованы;
* stale observation со старой generation отвергается;
* retry materialization не создаёт duplicate representation.

Это отдельный обязательный regression cluster Goal 5.

## 5.4.6. Reconciliation contract

Post-restore reconciliation должна иметь explicit состояние.

Минимальная state machine:

`RESTORED_SEMANTIC`

`RECONCILIATION_REQUIRED`

`RECONCILING`

`READY_TO_RESUME`

`FAILED_TERMINAL`, только если contract допускает terminal failure.

Time и Simulation не должны возобновлять semantic execution до `READY_TO_RESUME`.

Recoverable external failure должен оставлять достаточно durable/local ownership для idempotent retry.

## 5.4.7. Whole-engine harness

Harness обязан использовать production boot/composition и production save/restore coordinator настолько полно, насколько позволяет headless profile.

Он должен уметь:

* создать non-trivial state сразу в Runtime и Framework;
* сохранить его production save path;
* уничтожить все instances;
* создать fresh composition;
* загрузить durable package;
* restore;
* reconcile;
* materialize;
* продолжить causal scenario.

Нельзя считать whole-engine restore доказанным, если test вызывает несколько `RestoreSnapshot()` вручную.

Fresh-composition restore обязателен для основного whole-engine suite. Отдельно должен существовать хотя бы один process-boundary storage/reopen smoke, чтобы static/process-local state не могло незаметно подменить durable restore. Не каждый regression обязан spawn-ить новый процесс.

## 5.4.8. Failure matrix

Обязательные failure points:

| Фаза | Проверяемое поведение |
|---|---|
| LOAD | corrupted/truncated storage rejected |
| VALIDATE | live state не изменён |
| STAGE | live state не изменён |
| COMMIT preparation | commit не начинается, если no-fail completion не гарантирован |
| REBUILD_TRANSIENT | controlled recovery policy |
| RECONCILE_EXTERNAL | reconciliation state сохраняется, retry безопасен |
| MATERIALIZE_RUNTIME | semantic state не теряется, duplicate representation не появляется |
| RESUME | simulation не стартует до готовности |

## 5.4.9. Критерий выхода 5.4

Этап закрыт, когда:

* fresh-engine whole restore проходит;
* dependency order production-driven;
* derived indexes valid;
* RuntimeBridge identities rebuilt;
* stale old Runtime identities rejected;
* pre-commit failures atomic;
* post-commit external failures переходят в controlled reconciliation;
* continuation после successful restore работает;
* harness зарегистрирован как отдельный persistence/whole-engine suite.

---

# 5.5. Deterministic semantic scenario runner

## 5.5.1. Цель этапа

Создать общий инструмент, который позволяет доказательно сравнивать две executions одного semantic scenario.

Runner является основной автоматической инфраструктурой второй половины Milestone 5.

## 5.5.2. Граница deterministic domain

Milestone 5 квалифицирует deterministic semantic simulation, а не весь физический процесс целиком.

В deterministic domain входят state и события, которые влияют на дальнейшее gameplay behavior.

В deterministic digest по умолчанию не входят:

* pointer/allocator addresses;
* Runtime transient handles;
* thread IDs;
* wall-clock timestamps;
* diagnostics counters;
* log formatting;
* RHI object identities;
* renderer/audio backend internal state;
* timing измерения test runner.

Каждое исключение должно быть связано с persistence map/determinism registry. Нельзя исключать state из digest только потому, что он даёт нестабильный test.

## 5.5.3. Scenario input model

Runner должен получать не произвольные callbacks, а canonical ordered inputs.

Пример классов input step:

* `SubmitCommand`
* `AdvanceGameTime`
* `CompleteExternalOperation`
* `ApplyBudget`
* `StreamOut`
* `StreamIn`
* `SaveReload`
* `Tick`
* `Reconcile`
* `Materialize`

External completion order является частью input stream там, где backend completion сам по себе не входит в deterministic engine contract.

## 5.5.4. Semantic digest

Digest должен строиться из canonical state contributions.

Каждый deterministic-relevant owner из persistence map должен иметь одно из решений:

* `INCLUDED`
* `DERIVED_AND_VERIFIED`
* `EXCLUDED_NON_SEMANTIC` с rationale.

Digest contribution должна быть:

* stable ordered;
* independent of pointer identity;
* independent of unordered container iteration;
* canonical по integer/enum/string encoding;
* explicit по floating representation.

Нельзя использовать raw in-memory bytes C++ object.

Нежелательно использовать raw save package как единственный semantic digest, потому что persistence encoding и semantic equivalence являются разными контрактами.

## 5.5.5. Semantic event trace

Runner дополнительно собирает ordered semantic event trace.

Trace отражает externally meaningful semantic commits, например:

* entity lifecycle;
* condition/effect application;
* combat result;
* loot/reward commit;
* progression change;
* process completion;
* economy transaction;
* AI intent commit;
* schedule trigger.

Internal retry attempts, diagnostics и backend implementation events не должны автоматически становиться semantic events.

Каждый trace event должен иметь stable identity и canonical payload representation.

## 5.5.6. Stable test identity

Каждый deterministic scenario должен иметь:

* scenario ID;
* deterministic config ID;
* seed set;
* initial state identity;
* ordered input stream identity;
* expected digest policy;
* expected trace policy.

При failure test обязан печатать достаточно информации для exact reproduction.

## 5.5.7. Автоматические проверки runner infrastructure

Нужны self-tests, которые намеренно внедряют:

* unordered iteration;
* pointer-derived value;
* wall-clock value;
* random seed from ambient source;
* missing digest contributor;
* duplicate event ordering;
* unstable tie-breaking.

Runner/validator должен обнаруживать эти нарушения.

## 5.5.8. Критерий выхода 5.5

Этап закрыт, когда:

* существует canonical scenario model;
* каждый deterministic-relevant owner имеет digest decision;
* semantic digest и trace reproducible;
* runner умеет save/reload и fresh-engine continuation;
* failure output полностью воспроизводим;
* adversarial self-tests доказывают чувствительность к nondeterministic behavior.

---

# 5.6. Determinism audit and scenario matrix

## 5.6.1. Цель этапа

Использовать runner из 5.5 для системного доказательства deterministic continuation и одновременно провести целевой аудит известных источников nondeterminism.

Этот этап не является ручным просмотром всего проекта.

Сначала scanners строят candidate inventory. Затем reviewed registry закрывает каждый candidate. Затем executable scenarios проверяют поведение.

## 5.6.2. Determinism candidate discovery

Статический scanner должен работать по production sources/headers и machine-generated production inventory, исключая tests, third-party, build output и generated fixtures. По возможности он должен ограничивать кандидатов модулями/paths, которые могут влиять на deterministic semantic domain; остальные находки классифицируются module-level `N/A`, а не создают ручной аудит всего дерева.

Scanner должен находить как минимум:

* `unordered_map` / `unordered_set` в state, откуда может формироваться observable order;
* `std::random_device`;
* создание random seed из clock/process state;
* `system_clock`, `steady_clock`, high-resolution clocks внутри semantic decisions;
* pointer/address hashing;
* ordering по object address;
* floating calculations, влияющие на stable semantic branch;
* async completion order, влияющий на commit order;
* same-time schedule without stable tie breaker;
* serialization iteration по unordered containers;
* ID generation, зависящее от execution timing;
* parallel reduction без deterministic policy.

Scanner выдаёт candidates, а не defects.

## 5.6.3. Reviewed determinism registry

Для каждого candidate фиксируется decision:

* `DETERMINISTIC_BY_CONSTRUCTION`
* `CANONICALIZED`
* `EXPLICIT_INPUT`
* `BUDGET_NON_SEMANTIC`
* `TOLERANCE_DEFINED`
* `EXCLUDED_NON_SEMANTIC`
* `DEFECT`

Каждое решение требует rationale и evidence.

## 5.6.4. Wall-clock budgets

Runtime subsystems могут легитимно использовать `steady_clock` для реального frame/work budget.

Deterministic profile не должен позволять производительности машины менять semantic result.

Для каждого такого budgeted subsystem выбирается один contract:

1. deterministic tests используют deterministic work-count budget;
2. wall-clock completion boundary становится explicit scenario input;
3. subsystem доказанно влияет только на latency, но не на semantic committed result/order.

Нельзя считать «обычно одинаково работает» достаточным доказательством.

## 5.6.5. Randomness

Требуется доказать:

* все semantic randomness идёт через explicit stream;
* seed и stream identity известны;
* random state сохраняется, если влияет на future continuation;
* save/load продолжает тот же stream;
* нет ambient `random_device` или clock seed внутри deterministic domain;
* independent streams не используют hidden shared mutable state.

## 5.6.6. Ordering

Нужно проверить:

* unordered containers не задают observable order без sort/canonicalization;
* same timestamp имеет explicit tie-breaker;
* equal-priority work имеет stable order либо order не влияет на semantics;
* async completion не становится implicit semantic ordering source;
* serialization canonical ordering отделена от internal container ordering;
* event queues имеют documented deterministic ordering.

## 5.6.7. Floating behavior

Не требуется обещать bit-identical graphics/physics backend state на всех toolchains.

Для каждого floating-sensitive semantic path определяется:

* fixed/integer representation;
* canonical quantization;
* explicit tolerance;
* exclusion from deterministic domain.

Semantic digest не должен напрямую включать non-canonical floating bytes.

## 5.6.8. Уровни determinism qualification

Milestone 5 должен явно определить scope обещания.

| Уровень | Требование |
|---|---|
| D1 | Повторный run в одном executable даёт одинаковый result |
| D2 | Fresh process с тем же build даёт одинаковый result |
| D3 | Debug и Release одного compiler family дают semantic equivalent result |
| D4 | Разные поддерживаемые compiler families дают semantic equivalent result |

D1 и D2 обязательны.

D3 рекомендуется сделать обязательным для semantic/headless profile, если это не требует обещания, противоречащего floating contract.

D4 выполняется только если проект хочет официально обещать cross-compiler semantic determinism. Иначе cross-compiler run остаётся portability detector, а не frozen contract.

Ни D3, ни D4 не должны автоматически становиться blocker Milestone 5 без принятого compatibility contract. Обязательный freeze contract Milestone 5 по умолчанию: D1 + D2. Расширение обещания фиксируется отдельно и затем становится обязательным.

## 5.6.9. Scenario matrix

Минимальный обязательный набор:

### Scenario A: control continuation

Non-trivial world state, несколько systems, N ticks без save/load.

### Scenario B: save/load equivalence

Тот же input stream, но на фиксированном шаге выполняются save, destroy, fresh boot, restore и continuation.

Финальный semantic digest и post-normalization event trace должны совпасть с Scenario A.

### Scenario C: streaming interruption

В середине causal scenario выполняются stream-out и stream-in с последующим продолжением.

### Scenario D: deferred/budget exhaustion

Одинаковая semantic work выполняется с ограниченным budget и deferred continuation.

### Scenario E: combat/effects

Ability, combat, effects/conditions, terminal outcome, reward/progression.

### Scenario F: perception/knowledge/AI

Observation, knowledge update, decision, target change, replan.

### Scenario G: economy/process/resources

Reservation, process execution, resource production, commit/retry.

### Scenario H: runtime materialization

Semantic object, RuntimeBridge materialization, save/load, fresh runtime identities, stale old observation rejection.

## 5.6.10. Repetition

Каждый core deterministic scenario должен выполняться минимум:

* 10 повторов одного build;
* несколько fresh-process повторов;
* control и save/load variants;
* fixed seed report;
* failure reproduction metadata.

Большую flakiness campaign можно оставить Goal 9. Milestone 5 обязан доказать устойчивый deterministic contract для своих сценариев.

## 5.6.11. Критерий выхода 5.6

Этап закрыт, когда:

* determinism discovery inventory полностью reviewed;
* unknown candidate = 0;
* deterministic scenarios зелёные;
* save/load не меняет semantic continuation;
* wall-clock budgets не влияют неявно на semantic result;
* random streams и ordering contracts доказаны;
* D1/D2 qualification проходит для всего обязательного scenario set;
* все найденные defects имеют permanent regression.

---

# 5.7. Final Milestone 5 qualification

## 5.7.1. Цель этапа

Свести результаты 5.1–5.6 в единый fail-closed gate и доказать отсутствие persistence/determinism debt перед переходом к memory/lifetime/concurrency qualification.

Этот этап не добавляет новую функциональность. Он проверяет полноту предыдущих контрактов.

## 5.7.2. Canonical Goal 5 status

Milestone 5 должен иметь собственный machine-readable status, независимый от рабочих документов.

Для каждого автоматически обнаруженного production module фиксируются applicability и result минимум по двум областям. На текущем baseline таких modules 78, но validator использует discovery inventory, а не hard-coded count:

* persistence;
* determinism.

Разрешённые итоговые решения:

* `PASS`
* `N/A` с reviewed rationale
* `BLOCKED`

Нельзя использовать `UNKNOWN` или автоматически считать `N/A`.

## 5.7.3. Обязательные canonical validators

Финальный Goal 5 gate должен агрегировать минимум следующие проверки:

| Validator | Что доказывает |
|---|---|
| Persistence discovery/contract | Все state candidates классифицированы |
| Participant coverage | Каждый durable owner имеет save path |
| Schema/version validator | Version/migration contract непротиворечив |
| Storage contract | Durable package можно записать и открыть |
| Save generation/barrier | Все planes принадлежат одному save point |
| Restore graph | Ordered restore и dependencies valid |
| Reconciliation contract | Post-restore external state контролируем |
| Determinism inventory | Все nondeterminism candidates reviewed |
| Digest coverage | Каждый deterministic-relevant owner имеет digest policy |
| Scenario qualification | Control/save-load scenarios equivalent |
| Evidence quality | Нет placeholder/generic evidence |
| Goal 5 contract | Все обязательства сведены в PASS/N/A/BLOCKED |

Каждый validator имеет positive и negative fixtures.

## 5.7.4. Test taxonomy

После Goal 5 в CTest должны быть различимы как минимум:

* `persistence-local`
* `persistence-wire`
* `persistence-storage`
* `persistence-whole-engine`
* `determinism`
* `determinism-save-load`
* `goal5-regression`

Existing module tests не обязательно переименовывать. Новые системные suites должны иметь однозначные labels.

## 5.7.5. Final qualification matrix

На exact Goal 5 tree требуется локально воспроизвести поддерживаемую strict build/test matrix проекта и дополнительно выполнить Goal 5 suites.

Минимум:

* Base Debug/Release;
* Runtime Debug/Release;
* Full Debug/Release;
* warnings-as-errors;
* exact CTest manifests;
* public-header self-containment;
* freeze validators;
* persistence validators;
* Goal 5 self-tests;
* whole-engine restore;
* deterministic scenario suite.

Remote CI является подтверждением exact recorded SHA. Локальный Milestone 5 не должен зависеть от Markdown roadmap.

## 5.7.6. Exit criteria

Milestone 5 считается завершённым только при одновременном выполнении всех условий:

```text
100% discovered production modules persistence classification = reviewed
100% discovered production modules determinism classification = reviewed

unclassified persistence candidates = 0
unclassified determinism candidates = 0

persistent owners without durable path = 0
persistent checkpoints without owner = 0
transient reusable runtime identities in save = 0

participant schema/version gaps = 0
unsupported silent migrations = 0

Framework SaveGame and Runtime Persistence use one logical save generation
production save barrier = qualified

durable storage roundtrip = PASS
fresh-engine whole restore = PASS
post-restore reconciliation = PASS
RuntimeBridge rematerialization = PASS

deterministic semantic runner = PASS
control vs save/load continuation = equivalent
required deterministic scenarios = PASS
D1/D2 repetition = PASS

Goal 5 validators/self-tests = PASS
known persistence/determinism P0/P1 = 0
known unresolved Goal 5 code debt = 0
```

## 5.7.7. Что Milestone 5 не присваивает

Milestone 5 не присваивает всему движку `SYSTEM_READY` или `FROZEN`.

После него доказаны persistence и deterministic continuation.

Memory/lifetime/concurrency остаются Goal 6.

Cross-module causal chains в полном объёме остаются Goal 7.

Load/degradation и broad failure qualification остаются Goal 8.

Final whole-engine freeze остаётся Goal 9.

Итоговый статус Milestone 5 должен означать только:

`PERSISTENCE_QUALIFIED`

`DETERMINISM_QUALIFIED`

`GOAL5_COMPLETE`

---

# 6. Последовательность выполнения Milestone 5

Работа выполняется строго в следующем порядке.

## Phase 1: 5.1 Persistence ownership map

Сначала формируется полный inventory и canonical classification.

Нельзя писать массовые save participants до того, как доказано, какие state действительно являются persistent.

## Phase 2: 5.2 Durable representation

На основании карты закрывается participant coverage, versioning, wire format и durable storage.

Нельзя проектировать whole-engine restore, пока durable package не имеет стабильного contract.

## Phase 3: 5.3 Unified save point

После появления полного durable path связываются Framework, Runtime Persistence, Time и checkpoints одним save generation и production barrier.

## Phase 4: 5.4 Ordered restore

Только после определения atomic save point строится fresh-engine restore и reconciliation model.

## Phase 5: 5.5 Deterministic runner

Runner строится поверх уже работающего whole-engine save/restore, чтобы `SaveReload` являлся production-equivalent operation, а не test shortcut.

## Phase 6: 5.6 Determinism qualification

Runner используется для candidate-driven audit и обязательных system scenarios.

## Phase 7: 5.7 Final qualification

Последним шагом сводятся canonical contracts, validators, tests и exact-tree build matrix.

Переход к Goal 6 разрешён только после полного выхода 5.7.

---

# 7. Архитектура автоматической проверки Milestone 5

Для проекта текущего размера ручной аудит исходников используется только для расследования конкретного candidate или defect.

Основная модель контроля должна быть следующей:

```text
source tree
    |
    v
automatic discovery
    |
    v
candidate inventory
    |
    v
reviewed machine-readable contract
    |
    +--> structural validator
    |
    +--> adversarial validator self-tests
    |
    +--> executable behavioral evidence
    |
    v
canonical Goal 5 ledger
```

Discovery scanners должны быть deterministic и reproducible.

Reviewed registry является source of truth для semantic decisions.

Validator доказывает полноту registry относительно автоматически найденной поверхности.

Executable tests доказывают реальное поведение.

Ledger не имеет права объявлять PASS только по наличию записи в registry.

## 7.1. Что нельзя автоматизировать слепо

Нельзя автоматически решить:

* является ли state authoritative;
* должен ли он переживать save;
* является ли runtime identity reusable;
* какая migration policy является правильной;
* какой floating tolerance семантически допустим;
* является ли external completion частью deterministic input.

Автоматизация должна находить поверхность и проверять непротиворечивость. Semantic decision проходит review один раз и затем фиксируется машинно.

## 7.2. Что обязательно автоматизируется полностью

Не должно требовать ручного просмотра:

* completeness inventory;
* duplicate/missing IDs;
* unknown classifications;
* dependency cycles;
* missing participant coverage;
* schema/version gaps;
* stale evidence;
* invalid test target;
* forbidden transient field categories;
* unreviewed nondeterminism candidate;
* digest coverage;
* scenario repetition;
* control/reload result comparison;
* merge/idempotence;
* generated canonical artifacts;
* CI enforcement.

---

# 8. Правила работы с найденными проблемами

Каждый defect Milestone 5 получает stable ID.

Рекомендуемые области IDs:

* `G5-MAP-*`
* `G5-SAVE-*`
* `G5-SCHEMA-*`
* `G5-STORAGE-*`
* `G5-BARRIER-*`
* `G5-RESTORE-*`
* `G5-RECON-*`
* `G5-DET-*`

Defect record должен содержать:

* reproduction;
* violated contract;
* root cause;
* affected owner/participant;
* fix scope;
* regression target;
* test case identity;
* compatibility impact;
* persistence/determinism evidence;
* review status.

`CLOSED` как произвольный worker status не используется. Canonical review status должен иметь единую schema.

Production fix принимается только после появления regression.

---

# 9. Критерии отсутствия долга

После завершения Milestone 5 недопустимы следующие остатки:

* «этот snapshot вроде не нужен» без classification;
* «этот runtime handle, скорее всего, пересоздаётся» без regression;
* «migration пока не нужна» при уже versioned public save contract без policy;
* «storage потом»;
* «save/load тестируется вручную»;
* «determinism проверяли несколько раз, вроде совпало»;
* «unordered container здесь обычно возвращает одинаково»;
* «wall-clock budget на быстрых машинах не мешает»;
* «после restore backend обычно успешно создаётся»;
* `N/A` без rationale;
* validator без negative self-tests;
* PASS на основании generic test anchor;
* canonical status, зависящий от Markdown roadmap;
* hidden auto-repair, после которого первый merge меняет canonical evidence;
* known persistence/determinism defect без permanent regression.

Milestone 5 закрывается не количеством новых tests и не объёмом изменённого кода.

Он закрывается только тогда, когда persistence и determinism surface стали конечными, перечислимыми, автоматически проверяемыми и воспроизводимыми на exact tree.
