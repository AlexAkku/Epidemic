---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "world_integration"]
source_path: "EngineFramework/IntegrationLayer/WorldIntegration"
---

# EngineFramework / WorldIntegration

Модуль: **WorldIntegration**. Связь gameplay world.

**Реализация:** `EngineFramework/IntegrationLayer/WorldIntegration/`.
**CMake targets:** `EpidemicGameFrameworkWorldIntegration`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: B07 Goal 4 review. Local integration contract only.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/WorldIntegration/world_adapters.h|world_adapters.h]]: `EngineFramework/IntegrationLayer/WorldIntegration/include/Epidemic/GameFramework/WorldIntegration/world_adapters.h`; объявленные типы: `AreasAtPositionQuery`, `AlterationsInAreaQuery`, `EnvironmentSampleQuery`, `ActiveInteractionsQuery`, `WorldFactsCheckpoint`, `WorldQueryAdapter`, `WorldFactsAdapter`, `EntityInteractionStateProvider`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/modules/world_integration|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Операции и хранилища состояния

#### `WorldFactsAdapter`

Объявлен в `EngineFramework/IntegrationLayer/WorldIntegration/include/Epidemic/GameFramework/WorldIntegration/world_adapters.h:98`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
WorldFactsAdapter(world::WorldService &w, environment::EnvironmentService &e, interaction::InteractionService &i, facts::GameplayFactsService &f) : world_(w), environment_(e), interaction_(i), facts_(f);
[[nodiscard]] foundation::Result<void> RegisterContracts();
[[nodiscard]] WorldFactsCheckpoint CaptureCheckpoint() const noexcept;
[[nodiscard]] foundation::Result<void> RestoreCheckpoint(WorldFactsCheckpoint checkpoint);
```

#### `EntityInteractionStateProvider`

Объявлен в `EngineFramework/IntegrationLayer/WorldIntegration/include/Epidemic/GameFramework/WorldIntegration/world_adapters.h:125`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
explicit EntityInteractionStateProvider(const entities::EntityService &e) : entities_(e);
[[nodiscard]] bool IsMaterialized(GameplayObjectRef object) const override;
[[nodiscard]] Revision RevisionOf(GameplayObjectRef object) const override;
```

#### `WorldQueryAdapter`

Объявлен в `EngineFramework/IntegrationLayer/WorldIntegration/include/Epidemic/GameFramework/WorldIntegration/world_adapters.h:81`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
WorldQueryAdapter(world::WorldService &w, environment::EnvironmentService &e, interaction::InteractionService &i, queries::GameplayQueryService &q) : world_(w), environment_(e), interaction_(i), queries_(q);
[[nodiscard]] foundation::Result<void> RegisterProviders();
```

#### `AreasAtPositionQuery`

Объявлен в `EngineFramework/IntegrationLayer/WorldIntegration/include/Epidemic/GameFramework/WorldIntegration/world_adapters.h:18`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] static constexpr QueryTypeId Type() noexcept;
```

#### `AlterationsInAreaQuery`

Объявлен в `EngineFramework/IntegrationLayer/WorldIntegration/include/Epidemic/GameFramework/WorldIntegration/world_adapters.h:27`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] static constexpr QueryTypeId Type() noexcept;
```

#### `EnvironmentSampleQuery`

Объявлен в `EngineFramework/IntegrationLayer/WorldIntegration/include/Epidemic/GameFramework/WorldIntegration/world_adapters.h:37`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] static constexpr QueryTypeId Type() noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::vector<GameplayObjectRef> scopes`.

#### `ActiveInteractionsQuery`

Объявлен в `EngineFramework/IntegrationLayer/WorldIntegration/include/Epidemic/GameFramework/WorldIntegration/world_adapters.h:48`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] static constexpr QueryTypeId Type() noexcept;
```

#### `WorldFactsCheckpoint`

Объявлен в `EngineFramework/IntegrationLayer/WorldIntegration/include/Epidemic/GameFramework/WorldIntegration/world_adapters.h:58`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `static constexpr std::uint32_t kSchemaVersion = 2`; `std::uint32_t schema_version = kSchemaVersion`; `std::uint64_t world_sequence = 0`; `std::uint64_t environment_sequence = 0`; `std::uint64_t interaction_sequence = 0`; `std::uint64_t world_epoch = 0`; `std::uint64_t environment_epoch = 0`; `std::uint64_t interaction_epoch = 0`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/IntegrationLayer/WorldIntegration/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkWorld`, `EpidemicFoundation`, `EpidemicGameFrameworkSaveGame`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/IntegrationLayer/WorldIntegration/src/save_participants.cpp` (121 строк): `SaveParticipantId::FromString`
- `EngineFramework/IntegrationLayer/WorldIntegration/src/world_adapters.cpp` (275 строк): `WorldService::Domain`, `WorldQueryAdapter::RegisterProviders`, `Error::Create`, `WorldFactsAdapter::RegisterContracts`, `EnvironmentService::Domain`, `InteractionService::Domain`, `WorldFactsAdapter::PublishPending`, `ProducerId::FromString`, `WorldFactsAdapter::CaptureCheckpoint`, `WorldFactsAdapter::RestoreCheckpoint`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `framework.environment`: `EngineFramework/IntegrationLayer/WorldIntegration/src/save_participants.cpp:119`
- `framework.interaction`: `EngineFramework/IntegrationLayer/WorldIntegration/src/save_participants.cpp:119`
- `framework.world`: `EngineFramework/IntegrationLayer/WorldIntegration/src/save_participants.cpp:119`
- `integration.world_facts`: `EngineFramework/IntegrationLayer/WorldIntegration/src/save_participants.cpp:119`
- `framework.facts`: `EngineFramework/IntegrationLayer/WorldIntegration/src/save_participants.cpp:119`
- `framework.query.world.areas_at`: `EngineFramework/IntegrationLayer/WorldIntegration/src/world_adapters.cpp:53`
- `framework.query.world.alterations`: `EngineFramework/IntegrationLayer/WorldIntegration/src/world_adapters.cpp:62`
- `framework.query.environment.sample`: `EngineFramework/IntegrationLayer/WorldIntegration/src/world_adapters.cpp:72`
- `gameplay.world_integration.incoherent_read`: `EngineFramework/IntegrationLayer/WorldIntegration/src/world_adapters.cpp:97`
- `framework.query.interaction.active`: `EngineFramework/IntegrationLayer/WorldIntegration/src/world_adapters.cpp:107`
- `framework.world.changed`: `EngineFramework/IntegrationLayer/WorldIntegration/src/world_adapters.cpp:117`
- `framework.environment.changed`: `EngineFramework/IntegrationLayer/WorldIntegration/src/world_adapters.cpp:123`
- `framework.interaction.changed`: `EngineFramework/IntegrationLayer/WorldIntegration/src/world_adapters.cpp:128`
- `framework.world_integration`: `EngineFramework/IntegrationLayer/WorldIntegration/src/world_adapters.cpp:137`
- `gameplay.world_integration.snapshot_required`: `EngineFramework/IntegrationLayer/WorldIntegration/src/world_adapters.cpp:141`
- `gameplay.world_integration.environment_snapshot_required`: `EngineFramework/IntegrationLayer/WorldIntegration/src/world_adapters.cpp:155`
- `gameplay.world_integration.interaction_snapshot_required`: `EngineFramework/IntegrationLayer/WorldIntegration/src/world_adapters.cpp:170`
- `gameplay.world_integration.checkpoint_version`: `EngineFramework/IntegrationLayer/WorldIntegration/src/world_adapters.cpp:205`
- `gameplay.world_integration.world_checkpoint_gap`: `EngineFramework/IntegrationLayer/WorldIntegration/src/world_adapters.cpp:229`
- `gameplay.world_integration.environment_checkpoint_gap`: `EngineFramework/IntegrationLayer/WorldIntegration/src/world_adapters.cpp:238`
- `gameplay.world_integration.interaction_checkpoint_gap`: `EngineFramework/IntegrationLayer/WorldIntegration/src/world_adapters.cpp:247`

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `framework.environment` (`EngineFramework/IntegrationLayer/WorldIntegration/src/save_participants.cpp:119`)

```cpp
117 std::unique_ptr<ISaveParticipant> CreateWorldFactsAdapterSaveParticipant(epidemic::gameplay::world_integration::WorldFactsAdapter& owner, SaveRestoreWorkspace& workspace)
  118 {
  119     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::world_integration::WorldFactsAdapter>>(owner, workspace, "integration.world_facts", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.world"), SaveParticipantId::FromString("framework.environment"), SaveParticipantId::FromString("framework.interaction"), SavePart…
```

#### `framework.interaction` (`EngineFramework/IntegrationLayer/WorldIntegration/src/save_participants.cpp:119`)

```cpp
117 std::unique_ptr<ISaveParticipant> CreateWorldFactsAdapterSaveParticipant(epidemic::gameplay::world_integration::WorldFactsAdapter& owner, SaveRestoreWorkspace& workspace)
  118 {
  119     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::world_integration::WorldFactsAdapter>>(owner, workspace, "integration.world_facts", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.world"), SaveParticipantId::FromString("framework.environment"), SaveParticipantId::FromString("framework.interaction"), SavePart…
```

#### `framework.world` (`EngineFramework/IntegrationLayer/WorldIntegration/src/save_participants.cpp:119`)

```cpp
117 std::unique_ptr<ISaveParticipant> CreateWorldFactsAdapterSaveParticipant(epidemic::gameplay::world_integration::WorldFactsAdapter& owner, SaveRestoreWorkspace& workspace)
  118 {
  119     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::world_integration::WorldFactsAdapter>>(owner, workspace, "integration.world_facts", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.world"), SaveParticipantId::FromString("framework.environment"), SaveParticipantId::FromString("framework.interaction"), SavePart…
```

#### `integration.world_facts` (`EngineFramework/IntegrationLayer/WorldIntegration/src/save_participants.cpp:119`)

```cpp
117 std::unique_ptr<ISaveParticipant> CreateWorldFactsAdapterSaveParticipant(epidemic::gameplay::world_integration::WorldFactsAdapter& owner, SaveRestoreWorkspace& workspace)
  118 {
  119     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::world_integration::WorldFactsAdapter>>(owner, workspace, "integration.world_facts", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.world"), SaveParticipantId::FromString("framework.environment"), SaveParticipantId::FromString("framework.interaction"), SavePart…
```

#### `framework.facts` (`EngineFramework/IntegrationLayer/WorldIntegration/src/save_participants.cpp:119`)

```cpp
117 std::unique_ptr<ISaveParticipant> CreateWorldFactsAdapterSaveParticipant(epidemic::gameplay::world_integration::WorldFactsAdapter& owner, SaveRestoreWorkspace& workspace)
  118 {
  119     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::world_integration::WorldFactsAdapter>>(owner, workspace, "integration.world_facts", std::vector<SaveParticipantId>{SaveParticipantId::FromString("framework.world"), SaveParticipantId::FromString("framework.environment"), SaveParticipantId::FromString("framework.interaction"), SavePart…
```

#### `framework.query.world.areas_at` (`EngineFramework/IntegrationLayer/WorldIntegration/src/world_adapters.cpp:53`)

```cpp
51     const queries::QueryProviderCapabilities caps{true, false, false, false};
   52     auto a = queries_.RegisterProvider<AreasAtPositionQuery>(
   53         "framework.query.world.areas_at", caps, [this](const AreasAtPositionQuery &q, const queries::QueryContext &) {
   54             auto v = world_.FindAreasAt(q.position);
   55             const auto n = v.size();
```

#### `framework.query.world.alterations` (`EngineFramework/IntegrationLayer/WorldIntegration/src/world_adapters.cpp:62`)

```cpp
60         return a;
   61     auto b = queries_.RegisterProvider<AlterationsInAreaQuery>(
   62         "framework.query.world.alterations", caps,
   63         [this](const AlterationsInAreaQuery &q, const queries::QueryContext &) {
   64             auto v = world_.FindAlterations(q.area, q.type);
```

#### `framework.query.environment.sample` (`EngineFramework/IntegrationLayer/WorldIntegration/src/world_adapters.cpp:72`)

```cpp
70         return b;
   71     auto c = queries_.RegisterProvider<EnvironmentSampleQuery>(
   72         "framework.query.environment.sample", caps,
   73         [this](const EnvironmentSampleQuery &q, const queries::QueryContext &) {
   74             // Cross-owner current reads are valid only inside the composition gameplay read phase.
```

#### `gameplay.world_integration.incoherent_read` (`EngineFramework/IntegrationLayer/WorldIntegration/src/world_adapters.cpp:97`)

```cpp
95             if (world_.CurrentRevision() != world_revision || environment_.CurrentRevision() != environment_revision)
   96                 return foundation::Result<queries::QueryResponse<EnvironmentSampleQuery::ResultType>>::Failure(
   97                     foundation::Error::Create("gameplay.world_integration.incoherent_read",
   98                                               "World or Environment changed during a cross-owner query"));
   99             const auto dependency_revision = CompositeRevision(world_revision, environment_rev…
```
