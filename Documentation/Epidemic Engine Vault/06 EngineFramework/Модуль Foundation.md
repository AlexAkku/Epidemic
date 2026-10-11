---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "foundation"]
source_path: "EngineFramework/BaseInfrastructure/Foundation"
---

# EngineFramework / Foundation

Модуль: **Foundation**. Базовые типы, Result, errors и ID.

**Реализация:** `EngineFramework/BaseInfrastructure/Foundation/`.
**CMake targets:** `EpidemicGameFrameworkFoundation`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: B01 Goal 4 review. The module is qualified only for local readiness; this document does not claim whole-engine freeze.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Foundation/change_cursor.h|change_cursor.h]]: `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/change_cursor.h`; объявленные типы: `ChangeCursor`
- [[11 API/EngineFramework/Foundation/gameplay_context.h|gameplay_context.h]]: `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/gameplay_context.h`; объявленные типы: `GameplayContext`
- [[11 API/EngineFramework/Foundation/gameplay_foundation.h|gameplay_foundation.h]]: `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/gameplay_foundation.h`; объявленные типы: `SnapshotRestoreAccess`
- [[11 API/EngineFramework/Foundation/id_generator.h|id_generator.h]]: `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/id_generator.h`; объявленные типы: `MonotonicIdGenerator`, `Snapshot`, `MonotonicIdGeneratorSnapshotStatus`, `MonotonicIdGeneratorSnapshotValidation`
- [[11 API/EngineFramework/Foundation/ids.h|ids.h]]: `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/ids.h`; объявленные типы: `StableId128`, `StrongId128`, `StrongTypeId`, `GameplayObjectIdTag`, `OperationIdTag`, `CorrelationIdTag`, `QueryIdTag`, `EventIdTag`, `FactIdTag`, `ScheduleIdTag`, `GameplayDomainIdTag`, `TypeIdTag`…
- [[11 API/EngineFramework/Foundation/tags.h|tags.h]]: `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/tags.h`; объявленные типы: `GameplayTagRegistry`, `Entry`, `GameplayTagSet`
- [[11 API/EngineFramework/Foundation/time_types.h|time_types.h]]: `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/time_types.h`; объявленные типы: `GameplayDuration`, `GameplayTimePoint`, `GameplayTickId`, `Revision`, `GameplayTickContext`, `hash`
- [[11 API/EngineFramework/Foundation/type_registry.h|type_registry.h]]: `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/type_registry.h`; объявленные типы: `StableTypeRegistry`, `Entry`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/modules/foundation|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Состояния и доменные перечисления

- `MonotonicIdGeneratorSnapshotStatus` (`EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/id_generator.h`): `Valid`, `InvalidExpectedScope`, `InvalidSnapshotScope`, `UnexpectedScope`, `SequenceNotAheadOfRestoredIds`.

### Операции и хранилища состояния

#### `GameplayTagRegistry`

Объявлен в `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/tags.h:14`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] foundation::Result<TagId> Register(std::string_view canonical_name);
void Freeze() noexcept;
[[nodiscard]] bool IsFrozen() const noexcept;
[[nodiscard]] std::size_t Size() const noexcept;
[[nodiscard]] const Entry* Find(TagId id) const noexcept;
[[nodiscard]] bool Matches(TagId candidate, TagId required) const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::unordered_map<TagId, Entry> entries_`; `bool frozen_ = false`.

#### `StableTypeRegistry`

Объявлен в `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/type_registry.h:16`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] foundation::Result<TId> Register(std::string_view canonical_name);
[[nodiscard]] const Entry* Find(TId id) const noexcept;
[[nodiscard]] const Entry* Find(std::string_view canonical_name) const noexcept;
void Freeze() noexcept;
[[nodiscard]] bool IsFrozen() const noexcept;
[[nodiscard]] std::size_t Size() const noexcept;
[[nodiscard]] std::vector<Entry> Entries() const;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::unordered_map<std::uint64_t, Entry> entries_`; `bool frozen_ = false`.

#### `MonotonicIdGenerator`

Объявлен в `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/id_generator.h:14`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
explicit constexpr MonotonicIdGenerator(std::uint64_t scope) noexcept : scope_(NormalizeScope(scope));
explicit constexpr MonotonicIdGenerator(IdScopeId scope) noexcept : scope_(NormalizeScope(scope.Raw()));
[[nodiscard]] static constexpr MonotonicIdGenerator FromScopeName(std::string_view stable_scope_name) noexcept;
[[nodiscard]] TId Next() noexcept;
[[nodiscard]] constexpr bool IsExhausted() const noexcept;
[[nodiscard]] constexpr IdScopeId Scope() const noexcept;
[[nodiscard]] Snapshot GetSnapshot() const noexcept;
// Low-level structural check. State owners restoring persisted IDs should use // ValidateMonotonicIdGeneratorSnapshot instead, because only the owner knows // the expected persistent scope and the maximum restored low-part for its // records. [[nodiscard]] static constexpr bool IsValidSnapshot(Snapshot snapshot) noexcept;
[[nodiscard]] bool Restore(Snapshot snapshot) noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t scope_ = 1`; `std::uint64_t next_ = 1`.

#### `GameplayTagSet`

Объявлен в `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/tags.h:39`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
void Add(TagId tag);
void Remove(TagId tag);
[[nodiscard]] bool HasExact(TagId tag) const noexcept;
[[nodiscard]] bool HasMatching(TagId required, const GameplayTagRegistry& registry) const noexcept;
[[nodiscard]] bool HasAll(const GameplayTagSet& required, const GameplayTagRegistry& registry) const noexcept;
[[nodiscard]] bool HasAny(const GameplayTagSet& required, const GameplayTagRegistry& registry) const noexcept;
[[nodiscard]] const std::vector<TagId>& Values() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::vector<TagId> tags_`.

#### `StrongId128`

Объявлен в `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/ids.h:66`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] static constexpr StrongId128 FromRaw(std::uint64_t high, std::uint64_t low) noexcept;
[[nodiscard]] constexpr explicit operator bool() const noexcept;
[[nodiscard]] constexpr std::uint64_t High() const noexcept;
[[nodiscard]] constexpr std::uint64_t Low() const noexcept;
```

#### `StrongTypeId`

Объявлен в `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/ids.h:100`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] static constexpr StrongTypeId FromRaw(std::uint64_t raw) noexcept;
[[nodiscard]] constexpr std::uint64_t Raw() const noexcept;
[[nodiscard]] constexpr explicit operator bool() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t value = 0`.

#### `GameplayTickId`

Объявлен в `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/time_types.h:142`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] constexpr std::uint64_t Raw() const noexcept;
[[nodiscard]] constexpr explicit operator bool() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t value = 0`.

#### `GameplayObjectRef`

Объявлен в `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/ids.h:169`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] constexpr explicit operator bool() const noexcept;
```

#### `GameplayObjectPartRef`

Объявлен в `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/ids.h:183`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] constexpr explicit operator bool() const noexcept;
```

#### `ChangeCursor`

Объявлен в `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/change_cursor.h:10`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] constexpr ChangeCursor AtSequence(std::uint64_t value) const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t epoch = 0`; `std::uint64_t sequence = 0`.

#### `GameplayDuration`

Объявлен в `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/time_types.h:21`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] constexpr bool IsZero() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::int64_t ticks = 0`.

#### `Revision`

Объявлен в `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/time_types.h:153`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] constexpr std::uint64_t Raw() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t value = 0`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/BaseInfrastructure/Foundation/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/BaseInfrastructure/Foundation/src/gameplay_foundation.cpp` (149 строк): `GameplayTagRegistry::Register`, `Error::Create`, `TagId::FromString`, `GameplayTagRegistry::RegisterSingle`, `GameplayTagRegistry::Find`, `GameplayTagRegistry::Matches`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `gameplay.registry_frozen`: `EngineFramework/BaseInfrastructure/Foundation/src/gameplay_foundation.cpp:37`
- `gameplay.invalid_tag`: `EngineFramework/BaseInfrastructure/Foundation/src/gameplay_foundation.cpp:42`
- `gameplay.id_collision`: `EngineFramework/BaseInfrastructure/Foundation/src/gameplay_foundation.cpp:68`
- `gameplay.already_registered`: `EngineFramework/BaseInfrastructure/Foundation/src/gameplay_foundation.cpp:89`
- `tag_registry.publish`: `EngineFramework/BaseInfrastructure/Foundation/src/gameplay_foundation.cpp:103`

### Конкретные тестовые якоря

- `EngineFramework/DevelopmentInfrastructure/Tests/foundation_tests.cpp`: `TestFrameworkDocumentationRoot`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `gameplay.registry_frozen` (`EngineFramework/BaseInfrastructure/Foundation/src/gameplay_foundation.cpp:37`)

```cpp
35     {
   36         return foundation::Result<TagId>::Failure(
   37             foundation::Error::Create("gameplay.registry_frozen", "tag registry is frozen", std::string(canonical_name)));
   38     }
   39     if (!detail::IsCanonicalName(canonical_name))
```

#### `gameplay.invalid_tag` (`EngineFramework/BaseInfrastructure/Foundation/src/gameplay_foundation.cpp:42`)

```cpp
40     {
   41         return foundation::Result<TagId>::Failure(
   42             foundation::Error::Create("gameplay.invalid_tag", "tag name is invalid", std::string(canonical_name)));
   43     }
   44
```

#### `gameplay.id_collision` (`EngineFramework/BaseInfrastructure/Foundation/src/gameplay_foundation.cpp:68`)

```cpp
66             {
   67                 return foundation::Result<TagId>::Failure(
   68                     foundation::Error::Create("gameplay.id_collision", "tag id collision", std::string(prefix)));
   69             }
   70             parent = id;
```

#### `gameplay.already_registered` (`EngineFramework/BaseInfrastructure/Foundation/src/gameplay_foundation.cpp:89`)

```cpp
87     {
   88         return foundation::Result<TagId>::Failure(foundation::Error::Create(
   89             "gameplay.already_registered", "tag is already registered", std::string(canonical_name)));
   90     }
   91
```

#### `tag_registry.publish` (`EngineFramework/BaseInfrastructure/Foundation/src/gameplay_foundation.cpp:103`)

```cpp
101         }
  102     }
  103     FoundationPublicationFaultPointForInternalTest("tag_registry.publish");
  104     entries_.swap(staged_entries);
  105     return foundation::Result<TagId>::Success(pending.back().id);
```
