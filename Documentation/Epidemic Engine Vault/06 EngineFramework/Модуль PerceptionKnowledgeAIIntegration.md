---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "perception_knowledge_a_i_integration"]
source_path: "EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration"
---

# EngineFramework / PerceptionKnowledgeAIIntegration

Модуль: **PerceptionKnowledgeAIIntegration**. Связь восприятия, знаний и AI.

**Реализация:** `EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/`.
**CMake targets:** `EpidemicGameFrameworkPerceptionKnowledgeAIIntegration`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Goal 4 block: `b08_integration_adapters`. Freeze unit: `EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration`. Admission: 14 public API, 12 mutation obligations, 0 lifecycle, 2 stale-identity, 0 external-boundary candidates.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/PerceptionKnowledgeAIIntegration/perception_knowledge_ai_adapters.h|perception_knowledge_ai_adapters.h]]: `EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/include/Epidemic/GameFramework/PerceptionKnowledgeAIIntegration/perception_knowledge_ai_adapters.h`; объявленные типы: `PerceptionKnowledgeMapping`, `ObservationKnowledgeResult`, `PerceptionKnowledgeAdapter`, `AIExecutionAvailability`, `KnowledgeAIInputKeys`, `KnowledgeAIAdapter`, `AIIntentExecutionRecorder`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/PerceptionKnowledgeAIIntegration|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Public contract review

- Observation -> Knowledge mapping.
- Duplicate observation.
- Knowledge update -> AI inputs.
- Missing knowledge.
- AI execution availability.
- Intent execution record exactly once.
- Stale observation/agent generation.

#### Failure atomicity and accepted-work review

Every local adapter mutation was reviewed for success, invalid input, retry/duplicate behavior, stale identities/cursors and publication ordering. Fallible local bookkeeping must complete before an externally owned side effect is accepted, or a durable reconciliation record must already exist.

#### Persistence and reconciliation

No independent adapter-owned snapshot/checkpoint surface is required by this freeze unit. Durable state is either carried by owner-issued tokens/outputs or explicitly delegated to the authoritative owners; whole-engine persistence remains Goal 5.

#### Test evidence

- Source: `EngineFramework/DevelopmentInfrastructure/Tests/perception_knowledge_ai_integration_tests.cpp`.
- Target: `EpidemicGameFrameworkPerceptionKnowledgeAIIntegrationTests`.
- Reviewed paths include happy path, invalid/stale input, duplicate/retry behavior and local failure/reconciliation behavior applicable to this module.
- The B08 worker compiled and executed this target under C++23 with Clang warnings-as-errors in the available Linux environment. Required Windows MSVC Debug/Release qualification remains a serial-integration gate.

#### Goal 4 result

`LOCAL_READY_CANDIDATE` for block-local evidence. This is not a whole-Framework `LOCAL_READY`, `SYSTEM_READY` or `FROZEN` claim.

<!-- goal4-exact-evidence -->

Источник контрактов: `Documentation/EngineFramework/PerceptionKnowledgeAIIntegration.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Операции и хранилища состояния

#### `AIIntentExecutionRecorder`

Объявлен в `EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/include/Epidemic/GameFramework/PerceptionKnowledgeAIIntegration/perception_knowledge_ai_adapters.h:87`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
explicit AIIntentExecutionRecorder(std::size_t retention_capacity = 1024) noexcept;
[[nodiscard]] foundation::Result<void> Accept(const ai::AIIntent &intent, ai::AIService &ai_service);
[[nodiscard]] foundation::Result<void> Succeed(ai::AIIntentId intent, ai::AIService &ai_service);
[[nodiscard]] const std::vector<ai::AIIntent> &AcceptedIntents() const noexcept;
[[nodiscard]] std::size_t RetentionCapacity() const noexcept;
void Clear() noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::size_t retention_capacity_ = 1024`; `std::vector<ai::AIIntent> accepted_`.

#### `PerceptionKnowledgeAdapter`

Объявлен в `EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/include/Epidemic/GameFramework/PerceptionKnowledgeAIIntegration/perception_knowledge_ai_adapters.h:33`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] foundation::Result<ObservationKnowledgeResult> LearnFromObservation(const perception::PerceptionObservation &observation);
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `PerceptionKnowledgeMapping mapping_`.

#### `ObservationKnowledgeResult`

Объявлен в `EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/include/Epidemic/GameFramework/PerceptionKnowledgeAIIntegration/perception_knowledge_ai_adapters.h:23`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::optional<knowledge::MemoryRecordId> memory`; `bool memory_creation_failed = false`; `std::optional<foundation::Error> memory_error`.

#### `AIExecutionAvailability`

Объявлен в `EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/include/Epidemic/GameFramework/PerceptionKnowledgeAIIntegration/perception_knowledge_ai_adapters.h:46`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `bool materialized = false`; `bool runtime_projection_available = false`.

#### `KnowledgeAIAdapter`

Объявлен в `EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/include/Epidemic/GameFramework/PerceptionKnowledgeAIIntegration/perception_knowledge_ai_adapters.h:73`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `KnowledgeAIInputKeys input_keys_`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkPerception`, `EpidemicFoundation`, `EpidemicGameFrameworkSaveGame`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/src/save_participants.cpp` (105 строк)
- `EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/src/perception_knowledge_ai_adapters.cpp` (387 строк): `PerceptionKnowledgeAdapter::PerceptionKnowledgeAdapter`, `PerceptionKnowledgeAdapter::LearnFromObservation`, `Error::Create`, `KnowledgeSourceId::FromString`, `KnowledgeAIAdapter::KnowledgeAIAdapter`, `KnowledgeAIAdapter::BuildContext`, `KnowledgeTopicId::FromString`, `AIInputKeyId::FromType`, `AIIntentExecutionRecorder::AIIntentExecutionRecorder`, `AIIntentExecutionRecorder::RecordAccepted`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `framework.ai`: `EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/src/save_participants.cpp:95`
- `framework.knowledge`: `EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/src/save_participants.cpp:99`
- `framework.perception`: `EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/src/save_participants.cpp:103`
- `gameplay.integration.perception.invalid_observation`: `EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/src/perception_knowledge_ai_adapters.cpp:214`
- `framework.knowledge.direct_perception`: `EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/src/perception_knowledge_ai_adapters.cpp:230`
- `framework.knowledge.last_known_position`: `EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/src/perception_knowledge_ai_adapters.cpp:280`

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `framework.ai` (`EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/src/save_participants.cpp:95`)

```cpp
93 std::unique_ptr<ISaveParticipant> CreateAIServiceSaveParticipant(epidemic::gameplay::ai::AIService& owner, SaveRestoreWorkspace& workspace)
   94 {
   95     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::ai::AIService>>(owner, workspace, "framework.ai", std::vector<SaveParticipantId>{});
   96 }
   97 std::unique_ptr<ISaveParticipant> CreateKnowledgeServiceSaveParticipant(epidemic::gameplay::knowledge::KnowledgeService& owner, SaveRestoreWorkspace& workspace)
```

#### `framework.knowledge` (`EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/src/save_participants.cpp:99`)

```cpp
97 std::unique_ptr<ISaveParticipant> CreateKnowledgeServiceSaveParticipant(epidemic::gameplay::knowledge::KnowledgeService& owner, SaveRestoreWorkspace& workspace)
   98 {
   99     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::knowledge::KnowledgeService>>(owner, workspace, "framework.knowledge", std::vector<SaveParticipantId>{});
  100 }
  101 std::unique_ptr<ISaveParticipant> CreatePerceptionServiceSaveParticipant(epidemic::gameplay::perception::PerceptionService& owner, SaveRestoreWorkspace& workspace)
```

#### `framework.perception` (`EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/src/save_participants.cpp:103`)

```cpp
101 std::unique_ptr<ISaveParticipant> CreatePerceptionServiceSaveParticipant(epidemic::gameplay::perception::PerceptionService& owner, SaveRestoreWorkspace& workspace)
  102 {
  103     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::perception::PerceptionService>>(owner, workspace, "framework.perception", std::vector<SaveParticipantId>{});
  104 }
  105 }
```

#### `gameplay.integration.perception.invalid_observation` (`EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/src/perception_knowledge_ai_adapters.cpp:214`)

```cpp
212     {
  213         return foundation::Result<ObservationKnowledgeResult>::Failure(foundation::Error::Create(
  214             "gameplay.integration.perception.invalid_observation", "perception observation cannot be learned"));
  215     }
  216
```

#### `framework.knowledge.direct_perception` (`EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/src/perception_knowledge_ai_adapters.cpp:230`)

```cpp
228     learn.type = mapping_.observed_belief;
  229     learn.topic = std::move(topic);
  230     learn.source_kind = knowledge::KnowledgeSourceId::FromString("framework.knowledge.direct_perception");
  231     // PerceptionObservation is already the information boundary. Never consult the stimulus source here.
  232     learn.source_object = observation.perceived_subject;
```

#### `framework.knowledge.last_known_position` (`EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/src/perception_knowledge_ai_adapters.cpp:280`)

```cpp
278 
  279     std::map<GameplayObjectRef, CandidateState> candidates;
  280     const auto last_known_topic = knowledge::KnowledgeTopicId::FromString("framework.knowledge.last_known_position");
  281 
  282     auto records = knowledge_.FindKnowledgeByOwner(agent);
```
