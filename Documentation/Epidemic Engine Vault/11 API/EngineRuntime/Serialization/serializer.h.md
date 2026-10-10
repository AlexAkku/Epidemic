---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/serializer.h"
---

# Serialization: serializer.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/serializer.h`.

**Модуль:** [[05 EngineRuntime/Модуль Serialization|Serialization]].
**Размер исходного файла:** 6801 байт.
**Подключение:** `#include <Epidemic/Runtime/Serialization/serializer.h>`.

## Объявления типов (лексический индекс)
`IArchiveFactory`, `ISerializer`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Foundation/string_id.h"
#include "Epidemic/Runtime/Serialization/archive_reader.h"
#include "Epidemic/Runtime/Serialization/archive_writer.h"
#include "Epidemic/Runtime/Serialization/schema_version.h"
#include "Epidemic/Runtime/Serialization/serialization_error.h"

#include <exception>
#include <new>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <utility>

namespace epidemic::runtime
{
class IArchiveFactory;
class ISerializer;

template <typename TObject>
[[nodiscard]] foundation::Result<void> SerializeObject(const ISerializer& serializer, const TObject& object, IArchiveWriter& writer);

template <typename TObject>
[[nodiscard]] foundation::Result<void> DeserializeObject(const ISerializer& serializer, IArchiveReader& reader, TObject& object);

template <typename TObject>
[[nodiscard]] foundation::Result<SerializedDocument> SerializeToDocument(
    const ISerializer& serializer,
    const TObject& object,
    IArchiveFactory& archives);

class ISerializer
{
  public:
    virtual ~ISerializer() = default;

    [[nodiscard]] virtual foundation::StringId GetTypeId() const = 0;
    [[nodiscard]] virtual SchemaVersion GetSchemaVersion() const = 0;
    [[nodiscard]] virtual std::type_index GetCppType() const = 0;

  protected:
    [[nodiscard]] virtual foundation::Result<void> Serialize(const void* object, IArchiveWriter& writer) const = 0;
    [[nodiscard]] virtual foundation::Result<void> Deserialize(IArchiveReader& reader, void* object) const = 0;

    template <typename TObject>
    friend foundation::Result<void> SerializeObject(const ISerializer& serializer, const TObject& object, IArchiveWriter& writer);

    template <typename TObject>
    friend foundation::Result<void> DeserializeObject(const ISerializer& serializer, IArchiveReader& reader, TObject& object);

    template <typename TObject>
    friend foundation::Result<SerializedDocument> SerializeToDocument(
        const ISerializer& serializer,
        const TObject& object,
        IArchiveFactory& archives);
};

template <typename TObject>
[[nodiscard]] foundation::Result<void> SerializeObject(const ISerializer& serializer, const TObject& object, IArchiveWriter& writer)
{
    // Low-level streaming helper. It is intentionally not rollback-capable for arbitrary
    // IArchiveWriter implementations; callers that need public failure atomicity should
    // use SerializeToDocument(), which owns a fresh writer and publishes nothing on failure.
    try
    {
        if (serializer.GetCppType() != std::type_index(typeid(TObject)))
        {
            return foundation::Result<void>::Failure(
                CreateSerializationError("serialization.cpp_type_mismatch", "serializer C++ type does not match source object"));
        }
        return serializer.Serialize(&object, writer);
    }
    catch (const std::bad_alloc&)
    {
        return foundation::Result<void>::Failure(
            CreateSerializationError("serialization.out_of_memory", "serialization could not allocate required staging state"));
    }
    catch (const std::exception& error)
    {
        return foundation::Result<void>::Failure(
            CreateSerializationError("serialization.serializer.exception", "serializer threw during serialization", error.what()));
    }
    catch (...)
    {
        return foundation::Result<void>::Failure(
            CreateSerializationError("serialization.serializer.exception", "serializer threw an unknown exception during serialization"));
    }
}

template <typename TObject>
[[nodiscard]] foundation::Result<void> DeserializeObject(const ISerializer& serializer, IArchiveReader& reader, TObject& object)
{
    try
    {
        if (serializer.GetCppType() != std::type_index(typeid(TObject)))
        {
            return foundation::Result<void>::Failure(
                CreateSerializationError("serialization.cpp_type_mismatch", "serializer C++ type does not match target object"));
        }

        const foundation::StringId serializer_type = serializer.GetTypeId();
        if (!serializer_type.IsValid())
        {
            return foundation::Result<void>::Failure(
                CreateSerializationError("serialization.serializer.invalid_type", "serializer must declare a valid type id"));
        }
        if (reader.GetTypeId() != serializer_type)
        {
            return foundation::Result<void>::Failure(
                CreateSerializationError("serialization.type_mismatch", "document type does not match serializer"));
        }

        const SchemaVersion serializer_schema = serializer.GetSchemaVersion();
        if (serializer_schema == SchemaVersion{})
        {
            return foundation::Result<void>::Failure(CreateSerializationError(
                "serialization.serializer.invalid_schema_version", "serializer must declare a non-zero schema version"));
        }
        if (reader.GetSchemaVersion() != serializer_schema)
        {
            return foundation::Result<void>::Failure(
                CreateSerializationError("serialization.schema_mismatch", "document schema version does not match serializer"));
        }

        if constexpr (std::is_copy_constructible_v<TObject> && std::is_nothrow_swappable_v<TObject>)
        {
            TObject candidate(object);
            auto result = serializer.Deserialize(reader, &candidate);
            if (!result)
            {
                return result;
            }
            using std::swap;
            swap(object, candidate);
            return foundation::Result<void>::Success();
        }
        else
        {
            return foundation::Result<void>::Failure(CreateSerializationError(
                "serialization.staging_unsupported",
                "destination type must be copy constructible and nothrow swappable for failure-atomic deserialization"));
        }
    }
    catch (const std::bad_alloc&)
    {
        return foundation::Result<void>::Failure(
            CreateSerializationError("serialization.out_of_memory", "deserialization could not allocate candidate state"));
    }
    catch (const std::exception& error)
    {
        return foundation::Result<void>::Failure(
            CreateSerializationError("serialization.serializer.exception", "serializer threw during deserialization", error.what()));
    }
    catch (...)
    {
        return foundation::Result<void>::Failure(
            CreateSerializationError("serialization.serializer.exception", "serializer threw an unknown exception during deserialization"));
    }
}

} // namespace epidemic::runtime
```
