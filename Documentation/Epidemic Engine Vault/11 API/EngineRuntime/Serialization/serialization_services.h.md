---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/serialization_services.h"
---

# Serialization: serialization_services.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/serialization_services.h`.

**Модуль:** [[05 EngineRuntime/Модуль Serialization|Serialization]].
**Размер исходного файла:** 3051 байт.
**Подключение:** `#include <Epidemic/Runtime/Serialization/serialization_services.h>`.

## Объявления типов (лексический индекс)
`SerializationOptions`, `IArchiveFactory`, `SerializationServices`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Serialization/archive_reader.h"
#include "Epidemic/Runtime/Serialization/archive_writer.h"
#include "Epidemic/Runtime/Serialization/migration_registry.h"
#include "Epidemic/Runtime/Serialization/serializer_registry.h"

#include <exception>
#include <memory>
#include <new>
#include <typeindex>
#include <typeinfo>

namespace epidemic::runtime
{
struct SerializationOptions
{
};

class IArchiveFactory
{
  public:
    virtual ~IArchiveFactory() = default;

    [[nodiscard]] virtual std::unique_ptr<IArchiveWriter> CreateWriter() const = 0;
    [[nodiscard]] virtual foundation::Result<std::unique_ptr<IArchiveReader>> CreateReader(const SerializedDocument& document) const = 0;
};

struct SerializationServices
{
    std::shared_ptr<ISerializerRegistry> serializers;
    std::shared_ptr<IMigrationRegistry> migrations;
    std::shared_ptr<IArchiveFactory> archives;
};

[[nodiscard]] foundation::Result<SerializationServices> CreateSerializationServices(const SerializationOptions& options = {});

template <typename TObject>
[[nodiscard]] foundation::Result<SerializedDocument> SerializeToDocument(
    const ISerializer& serializer,
    const TObject& object,
    IArchiveFactory& archives)
{
    try
    {
        if (serializer.GetCppType() != std::type_index(typeid(TObject)))
        {
            return foundation::Result<SerializedDocument>::Failure(
                CreateSerializationError("serialization.cpp_type_mismatch", "serializer C++ type does not match source object"));
        }

        auto writer = archives.CreateWriter();
        if (!writer)
        {
            return foundation::Result<SerializedDocument>::Failure(
                CreateSerializationError("serialization.archive_factory_failed", "archive factory returned a null writer"));
        }
        auto serialized = serializer.Serialize(&object, *writer);
        if (!serialized)
        {
            return foundation::Result<SerializedDocument>::Failure(serialized.GetError());
        }
        return writer->Finalize(serializer.GetTypeId(), serializer.GetSchemaVersion());
    }
    catch (const std::bad_alloc&)
    {
        return foundation::Result<SerializedDocument>::Failure(
            CreateSerializationError("serialization.out_of_memory", "document serialization could not allocate required state"));
    }
    catch (const std::exception& error)
    {
        return foundation::Result<SerializedDocument>::Failure(
            CreateSerializationError("serialization.serializer.exception", "serializer or archive factory threw during document serialization", error.what()));
    }
    catch (...)
    {
        return foundation::Result<SerializedDocument>::Failure(CreateSerializationError(
            "serialization.serializer.exception", "serializer or archive factory threw an unknown exception during document serialization"));
    }
}
} // namespace epidemic::runtime
```
