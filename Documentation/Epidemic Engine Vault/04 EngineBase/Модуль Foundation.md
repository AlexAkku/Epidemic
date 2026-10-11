---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["enginebase", "module", "foundation"]
source_path: "EngineBase/Foundation"
---

# EngineBase / Foundation

Модуль: **Foundation**. Базовые типы, Result, errors и ID.

**Реализация:** `EngineBase/Foundation/`.
**CMake targets:** `EpidemicFoundation`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineBase/Foundation/error.h|error.h]]: `EngineBase/Foundation/include/Epidemic/Foundation/error.h`; объявленные типы: `Error`
- [[11 API/EngineBase/Foundation/handle.h|handle.h]]: `EngineBase/Foundation/include/Epidemic/Foundation/handle.h`; объявленные типы: `Handle`, `hash`
- [[11 API/EngineBase/Foundation/path.h|path.h]]: `EngineBase/Foundation/include/Epidemic/Foundation/path.h`; объявленные типы: `Path`
- [[11 API/EngineBase/Foundation/result.h|result.h]]: `EngineBase/Foundation/include/Epidemic/Foundation/result.h`; объявленные типы: `Result`, `Storage`
- [[11 API/EngineBase/Foundation/string_id.h|string_id.h]]: `EngineBase/Foundation/include/Epidemic/Foundation/string_id.h`; объявленные типы: `BasicId`, `StringIdTag`, `NameIdTag`, `ModuleIdTag`, `ServiceIdTag`, `EventTypeIdTag`, `hash`
- [[11 API/EngineBase/Foundation/time.h|time.h]]: `EngineBase/Foundation/include/Epidemic/Foundation/time.h`; объявленные типы: `FrameTime`, `FrameIndex`, `hash`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineBase/modules/foundation|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Назначение

Foundation — самый нижний модуль EngineBase. Он предоставляет маленькие типы, на которых строятся остальные библиотеки, и не зависит от других частей движка. Сюда попадает только то, что одинаково необходимо Core, Platform, Memory, RHI и будущему Runtime.

#### Модель

Ошибки передаются через `Error` и `Result<T>`. Идентификаторы и handles являются строгими типами, чтобы разные пространства ID нельзя было случайно смешать. `Path` нормализует работу с путями без превращения Foundation в файловую систему. `FrameTime` и `FrameIndex` описывают базовые значения кадра.

#### Инварианты

Invalid ID и handle имеют явно определенное нулевое состояние. Сравнение и hashing должны быть детерминированными. Foundation не выполняет I/O, не создает threads и не владеет engine services.

#### Использование

Верхние модули объявляют собственные tag-типы и строят `BasicId<Tag>` или `Handle<Id>`, а не передают необозначенные целые числа. Ошибки получают стабильные machine-readable codes, чтобы tests и diagnostics не зависели от текста сообщения.

#### Ограничения и стабильность

Foundation не является общей свалкой helpers. Контейнеры, math subsystem, filesystem, serialization и gameplay utilities должны жить в своих слоях. Публичные value types заморожены; расширения допустимы только без изменения существующей семантики.

#### Карта публичных заголовков

Этот раздел служит быстрым индексом объявлений. Семантика и инварианты описаны выше; точные сигнатуры остаются источником истины в public headers.

- `error.h`: `Error`.
- `handle.h`: `Handle`.
- `path.h`: `Path`.
- `result.h`: `Result`.
- `string_id.h`: `BasicId`, `StringIdTag`, `NameIdTag`, `ModuleIdTag`, `ServiceIdTag`, `EventTypeIdTag`.
- `time.h`: `FrameTime`, `FrameIndex`.

Источник контрактов: `Documentation/EngineBase/modules/foundation.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Операции и хранилища состояния

#### `Path`

Объявлен в `EngineBase/Foundation/include/Epidemic/Foundation/path.h:15`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
// Builds a path from an already materialized std::filesystem::path. // Embedded NUL characters are rejected because downstream native filesystem APIs // would otherwise observe a truncated path. explicit Path(std::filesystem::path path) : path_(ValidateNative(std::move(path)));
// Returns the underlying native path object. [[nodiscard]] const std::filesystem::path &Native() const noexcept;
// Returns true when no path value is stored. [[nodiscard]] bool Empty() const noexcept;
// Returns a generic forward-slash string form. // Relationship: useful for diagnostics and platform-neutral comparisons. [[nodiscard]] std::string GenericString() const;
// Returns a lexically normalized copy of the path. [[nodiscard]] Path LexicallyNormal() const;
// Appends a relative child path and returns the normalized result. // Rooted or absolute children are rejected so they cannot replace the base path. [[nodiscard]] Path Join(std::string_view child) const;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::filesystem::path path_`.

#### `Result`

Объявлен в `EngineBase/Foundation/include/Epidemic/Foundation/result.h:18`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
// Wraps a success payload into Result<TValue>. // Input: owned success value. // Output: successful result instance. [[nodiscard]] static Result Success(TValue value);
// Wraps an Error into Result<TValue>. // Input: owned failure payload. // Output: failed result instance. [[nodiscard]] static Result Failure(Error error);
// Returns true when the result currently stores TValue. [[nodiscard]] bool HasValue() const noexcept;
// Returns a mutable reference to the success value. // Throws if the result currently stores an Error. [[nodiscard]] TValue &Value() &;
// Returns a read-only reference to the success value. // Throws if the result currently stores an Error. [[nodiscard]] const TValue &Value() const &;
// Moves the success value out of the result. // Relationship: this overload is important for move-only or large payload types. [[nodiscard]] TValue &&Value() &&;
// Returns a mutable reference to the stored Error. // Throws if the result currently stores a success value. [[nodiscard]] Error &GetError() &;
// Returns a read-only reference to the stored Error. // Throws if the result currently stores a success value. [[nodiscard]] const Error &GetError() const &;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `Storage storage_`.

#### `Storage`

Объявлен в `EngineBase/Foundation/include/Epidemic/Foundation/result.h:117`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
explicit Storage(TValue value) : value_(std::in_place_type<TValue>, std::move(value));
explicit Storage(Error error) : value_(std::in_place_type<Error>, std::move(error));
Storage(const Storage &) = default;
Storage(Storage &&) noexcept(std::is_nothrow_move_constructible_v<Variant>) = default;
[[nodiscard]] Variant &Get() noexcept;
[[nodiscard]] const Variant &Get() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `Variant value_`.

#### `FrameTime`

Объявлен в `EngineBase/Foundation/include/Epidemic/Foundation/time.h:24`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
// Wraps an engine-native Duration into FrameTime. [[nodiscard]] static constexpr FrameTime FromDuration(Duration duration) noexcept;
// Converts any std::chrono duration into the engine-native duration domain. // Non-finite floating inputs and values outside Duration's representable range are rejected. template <typename Rep, typename Period> [[nodiscard]] static constexpr FrameTime FromChrono(std::chrono::duration<Rep, Period> duration);
// Builds a FrameTime from seconds. [[nodiscard]] static FrameTime FromSeconds(double seconds);
// Builds a FrameTime from milliseconds. [[nodiscard]] static FrameTime FromMilliseconds(double milliseconds);
// Returns the underlying engine-native duration. [[nodiscard]] constexpr Duration Raw() const noexcept;
// Returns the duration in seconds as a floating-point value. [[nodiscard]] double Seconds() const noexcept;
// Returns the duration in milliseconds as a floating-point value. [[nodiscard]] double Milliseconds() const noexcept;
// Returns true when the wrapped duration is zero. [[nodiscard]] constexpr bool IsZero() const noexcept;
// Explicit conversion back to the raw duration type. [[nodiscard]] constexpr explicit operator Duration() const noexcept;
```

#### `Storage`

Объявлен в `EngineBase/Foundation/include/Epidemic/Foundation/result.h:233`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
Storage(bool has_value, Error error) : has_value_(has_value), error_(std::move(error));
Storage(const Storage &) = default;
Storage(Storage &&) noexcept = default;
[[nodiscard]] bool HasValue() const noexcept;
[[nodiscard]] const Error &GetError() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `Error error_`.

#### `Result`

Объявлен в `EngineBase/Foundation/include/Epidemic/Foundation/result.h:181`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
// Builds a successful Result<void>. [[nodiscard]] static Result Success();
// Builds a failed Result<void>. [[nodiscard]] static Result Failure(Error error);
// Returns true when the represented operation succeeded. [[nodiscard]] bool HasValue() const noexcept;
// Validates that the result is successful. // Throws if the represented operation failed. // Relationship: mirrors Value() from the generic specialization. void Value() const;
// Returns the stored failure payload. // Throws if the represented operation actually succeeded. [[nodiscard]] const Error &GetError() const;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `Storage storage_`.

#### `Handle`

Объявлен в `EngineBase/Foundation/include/Epidemic/Foundation/handle.h:15`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr std::uint32_t kInvalidIndex = std::numeric_limits<std::uint32_t>::max();
// Builds a handle from an explicit slot index and generation pair. // An invalid index always canonicalizes to generation 0. constexpr Handle(std::uint32_t index, std::uint32_t generation) noexcept : index_(index), generation_(index == kInvalidIndex ? 0u : generation);
// Returns the slot index portion of the handle. [[nodiscard]] constexpr std::uint32_t Index() const noexcept;
// Returns the generation used to detect recycled slots. [[nodiscard]] constexpr std::uint32_t Generation() const noexcept;
// Packs generation and index into a single 64-bit value. // Relationship: used by hashing and quick identity comparisons. [[nodiscard]] constexpr std::uint64_t Packed() const noexcept;
```

#### `FrameIndex`

Объявлен в `EngineBase/Foundation/include/Epidemic/Foundation/time.h:138`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
// Builds a frame index from an explicit raw value. explicit constexpr FrameIndex(std::uint64_t value) noexcept : value_(value);
// Returns the raw frame number. [[nodiscard]] constexpr std::uint64_t Value() const noexcept;
// Returns the next frame index without mutating the current instance. // Throws when UINT64_MAX has no representable successor. [[nodiscard]] constexpr FrameIndex Next() const;
// Prefix increment advances the frame index in place. // Throws without modifying the value when UINT64_MAX is reached. constexpr FrameIndex &operator++();
// Postfix increment returns the previous value and then advances the index. // Throws without modifying the value when UINT64_MAX is reached. constexpr FrameIndex operator++(int);
```

#### `Error`

Объявлен в `EngineBase/Foundation/include/Epidemic/Foundation/error.h:17`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
// Checks whether the stored code matches the requested code exactly. // Input: expected code string. // Output: true when the codes are equal. [[nodiscard]] bool HasCode(std::string_view expected_code) const noexcept;
// Reports whether the error contains a non-empty message. [[nodiscard]] bool HasMessage() const noexcept;
// Reports whether the error contains extra contextual details. [[nodiscard]] bool HasContext() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::string code`; `std::string message`; `std::string context`.

#### `BasicId`

Объявлен в `EngineBase/Foundation/include/Epidemic/Foundation/string_id.h:32`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
// Returns the raw numeric id value. [[nodiscard]] constexpr std::uint64_t Raw() const noexcept;
```

#### `hash`

Объявлен в `EngineBase/Foundation/include/Epidemic/Foundation/handle.h:66`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
// Hashes the packed handle identity so Handle<TTag> can be used in unordered containers. [[nodiscard]] size_t operator()(const epidemic::foundation::Handle<TTag> &value) const noexcept;
```

#### `hash`

Объявлен в `EngineBase/Foundation/include/Epidemic/Foundation/time.h:197`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
// Hashes the raw frame index value for unordered container support. [[nodiscard]] size_t operator()(const epidemic::foundation::FrameIndex &value) const noexcept;
```

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineBase/Foundation/CMakeLists.txt`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Конкретные тестовые якоря

- `EngineFramework/DevelopmentInfrastructure/Tests/foundation_tests.cpp`: `TestFrameworkDocumentationRoot`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.

