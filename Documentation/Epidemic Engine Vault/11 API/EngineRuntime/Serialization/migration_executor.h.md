---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/migration_executor.h"
---

# Serialization: migration_executor.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/migration_executor.h`.

**Модуль:** [[05 EngineRuntime/Модуль Serialization|Serialization]].
**Размер исходного файла:** 518 байт.
**Подключение:** `#include <Epidemic/Runtime/Serialization/migration_executor.h>`.

## Объявления типов (лексический индекс)
`IArchiveFactory`, `IMigrationRegistry`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Serialization/archive_value.h"
#include "Epidemic/Runtime/Serialization/schema_version.h"

namespace epidemic::runtime
{
class IArchiveFactory;
class IMigrationRegistry;

[[nodiscard]] foundation::Result<SerializedDocument> ApplyMigrations(
    const SerializedDocument& document,
    SchemaVersion target,
    const IMigrationRegistry& migrations,
    const IArchiveFactory& archives);
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Serialization/archive_value.h`, `Epidemic/Runtime/Serialization/schema_version.h`.

Этот заголовок преимущественно содержит free-functions, type aliases или implementation helpers. Точные сигнатуры смотрите в исходном блоке выше.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.

