---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "serialization"]
source_path: "EngineRuntime/Serialization"
---

# EngineRuntime / Serialization

Модуль: **Serialization**. Versioned wire format и serialization.

**Реализация:** `EngineRuntime/Serialization/`.
**CMake targets:** `EpidemicRuntimeSerialization`
**Локальные test targets:** `EpidemicRuntimeSerializationTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/Serialization/archive_reader.h|archive_reader.h]]: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/archive_reader.h`; объявленные типы: `IArchiveReader`
- [[11 API/EngineRuntime/Serialization/archive_value.h|archive_value.h]]: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/archive_value.h`; объявленные типы: `SerializedDocument`, `Impl`
- [[11 API/EngineRuntime/Serialization/archive_writer.h|archive_writer.h]]: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/archive_writer.h`; объявленные типы: `IArchiveWriter`
- [[11 API/EngineRuntime/Serialization/migration.h|migration.h]]: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/migration.h`; объявленные типы: `MigrationKey`, `IMigration`, `hash`
- [[11 API/EngineRuntime/Serialization/migration_executor.h|migration_executor.h]]: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/migration_executor.h`; объявленные типы: `IArchiveFactory`, `IMigrationRegistry`
- [[11 API/EngineRuntime/Serialization/migration_registry.h|migration_registry.h]]: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/migration_registry.h`; объявленные типы: `IMigrationRegistry`
- [[11 API/EngineRuntime/Serialization/schema_version.h|schema_version.h]]: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/schema_version.h`; объявленные типы: `SchemaVersion`
- [[11 API/EngineRuntime/Serialization/serialization_error.h|serialization_error.h]]: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/serialization_error.h`
- [[11 API/EngineRuntime/Serialization/serialization_services.h|serialization_services.h]]: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/serialization_services.h`; объявленные типы: `SerializationOptions`, `IArchiveFactory`, `SerializationServices`
- [[11 API/EngineRuntime/Serialization/serializer.h|serializer.h]]: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/serializer.h`; объявленные типы: `IArchiveFactory`, `ISerializer`
- [[11 API/EngineRuntime/Serialization/serializer_registry.h|serializer_registry.h]]: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/serializer_registry.h`; объявленные типы: `ISerializerRegistry`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/serialization|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Runtime и Support|Runtime Support]] как место technical cross-major adapters.

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Назначение

Serialization создает versioned immutable documents, регистрирует serializers и выполняет последовательные schema migrations. Он не решает, где хранится файл и какие world records должны сохраняться.

#### Модель

`IArchiveWriter` строит tree из scalar values, objects, arrays, bytes и null. После finalize получается opaque `SerializedDocument`. `IArchiveReader` читает его без доступа к внутренней variant implementation. `ISerializer` связывает C++ type и schema, `ISerializerRegistry` хранит serializers, а migration registry/executor переводит документ между версиями.

#### Инварианты

Document имеет valid type ID, поддерживаемый format version и non-null root. Duplicate object fields запрещены. Каждый migration step обязан сохранить type ID, вернуть точную целевую schema version и valid document. Migration chain должен быть непрерывным.

#### Граница и стабильность

Serialization не выполняет persistence transactions и не знает World. После Goal 3 и canonical evidence convergence модуль имеет статус `LOCAL_READY`; новые serializers и migrations регистрируются через существующие ports. Статус `FROZEN` присваивается только после общих Runtime freeze gates.

#### Карта публичных заголовков

Этот раздел служит быстрым индексом объявлений. Семантика и инварианты описаны выше; точные сигнатуры остаются источником истины в public headers.

- `archive_reader.h`: `IArchiveReader`.
- `archive_value.h`: `SerializedDocument`, `InMemoryArchiveReader`, `InMemoryArchiveWriter`, `InMemoryArchiveFactory`.
- `archive_writer.h`: `IArchiveWriter`.
- `migration.h`: `MigrationKey`, `IMigration`.
- `migration_executor.h`: `IArchiveFactory`, `IMigrationRegistry`.
- `migration_registry.h`: `IMigrationRegistry`.
- `schema_version.h`: `SchemaVersion`.
- `serialization_error.h`: factory-функции или backend implementation без отдельного публичного типа.
- `serialization_services.h`: `SerializationOptions`, `IArchiveFactory`, `SerializationServices`.
- `serializer.h`: `ISerializer`.
- `serializer_registry.h`: `ISerializerRegistry`.

Источник контрактов: `Documentation/EngineRuntime/modules/serialization.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Операции и хранилища состояния

#### `IArchiveReader`

Объявлен в `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/archive_reader.h:15`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IArchiveReader() = default;
[[nodiscard]] virtual foundation::StringId GetTypeId() const = 0;
[[nodiscard]] virtual SchemaVersion GetSchemaVersion() const = 0;
[[nodiscard]] virtual std::uint32_t GetFormatVersion() const = 0;
[[nodiscard]] virtual foundation::Result<void> BeginObject(std::string_view name) = 0;
[[nodiscard]] virtual foundation::Result<void> EndObject() = 0;
[[nodiscard]] virtual foundation::Result<std::size_t> BeginArray(std::string_view name) = 0;
[[nodiscard]] virtual foundation::Result<void> BeginArrayElement(std::size_t index) = 0;
[[nodiscard]] virtual foundation::Result<void> EndArrayElement() = 0;
[[nodiscard]] virtual foundation::Result<void> EndArray() = 0;
[[nodiscard]] virtual foundation::Result<std::string> ReadString(std::string_view name) const = 0;
[[nodiscard]] virtual foundation::Result<std::uint64_t> ReadUInt64(std::string_view name) const = 0;
[[nodiscard]] virtual foundation::Result<std::int64_t> ReadInt64(std::string_view name) const = 0;
[[nodiscard]] virtual foundation::Result<double> ReadDouble(std::string_view name) const = 0;
[[nodiscard]] virtual foundation::Result<bool> ReadBool(std::string_view name) const = 0;
[[nodiscard]] virtual foundation::Result<std::vector<std::byte>> ReadBytes(std::string_view name) const = 0;
[[nodiscard]] virtual foundation::Result<bool> IsNull(std::string_view name) const = 0;
```

#### `IArchiveWriter`

Объявлен в `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/archive_writer.h:13`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IArchiveWriter() = default;
[[nodiscard]] virtual foundation::Result<void> BeginObject(std::string_view name) = 0;
[[nodiscard]] virtual foundation::Result<void> EndObject() = 0;
[[nodiscard]] virtual foundation::Result<void> BeginArray(std::string_view name, std::size_t size) = 0;
[[nodiscard]] virtual foundation::Result<void> BeginArrayElement(std::size_t index) = 0;
[[nodiscard]] virtual foundation::Result<void> EndArrayElement() = 0;
[[nodiscard]] virtual foundation::Result<void> EndArray() = 0;
[[nodiscard]] virtual foundation::Result<void> WriteString(std::string_view name, std::string_view value) = 0;
[[nodiscard]] virtual foundation::Result<void> WriteUInt64(std::string_view name, std::uint64_t value) = 0;
[[nodiscard]] virtual foundation::Result<void> WriteInt64(std::string_view name, std::int64_t value) = 0;
[[nodiscard]] virtual foundation::Result<void> WriteDouble(std::string_view name, double value) = 0;
[[nodiscard]] virtual foundation::Result<void> WriteBool(std::string_view name, bool value) = 0;
[[nodiscard]] virtual foundation::Result<void> WriteBytes(std::string_view name, std::span<const std::byte> value) = 0;
[[nodiscard]] virtual foundation::Result<void> WriteNull(std::string_view name) = 0;
[[nodiscard]] virtual foundation::Result<SerializedDocument> Finalize(foundation::StringId type_id, SchemaVersion schema_version) = 0;
```

#### `ISerializer`

Объявлен в `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/serializer.h:34`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ISerializer() = default;
[[nodiscard]] virtual foundation::StringId GetTypeId() const = 0;
[[nodiscard]] virtual SchemaVersion GetSchemaVersion() const = 0;
[[nodiscard]] virtual std::type_index GetCppType() const = 0;
```

#### `IMigrationRegistry`

Объявлен в `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/migration_registry.h:13`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IMigrationRegistry() = default;
[[nodiscard]] virtual foundation::Result<void> RegisterMigration(std::shared_ptr<const IMigration> migration) = 0;
[[nodiscard]] virtual foundation::Result<void> Freeze() = 0;
[[nodiscard]] virtual bool IsFrozen() const noexcept = 0;
[[nodiscard]] virtual std::shared_ptr<const IMigration> FindMigration(const MigrationKey& key) const = 0;
[[nodiscard]] virtual bool HasMigration(const MigrationKey& key) const = 0;
[[nodiscard]] virtual foundation::Result<std::vector<std::shared_ptr<const IMigration>>> FindMigrationPath( foundation::StringId type_id, SchemaVersion from, SchemaVersion to) const = 0;
```

#### `ISerializerRegistry`

Объявлен в `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/serializer_registry.h:12`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ISerializerRegistry() = default;
[[nodiscard]] virtual foundation::Result<void> RegisterSerializer(std::shared_ptr<const ISerializer> serializer) = 0;
[[nodiscard]] virtual foundation::Result<void> Freeze() = 0;
[[nodiscard]] virtual bool IsFrozen() const noexcept = 0;
[[nodiscard]] virtual std::shared_ptr<const ISerializer> FindSerializer(foundation::StringId type_id) const = 0;
[[nodiscard]] virtual bool HasSerializer(foundation::StringId type_id) const = 0;
```

#### `IArchiveFactory`

Объявлен в `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/serialization_services.h:21`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IArchiveFactory() = default;
[[nodiscard]] virtual std::unique_ptr<IArchiveWriter> CreateWriter() const = 0;
[[nodiscard]] virtual foundation::Result<std::unique_ptr<IArchiveReader>> CreateReader(const SerializedDocument& document) const = 0;
```

#### `IMigration`

Объявлен в `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/migration.h:22`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IMigration() = default;
[[nodiscard]] virtual MigrationKey GetKey() const = 0;
[[nodiscard]] virtual foundation::Result<void> Apply(IArchiveReader& input, IArchiveWriter& output) const = 0;
```

#### `SerializationServices`

Объявлен в `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/serialization_services.h:30`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::shared_ptr<ISerializerRegistry> serializers`; `std::shared_ptr<IMigrationRegistry> migrations`; `std::shared_ptr<IArchiveFactory> archives`.

#### `SerializedDocument`

Объявлен в `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/archive_value.h:11`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
SerializedDocument() = default;
[[nodiscard]] foundation::StringId GetTypeId() const;
[[nodiscard]] SchemaVersion GetSchemaVersion() const;
[[nodiscard]] std::uint32_t GetFormatVersion() const;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `struct Impl`; `std::shared_ptr<const Impl> impl_`.

#### `SchemaVersion`

Объявлен в `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/schema_version.h:11`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint32_t major = 0`; `std::uint32_t minor = 0`; `std::uint32_t patch = 0`.

#### `hash`

Объявлен в `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/migration.h:34`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] size_t operator()(const epidemic::runtime::MigrationKey& key) const noexcept;
```

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineRuntime/Serialization/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicRuntimeFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineRuntime/Serialization/src/in_memory_archive.cpp` (571 строк): `InMemoryArchiveWriter::InMemoryArchiveWriter`, `InMemoryArchiveWriter::BeginObject`, `InMemoryArchiveWriter::EndObject`, `InMemoryArchiveWriter::BeginArray`, `InMemoryArchiveWriter::BeginArrayElement`, `InMemoryArchiveWriter::EndArrayElement`, `InMemoryArchiveWriter::EndArray`, `InMemoryArchiveWriter::WriteString`, `InMemoryArchiveWriter::WriteUInt64`, `InMemoryArchiveWriter::WriteInt64`
- `EngineRuntime/Serialization/src/serializer_registry.cpp` (94 строк): `SerializerRegistry::RegisterSerializer`, `SerializerRegistry::Freeze`, `SerializerRegistry::IsFrozen`, `SerializerRegistry::FindSerializer`, `SerializerRegistry::HasSerializer`
- `EngineRuntime/Serialization/src/migration_registry.cpp` (208 строк): `MigrationRegistry::RegisterMigration`, `MigrationRegistry::Freeze`, `MigrationRegistry::IsFrozen`, `MigrationRegistry::FindMigration`, `MigrationRegistry::HasMigration`, `MigrationRegistry::FindMigrationPath`
- `EngineRuntime/Serialization/src/serialization.cpp` (47 строк): `SerializedDocument::SerializedDocument`, `SerializedDocument::GetTypeId`, `SerializedDocument::GetSchemaVersion`, `SerializedDocument::GetFormatVersion`, `SerializedDocument::IsValid`
- `EngineRuntime/Serialization/src/migration_executor.cpp` (232 строк)

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `serialization.duplicate_field`: `EngineRuntime/Serialization/src/in_memory_archive.cpp:355`
- `serialization.out_of_memory`: `EngineRuntime/Serialization/src/migration_executor.cpp:198`
- `serialization.malformed`: `EngineRuntime/Serialization/src/in_memory_archive.cpp:543`
- `serialization.invalid_array_index`: `EngineRuntime/Serialization/src/in_memory_archive.cpp:473`
- `serialization.duplicate_array_element`: `EngineRuntime/Serialization/src/in_memory_archive.cpp:125`
- `serialization.array_incomplete`: `EngineRuntime/Serialization/src/in_memory_archive.cpp:345`
- `serialization.finalized`: `EngineRuntime/Serialization/src/in_memory_archive.cpp:275`
- `serialization.invalid_type`: `EngineRuntime/Serialization/src/in_memory_archive.cpp:228`
- `serialization.invalid_schema_version`: `EngineRuntime/Serialization/src/in_memory_archive.cpp:232`
- `serialization.invalid_name`: `EngineRuntime/Serialization/src/in_memory_archive.cpp:539`
- `serialization.type_mismatch`: `EngineRuntime/Serialization/src/in_memory_archive.cpp:508`
- `serialization.invalid_document`: `EngineRuntime/Serialization/src/migration_executor.cpp:60`
- `serialization.field_missing`: `EngineRuntime/Serialization/src/in_memory_archive.cpp:553`
- `serialization.registry_frozen`: `EngineRuntime/Serialization/src/migration_registry.cpp:36`
- `serialization.serializer.null`: `EngineRuntime/Serialization/src/serializer_registry.cpp:26`
- `serialization.serializer.invalid_type`: `EngineRuntime/Serialization/src/serializer_registry.cpp:34`
- `serialization.serializer.invalid_schema_version`: `EngineRuntime/Serialization/src/serializer_registry.cpp:38`
- `serialization.serializer.duplicate_type`: `EngineRuntime/Serialization/src/serializer_registry.cpp:44`
- `serialization.serializer.exception`: `EngineRuntime/Serialization/src/serializer_registry.cpp:65`
- `serialization.migration.null`: `EngineRuntime/Serialization/src/migration_executor.cpp:106`
- `serialization.migration.invalid_type`: `EngineRuntime/Serialization/src/migration_registry.cpp:117`
- `serialization.migration.invalid_version`: `EngineRuntime/Serialization/src/migration_executor.cpp:65`
- `serialization.migration.duplicate_key`: `EngineRuntime/Serialization/src/migration_registry.cpp:57`
- `serialization.migration.exception`: `EngineRuntime/Serialization/src/migration_executor.cpp:182`
- `serialization.migration.cycle`: `EngineRuntime/Serialization/src/migration_registry.cpp:197`
- `serialization.migration.path_missing`: `EngineRuntime/Serialization/src/migration_registry.cpp:199`
- `serialization.migration.ambiguous_path`: `EngineRuntime/Serialization/src/migration_registry.cpp:203`
- `serialization.migration.invalid_output`: `EngineRuntime/Serialization/src/migration_executor.cpp:31`
- `serialization.migration.type_changed`: `EngineRuntime/Serialization/src/migration_executor.cpp:36`
- `serialization.migration.version_mismatch`: `EngineRuntime/Serialization/src/migration_executor.cpp:41`
- `serialization.migration.invalid_format`: `EngineRuntime/Serialization/src/migration_executor.cpp:46`
- `serialization.migration_registry_exception`: `EngineRuntime/Serialization/src/migration_executor.cpp:92`

### Конкретные тестовые якоря

- `EngineRuntime/Serialization/tests/serialization_tests.cpp`: `TestArchiveStructuralAndReaderFailuresAreControlled`, `TestDeepEmptyStructuresBytesAndNullRoundTrip`, `TestFinalizeValidatesMetadataAndPreservesWriterOnFailure`, `TestArchiveAllocationFailureAtomicity`, `TestSerializerRegistryAndTypedBoundaryValidation`, `TestSerializerBodyExceptionsAreContained`, `TestDeserializeAllocationFailureAtomicity`, `TestApplyMigrationsAllocationFailureAtomicity`, `TestMigrationFailuresPreserveSourceAndContainExceptions`, `TestMigrationVersionValidationAndPathAllocationAtomicity`, `TestSerializeToDocumentAllocationFailureAtomicity`, `TestPrimitiveDocumentRoundTrip`, `TestDuplicateFieldsAreRejected`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `serialization.duplicate_field` (`EngineRuntime/Serialization/src/in_memory_archive.cpp:355`)

```cpp
353             if (!inserted)
  354             {
  355                 return Invalid("serialization.duplicate_field", "archive field is already written", name);
  356             }
  357         }
```

#### `serialization.out_of_memory` (`EngineRuntime/Serialization/src/migration_executor.cpp:198`)

```cpp
196             catch (const std::bad_alloc&)
  197             {
  198                 return MigrationExecutorFailure<SerializedDocument>("serialization.out_of_memory",
  199                                                                     "migration output finalization could not allocate state");
  200             }
```

#### `serialization.malformed` (`EngineRuntime/Serialization/src/in_memory_archive.cpp:543`)

```cpp
541     if (stack_.empty() || (stack_.back().kind != ContextKind::Object && stack_.back().kind != ContextKind::ArrayElement))
  542     {
  543         return InvalidValue<const ArchiveValue*>("serialization.malformed", "current archive context is not an object");
  544     }
  545     const ArchiveObject* current = CurrentObject();
```

#### `serialization.invalid_array_index` (`EngineRuntime/Serialization/src/in_memory_archive.cpp:473`)

```cpp
471     if (stack_.empty() || stack_.back().kind != ContextKind::Array || index >= stack_.back().array->elements.size())
  472     {
  473         return Invalid("serialization.invalid_array_index", "array element index is invalid");
  474     }
  475     const auto object = std::get_if<ArchiveObjectPtr>(&stack_.back().array->elements[index].storage);
```

#### `serialization.duplicate_array_element` (`EngineRuntime/Serialization/src/in_memory_archive.cpp:125`)

```cpp
123         if (stack_.back().array->initialized[index])
  124         {
  125             return Invalid("serialization.duplicate_array_element", "array element is already written");
  126         }
  127
```

#### `serialization.array_incomplete` (`EngineRuntime/Serialization/src/in_memory_archive.cpp:345`)

```cpp
343         if (HasUninitializedArrays(source))
  344         {
  345             return Invalid("serialization.array_incomplete", "transactional merge source contains uninitialized arrays");
  346         }
  347
```

#### `serialization.finalized` (`EngineRuntime/Serialization/src/in_memory_archive.cpp:275`)

```cpp
273     if (finalized_)
  274     {
  275         return Invalid("serialization.finalized", "archive writer is finalized");
  276     }
  277     return foundation::Result<void>::Success();
```

#### `serialization.invalid_type` (`EngineRuntime/Serialization/src/in_memory_archive.cpp:228`)

```cpp
226     if (!type_id.IsValid())
  227     {
  228         return InvalidValue<SerializedDocument>("serialization.invalid_type", "serialized document type id must be valid");
  229     }
  230     if (schema_version == SchemaVersion{})
```

#### `serialization.invalid_schema_version` (`EngineRuntime/Serialization/src/in_memory_archive.cpp:232`)

```cpp
230     if (schema_version == SchemaVersion{})
  231     {
  232         return InvalidValue<SerializedDocument>("serialization.invalid_schema_version",
  233                                                 "serialized document schema version must be non-zero");
  234     }
```
