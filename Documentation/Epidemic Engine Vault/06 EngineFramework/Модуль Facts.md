---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "facts"]
source_path: "EngineFramework/BaseInfrastructure/Facts"
---

# EngineFramework / Facts

Модуль: **Facts**. Gameplay facts.

**Реализация:** `EngineFramework/BaseInfrastructure/Facts/`.
**CMake targets:** `EpidemicGameplayFacts`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: B01 Goal 4 review. Local module contract only.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Facts/gameplay_facts.h|gameplay_facts.h]]: `EngineFramework/BaseInfrastructure/Facts/include/Epidemic/GameFramework/Facts/gameplay_facts.h`; объявленные типы: `HistoryPolicy`, `FactPersistence`, `EventEnvelope`, `EventRecord`, `FactKey`, `FactKeyHash`, `FactRecord`, `FactChangeKind`, `FactChange`, `HistoryQuery`, `EventDispatchLimits`, `FactsDiagnostics`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/modules/facts|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Состояния и доменные перечисления

- `HistoryPolicy` (`EngineFramework/BaseInfrastructure/Facts/include/Epidemic/GameFramework/Facts/gameplay_facts.h`): `None`, `Recent`, `Persistent`, `PersistentCompactable`.
- `FactPersistence` (`EngineFramework/BaseInfrastructure/Facts/include/Epidemic/GameFramework/Facts/gameplay_facts.h`): .
- `FactChangeKind` (`EngineFramework/BaseInfrastructure/Facts/include/Epidemic/GameFramework/Facts/gameplay_facts.h`): `Created`, `Updated`, `Removed`.
- `MutationKind` (`EngineFramework/BaseInfrastructure/Facts/include/Epidemic/GameFramework/Facts/gameplay_facts.h`): `Set`, `Remove`.

### Операции и хранилища состояния

#### `GameplayFactsService`

Объявлен в `EngineFramework/BaseInfrastructure/Facts/include/Epidemic/GameFramework/Facts/gameplay_facts.h:314`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
GameplayFactsService();
template <typename TPayload> [[nodiscard]] foundation::Result<EventTypeId> RegisterEventType( std::string_view canonical_name, GameplayDomainId owner, HistoryPolicy history_policy = HistoryPolicy::None, std::size_t recent_limit = 256);
template <typename TValue> [[nodiscard]] foundation::Result<FactTypeId> RegisterFactType(std::string_view canonical_name, GameplayDomainId owner);
template <typename TValue, typename TEncode, typename TDecode> [[nodiscard]] foundation::Result<void> RegisterFactCodec( FactTypeId type, std::string_view schema_name, std::uint32_t version, TEncode&& encode, TDecode&& decode);
template <typename TPayload, typename TEncode, typename TDecode> [[nodiscard]] foundation::Result<void> RegisterEventCodec( EventTypeId type, std::string_view schema_name, std::uint32_t version, TEncode&& encode, TDecode&& decode);
template <typename TPayload, typename TCallback> [[nodiscard]] foundation::Result<void> Subscribe(EventTypeId type, SubscriberId subscriber, int priority, TCallback&& callback);
void Freeze();
[[nodiscard]] bool IsFrozen() const noexcept;
[[nodiscard]] GameplayEventBatch CreateBatch(ProducerId producer, std::uint64_t batch_order) const;
[[nodiscard]] foundation::Result<void> SubmitBatch(GameplayEventBatch batch);
[[nodiscard]] FactTransaction BeginTransaction(GameplayDomainId owner) const;
[[nodiscard]] std::optional<FactRecord> FindFact(const FactKey& key) const;
[[nodiscard]] std::optional<FactRecord> FindFactCopy(const FactKey& key) const;
template <typename TValue> [[nodiscard]] std::optional<TValue> FindFactValueCopy(const FactKey& key) const;
[[nodiscard]] foundation::Result<std::uint64_t> ExpireDueFacts(GameplayTimePoint now, GameplayContext context);
[[nodiscard]] std::uint64_t HistoryCount() const noexcept;
[[nodiscard]] std::vector<EventRecord> FindHistory(const HistoryQuery& query) const;
[[nodiscard]] foundation::Result<void> CompactHistory(EventTypeId type, const IHistoryCompactor& compactor);
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `static constexpr std::string_view kFactChangedEventName = "framework.fact.changed"`; `static constexpr std::string_view kFactsDomainName = "framework.facts"`; `std::unordered_map<EventTypeId, EventTypeInfo> event_types_`; `std::unordered_map<FactTypeId, FactTypeInfo> fact_types_`; `std::unordered_map<EventTypeId, PayloadCodec> event_codecs_`; `std::unordered_map<FactTypeId, PayloadCodec> fact_codecs_`; `std::unordered_map<EventTypeId, std::vector<Subscriber>> subscribers_`; `std::unordered_map<FactKey, FactRecord, FactKeyHash> facts_`.

#### `IHistoryCompactor`

Объявлен в `EngineFramework/BaseInfrastructure/Facts/include/Epidemic/GameFramework/Facts/gameplay_facts.h:153`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IHistoryCompactor() = default;
[[nodiscard]] virtual std::vector<EventRecord> Compact(std::span<const EventRecord> records) const = 0;
```

#### `GameplayEventBatch`

Объявлен в `EngineFramework/BaseInfrastructure/Facts/include/Epidemic/GameFramework/Facts/gameplay_facts.h:160`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
GameplayEventBatch() = default;
GameplayEventBatch(ProducerId producer, std::uint64_t batch_order) : producer_(producer), batch_order_(batch_order);
template <typename TPayload> void Publish(EventTypeId type, GameplayContext context, GameplayObjectRef subject, TPayload payload);
template <typename TPayload> void Publish(EventTypeId type, GameplayContext context, GameplayObjectRef scope, GameplayObjectRef subject, TPayload payload);
[[nodiscard]] ProducerId Producer() const noexcept;
[[nodiscard]] std::uint64_t BatchOrder() const noexcept;
[[nodiscard]] bool Empty() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t batch_order_ = 0`; `std::uint64_t next_local_sequence_ = 0`; `std::vector<PendingEvent> pending_`.

#### `FactTransaction`

Объявлен в `EngineFramework/BaseInfrastructure/Facts/include/Epidemic/GameFramework/Facts/gameplay_facts.h:217`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
) : owner_(owner);
template <typename TValue> void Set(FactTypeId type, GameplayObjectRef subject, GameplayObjectRef scope, TValue value, FactPersistence persistence = FactPersistence::Session, std::optional<GameplayTimePoint> expires_at = std::nullopt);
void Remove(FactKey key);
[[nodiscard]] GameplayDomainId Owner() const noexcept;
[[nodiscard]] bool Empty() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::vector<Mutation> mutations_`.

#### `PayloadCodec`

Объявлен в `EngineFramework/BaseInfrastructure/Facts/include/Epidemic/GameFramework/Facts/gameplay_facts.h:613`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
std::function<foundation::Result<std::vector<std::byte>>(const std::any&)> encode;
std::function<foundation::Result<std::any>(std::span<const std::byte>, std::uint32_t)> decode;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint32_t version = 0`.

#### `FactKeyHash`

Объявлен в `EngineFramework/BaseInfrastructure/Facts/include/Epidemic/GameFramework/Facts/gameplay_facts.h:77`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] std::size_t operator()(const FactKey& key) const noexcept;
```

#### `Subscriber`

Объявлен в `EngineFramework/BaseInfrastructure/Facts/include/Epidemic/GameFramework/Facts/gameplay_facts.h:640`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
std::function<void(const EventRecord&)> callback;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `int priority = 0`.

#### `EventEnvelope`

Объявлен в `EngineFramework/BaseInfrastructure/Facts/include/Epidemic/GameFramework/Facts/gameplay_facts.h:47`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t sequence = 0`.

#### `EventRecord`

Объявлен в `EngineFramework/BaseInfrastructure/Facts/include/Epidemic/GameFramework/Facts/gameplay_facts.h:60`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::any payload`.

#### `FactRecord`

Объявлен в `EngineFramework/BaseInfrastructure/Facts/include/Epidemic/GameFramework/Facts/gameplay_facts.h:90`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `FactPersistence persistence = FactPersistence::Session`; `std::any value`.

#### `FactChange`

Объявлен в `EngineFramework/BaseInfrastructure/Facts/include/Epidemic/GameFramework/Facts/gameplay_facts.h:113`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `FactChangeKind kind = FactChangeKind::Created`; `std::optional<FactRecord> before`; `std::optional<FactRecord> after`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/BaseInfrastructure/Facts/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp` (1092 строк): `GameplayFactsService::GameplayFactsService`, `GameplayDomainId::FromString`, `GameplayFactsService::Freeze`, `GameplayFactsService::ValidateEventPayload`, `Error::Create`, `GameplayFactsService::SubmitBatch`, `GameplayFactsService::MergeSubmittedBatches`, `GameplayFactsService::Dispatch`, `GameplayFactsService::AppendHistoryTo`, `GameplayFactsService::ValidateMutation`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `gameplay.event_unknown`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:724`
- `gameplay.event_payload_mismatch`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:81`
- `gameplay.event_batch_invalid`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:91`
- `gameplay.event_batch_order_collision`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:121`
- `batch_merge.publish`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:181`
- `gameplay.event_dispatch_reentrant`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:192`
- `gameplay.event_dispatch_invalid_limits`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:197`
- `gameplay.event_budget_exceeded`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:225`
- `gameplay.event_id_exhausted`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:247`
- `gameplay.event_sequence_exhausted`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:235`
- `gameplay.event_wave_budget_exceeded`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:305`
- `gameplay.fact_unknown`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:340`
- `gameplay.fact_owner_violation`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:345`
- `gameplay.fact_invalid_subject`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:350`
- `gameplay.fact_value_mismatch`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:355`
- `gameplay.fact_missing_expiration`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:360`
- `gameplay.fact_unexpected_expiration`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:365`
- `gameplay.fact_invalid_owner`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:378`
- `gameplay.fact_duplicate_mutation`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:392`
- `gameplay.fact_revision_exhausted`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:582`
- `gameplay.fact_id_exhausted`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:452`
- `gameplay.event_order_exhausted`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:587`
- `transaction_commit.publish`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:499`
- `gameplay.history_snapshot_invalid`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:711`
- `gameplay.history_not_compactable`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:729`
- `gameplay.history_compactor_exception`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:754`
- `gameplay.history_compaction_invalid`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:761`
- `gameplay.facts_not_quiescent`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:787`
- `gameplay.snapshot_counter_invalid`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:898`
- `gameplay.fact_snapshot_invalid`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:863`
- `gameplay.restore_during_dispatch`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:908`
- `gameplay.fact_codec_missing`: `EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:959`

### Конкретные тестовые якоря

- `EngineFramework/DevelopmentInfrastructure/Tests/facts_tests.cpp`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `gameplay.event_unknown` (`EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:724`)

```cpp
722     if (info == event_types_.end())
  723     {
  724         return foundation::Result<void>::Failure(foundation::Error::Create("gameplay.event_unknown", "event type is not registered"));
  725     }
  726     if (info->second.history_policy != HistoryPolicy::PersistentCompactable)
```

#### `gameplay.event_payload_mismatch` (`EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:81`)

```cpp
79     {
   80         return foundation::Result<void>::Failure(
   81             foundation::Error::Create("gameplay.event_payload_mismatch", "event payload C++ type does not match registration"));
   82     }
   83     return foundation::Result<void>::Success();
```

#### `gameplay.event_batch_invalid` (`EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:91`)

```cpp
89     {
   90         return foundation::Result<void>::Failure(
   91             foundation::Error::Create("gameplay.event_batch_invalid", "worker event batch requires a stable producer id"));
   92     }
   93
```

#### `gameplay.event_batch_order_collision` (`EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:121`)

```cpp
119                     {
  120                         return foundation::Result<void>::Failure(foundation::Error::Create(
  121                             "gameplay.event_batch_order_collision", "worker batch ordering key already accepted in this wave"));
  122                     }
  123                 }
```

#### `batch_merge.publish` (`EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:181`)

```cpp
179         staged_pending.push_back(std::move(item.event));
  180 
  181     FactsPublicationFaultPointForInternalTest("batch_merge.publish");
  182     pending_events_.swap(staged_pending);
  183     submitted_batches_.clear();
```

#### `gameplay.event_dispatch_reentrant` (`EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:192`)

```cpp
190     {
  191         return foundation::Result<std::uint64_t>::Failure(
  192             foundation::Error::Create("gameplay.event_dispatch_reentrant", "event dispatch cannot be entered recursively"));
  193     }
  194     if (limits.max_events == 0 || limits.max_waves == 0)
```

#### `gameplay.event_dispatch_invalid_limits` (`EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:197`)

```cpp
195     {
  196         return foundation::Result<std::uint64_t>::Failure(
  197             foundation::Error::Create("gameplay.event_dispatch_invalid_limits", "dispatch limits must be non-zero"));
  198     }
  199
```

#### `gameplay.event_budget_exceeded` (`EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:225`)

```cpp
223             {
  224                 return foundation::Result<std::uint64_t>::Failure(
  225                     foundation::Error::Create("gameplay.event_budget_exceeded", "event dispatch exceeded max events budget"));
  226             }
  227             if (event_ids_.IsExhausted())
```

#### `gameplay.event_id_exhausted` (`EngineFramework/BaseInfrastructure/Facts/src/gameplay_facts.cpp:247`)

```cpp
245             {
  246                 return foundation::Result<std::uint64_t>::Failure(
  247                     foundation::Error::Create("gameplay.event_id_exhausted", "event id generator is exhausted"));
  248             }
  249             record.envelope.type = pending.type;
```
