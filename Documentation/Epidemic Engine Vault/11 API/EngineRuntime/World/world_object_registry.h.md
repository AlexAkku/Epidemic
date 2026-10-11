---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/World/include/Epidemic/Runtime/World/world_object_registry.h"
---

# World: world_object_registry.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/World/include/Epidemic/Runtime/World/world_object_registry.h`.

**Модуль:** [[05 EngineRuntime/Модуль World|World]].
**Размер исходного файла:** 875 байт.
**Подключение:** `#include <Epidemic/Runtime/World/world_object_registry.h>`.

## Объявления типов (лексический индекс)
`IWorldObjectRegistry`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/World/world_commands.h"
#include "Epidemic/Runtime/World/world_object.h"

namespace epidemic::runtime
{
class IWorldObjectRegistry
{
  public:
    virtual ~IWorldObjectRegistry() = default;

    [[nodiscard]] virtual foundation::Result<WorldCommandResult> Apply(const CreateObjectCommand& command) = 0;
    [[nodiscard]] virtual foundation::Result<WorldCommandResult> Apply(const ChangePlacementCommand& command) = 0;
    [[nodiscard]] virtual foundation::Result<WorldCommandResult> Apply(const ChangeResidencyCommand& command) = 0;
    [[nodiscard]] virtual foundation::Result<WorldCommandResult> Apply(const PromotePersistenceTierCommand& command) = 0;
    [[nodiscard]] virtual foundation::Result<WorldCommandResult> Apply(const DestroyObjectCommand& command) = 0;
};
}
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/World/world_commands.h`, `Epidemic/Runtime/World/world_object.h`.

### Состав типов и операции

#### `IWorldObjectRegistry` (строка заголовка 9)

Публичные сигнатуры операций:

```cpp
virtual ~IWorldObjectRegistry() = default;
[[nodiscard]] virtual foundation::Result<WorldCommandResult> Apply(const CreateObjectCommand& command) = 0;
[[nodiscard]] virtual foundation::Result<WorldCommandResult> Apply(const ChangePlacementCommand& command) = 0;
[[nodiscard]] virtual foundation::Result<WorldCommandResult> Apply(const ChangeResidencyCommand& command) = 0;
[[nodiscard]] virtual foundation::Result<WorldCommandResult> Apply(const PromotePersistenceTierCommand& command) = 0;
[[nodiscard]] virtual foundation::Result<WorldCommandResult> Apply(const DestroyObjectCommand& command) = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `Apply` | `WorldRuntime::Apply` | `EngineRuntime/World/src/world_runtime_impl.cpp:254` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `Apply`: `EngineRuntime/World/tests/world_tests.cpp:304`

