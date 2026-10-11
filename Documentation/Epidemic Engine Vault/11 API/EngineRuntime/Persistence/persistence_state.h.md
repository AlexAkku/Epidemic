---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_state.h"
---

# Persistence: persistence_state.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_state.h`.

**Модуль:** [[05 EngineRuntime/Модуль Persistence|Persistence]].
**Размер исходного файла:** 565 байт.
**Подключение:** `#include <Epidemic/Runtime/Persistence/persistence_state.h>`.

## Объявления типов (лексический индекс)
`PersistenceState`, `PersistentObjectKind`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once


// Header for runtime contracts or module-local helpers. Comments document how each
// function participates in the module API and what state it observes or mutates.
namespace epidemic::runtime
{
enum class PersistenceState
{
    Clean,
    Dirty,
    PendingSave,
    Saving,
    Saved,
    LoadPending,
    Loaded,
    Deleted,
    Tombstoned,
    Conflict,
    Corrupted,
};

enum class PersistentObjectKind
{
    Unknown,
    Object,
    ContainerEntry,
    SurfaceState,
    ZoneOverride,
    AbstractFact,
};
}
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Перечисления и допустимые категории

- `PersistenceState`: `Clean`, `Dirty`, `PendingSave`, `Saving`, `Saved`, `LoadPending`, `Loaded`, `Deleted`, `Tombstoned`, `Conflict`, `Corrupted`
- `PersistentObjectKind`: `Unknown`, `Object`, `ContainerEntry`, `SurfaceState`, `ZoneOverride`, `AbstractFact`

### Состав типов и операции

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.

