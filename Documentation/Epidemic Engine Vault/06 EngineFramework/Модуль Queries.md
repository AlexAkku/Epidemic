---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "queries"]
source_path: "EngineFramework/BaseInfrastructure/Queries"
---

# EngineFramework / Queries

Модуль: **Queries**. Read-model queries.

**Реализация:** `EngineFramework/BaseInfrastructure/Queries/`.
**CMake targets:** `EpidemicGameplayQueries`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: B01 Goal 4 review. Local module contract only.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Queries/gameplay_queries.h|gameplay_queries.h]]: `EngineFramework/BaseInfrastructure/Queries/include/Epidemic/GameFramework/Queries/gameplay_queries.h`; объявленные типы: `QueryConsistency`, `QueryCoverage`, `QueryAccuracy`, `QueryRequirements`, `QueryBudget`, `QueryContext`, `QuerySnapshotReadEpoch`, `IQuerySnapshotCoordinator`, `QueryMetadata`, `QueryResponse`, `QueryProviderSnapshot`, `QueryProviderCapabilities`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/modules/queries|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Состояния и доменные перечисления

- `QueryConsistency` (`EngineFramework/BaseInfrastructure/Queries/include/Epidemic/GameFramework/Queries/gameplay_queries.h`): `Current`, `Snapshot`.
- `QueryCoverage` (`EngineFramework/BaseInfrastructure/Queries/include/Epidemic/GameFramework/Queries/gameplay_queries.h`): `Complete`, `Partial`, `Unavailable`.
- `QueryAccuracy` (`EngineFramework/BaseInfrastructure/Queries/include/Epidemic/GameFramework/Queries/gameplay_queries.h`): `Exact`, `Approximate`.

### Операции и хранилища состояния

#### `GameplayQueryService`

Объявлен в `EngineFramework/BaseInfrastructure/Queries/include/Epidemic/GameFramework/Queries/gameplay_queries.h:351`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
GameplayQueryService() = default;
explicit GameplayQueryService(std::uint64_t initial_query_id);
GameplayQueryService(const GameplayQueryService&) = delete;
GameplayQueryService(GameplayQueryService&&) noexcept = default;
template <typename TQuery> [[nodiscard]] foundation::Result<void> RegisterProvider( std::string_view canonical_name, QueryProviderCapabilities capabilities, typename detail::FunctionQueryProvider<TQuery>::CurrentHandler handler);
[[nodiscard]] foundation::Result<void> SetSnapshotCoordinator(std::shared_ptr<const IQuerySnapshotCoordinator> coordinator);
// After Freeze(), provider registry and coordinator configuration are immutable. // Execute() and AcquireSnapshot() may be called concurrently. Registered providers // and the snapshot coordinator are responsible for thread-safe concurrent reads. foundation::Result<void> Freeze();
[[nodiscard]] bool IsFrozen() const noexcept;
[[nodiscard]] std::size_t ProviderCount() const noexcept;
[[nodiscard]] QueryDiagnostics GetDiagnostics() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `bool frozen_ = false`.

#### `IQueryProvider`

Объявлен в `EngineFramework/BaseInfrastructure/Queries/include/Epidemic/GameFramework/Queries/gameplay_queries.h:185`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IQueryProvider() = default;
[[nodiscard]] virtual QueryTypeId Type() const noexcept = 0;
[[nodiscard]] virtual std::type_index QueryCppType() const noexcept = 0;
[[nodiscard]] virtual std::type_index ResultCppType() const noexcept = 0;
[[nodiscard]] virtual QueryProviderCapabilities Capabilities() const noexcept = 0;
[[nodiscard]] virtual foundation::Result<AnyQueryResponse> ExecuteCurrent(const std::any& query, const QueryContext& context) const = 0;
[[nodiscard]] virtual foundation::Result<AnyProviderSnapshot> CaptureSnapshot(const QueryContext& context) const = 0;
[[nodiscard]] virtual foundation::Result<AnyQueryResponse> ExecuteSnapshot( const std::any& query, const QueryContext& context, const AnyProviderSnapshot& snapshot) const = 0;
```

#### `IQuerySnapshotCoordinator`

Объявлен в `EngineFramework/BaseInfrastructure/Queries/include/Epidemic/GameFramework/Queries/gameplay_queries.h:80`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IQuerySnapshotCoordinator() = default;
```

#### `QueryServiceState`

Объявлен в `EngineFramework/BaseInfrastructure/Queries/include/Epidemic/GameFramework/Queries/gameplay_queries.h:337`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::unordered_map<QueryTypeId, std::shared_ptr<IQueryProvider>> providers`; `std::unordered_map<QueryTypeId, std::string> names`; `std::vector<QueryTypeId> provider_order`.

#### `FunctionQueryProvider`

Объявлен в `EngineFramework/BaseInfrastructure/Queries/include/Epidemic/GameFramework/Queries/gameplay_queries.h:201`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
) : type_(type), capabilities_(capabilities), current_handler_(std::move(current_handler)), snapshot_capture_(std::move(snapshot_capture)), snapshot_handler_(std::move(snapshot_handler));
[[nodiscard]] QueryTypeId Type() const noexcept override;
[[nodiscard]] std::type_index QueryCppType() const noexcept override;
[[nodiscard]] std::type_index ResultCppType() const noexcept override;
[[nodiscard]] QueryProviderCapabilities Capabilities() const noexcept override;
[[nodiscard]] foundation::Result<AnyQueryResponse> ExecuteCurrent(const std::any& query, const QueryContext& context) const override;
[[nodiscard]] foundation::Result<AnyProviderSnapshot> CaptureSnapshot(const QueryContext& context) const override;
[[nodiscard]] foundation::Result<AnyQueryResponse> ExecuteSnapshot( const std::any& query, const QueryContext& context, const AnyProviderSnapshot& snapshot) const override;
```

#### `QuerySnapshot`

Объявлен в `EngineFramework/BaseInfrastructure/Queries/include/Epidemic/GameFramework/Queries/gameplay_queries.h:150`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
QuerySnapshot() = default;
[[nodiscard]] GameplayTickId Tick() const noexcept;
[[nodiscard]] std::size_t ProviderCount() const noexcept;
```

#### `QueryBudget`

Объявлен в `EngineFramework/BaseInfrastructure/Queries/include/Epidemic/GameFramework/Queries/gameplay_queries.h:51`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] constexpr bool HasResultLimit() const noexcept;
[[nodiscard]] constexpr bool HasWorkLimit() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t max_work_units = 0`.

#### `QuerySnapshotReadEpoch`

Объявлен в `EngineFramework/BaseInfrastructure/Queries/include/Epidemic/GameFramework/Queries/gameplay_queries.h:72`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t value = 0`.

#### `QueryRequirements`

Объявлен в `EngineFramework/BaseInfrastructure/Queries/include/Epidemic/GameFramework/Queries/gameplay_queries.h:44`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `bool require_complete = true`; `bool allow_approximate = false`; `bool allow_unavailable = false`.

#### `QueryContext`

Объявлен в `EngineFramework/BaseInfrastructure/Queries/include/Epidemic/GameFramework/Queries/gameplay_queries.h:63`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `QueryConsistency consistency = QueryConsistency::Current`; `std::uint64_t snapshot_epoch = 0`.

#### `QueryMetadata`

Объявлен в `EngineFramework/BaseInfrastructure/Queries/include/Epidemic/GameFramework/Queries/gameplay_queries.h:91`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `QueryCoverage coverage = QueryCoverage::Complete`; `std::uint64_t result_count = 0`; `std::uint64_t work_units = 0`; `bool deterministic_order = true`; `QueryAccuracy accuracy = QueryAccuracy::Exact`; `bool resident_only = false`.

#### `QueryProviderCapabilities`

Объявлен в `EngineFramework/BaseInfrastructure/Queries/include/Epidemic/GameFramework/Queries/gameplay_queries.h:115`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `bool supports_current = true`; `bool supports_snapshot = false`; `bool can_return_partial = false`; `bool can_return_approximate = false`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/BaseInfrastructure/Queries/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/BaseInfrastructure/Queries/src/gameplay_queries.cpp` (114 строк): `GameplayQueryService::NextQueryId`, `QueryId::FromString`, `Error::Create`, `QueryId::FromRaw`, `GameplayQueryService::ValidateResponse`, `GameplayQueryService::RequirementsSatisfied`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `framework.gameplay_queries`: `EngineFramework/BaseInfrastructure/Queries/src/gameplay_queries.cpp:30`
- `gameplay.query_id_exhausted`: `EngineFramework/BaseInfrastructure/Queries/src/gameplay_queries.cpp:38`
- `gameplay.query_provider_contract_violation`: `EngineFramework/BaseInfrastructure/Queries/src/gameplay_queries.cpp:77`
- `gameplay.query_coverage_insufficient`: `EngineFramework/BaseInfrastructure/Queries/src/gameplay_queries.cpp:82`
- `gameplay.query_budget_exceeded`: `EngineFramework/BaseInfrastructure/Queries/src/gameplay_queries.cpp:88`

### Конкретные тестовые якоря

- `EngineFramework/DevelopmentInfrastructure/Tests/queries_tests.cpp`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `framework.gameplay_queries` (`EngineFramework/BaseInfrastructure/Queries/src/gameplay_queries.cpp:30`)

```cpp
28 foundation::Result<QueryId> GameplayQueryService::NextQueryId() const noexcept
   29 {
   30     constexpr auto kScope = QueryId::FromString("framework.gameplay_queries").High();
   31     auto current = state_->next_query_id.load(std::memory_order_relaxed);
   32     for (;;)
```

#### `gameplay.query_id_exhausted` (`EngineFramework/BaseInfrastructure/Queries/src/gameplay_queries.cpp:38`)

```cpp
36             state_->next_query_id.store(std::numeric_limits<std::uint64_t>::max(), std::memory_order_relaxed);
   37             return foundation::Result<QueryId>::Failure(
   38                 foundation::Error::Create("gameplay.query_id_exhausted", "query id generator is exhausted"));
   39         }
   40
```

#### `gameplay.query_provider_contract_violation` (`EngineFramework/BaseInfrastructure/Queries/src/gameplay_queries.cpp:77`)

```cpp
75     {
   76         return foundation::Result<void>::Failure(foundation::Error::Create(
   77             "gameplay.query_provider_contract_violation", "available query response must contain a value"));
   78     }
   79     if (!RequirementsSatisfied(response.metadata, context.requirements))
```

#### `gameplay.query_coverage_insufficient` (`EngineFramework/BaseInfrastructure/Queries/src/gameplay_queries.cpp:82`)

```cpp
80     {
   81         return foundation::Result<void>::Failure(foundation::Error::Create(
   82             "gameplay.query_coverage_insufficient", "query result does not satisfy request requirements"));
   83     }
   84     if ((context.budget.HasResultLimit() && response.metadata.result_count > context.budget.max_results) ||
```

#### `gameplay.query_budget_exceeded` (`EngineFramework/BaseInfrastructure/Queries/src/gameplay_queries.cpp:88`)

```cpp
86     {
   87         return foundation::Result<void>::Failure(foundation::Error::Create(
   88             "gameplay.query_budget_exceeded", "query provider exceeded declared query budget"));
   89     }
   90     return foundation::Result<void>::Success();
```
