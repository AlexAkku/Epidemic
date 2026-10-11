---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_loader_registry.h"
---

# Resources: resource_loader_registry.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_loader_registry.h`.

**Модуль:** [[05 EngineRuntime/Модуль Resources|Resources]].
**Размер исходного файла:** 1048 байт.
**Подключение:** `#include <Epidemic/Runtime/Resources/resource_loader_registry.h>`.

## Объявления типов (лексический индекс)
`IResourceLoaderRegistry`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Resources/resource_loader.h"

namespace epidemic::runtime
{
// Threading: mutation must occur on the runtime thread.
// Concurrent reads/writes are not supported unless explicitly documented.
class IResourceLoaderRegistry
{
  public:
    virtual ~IResourceLoaderRegistry() = default;

    // The loader registry does not own loaders. Registered loaders must outlive the
    // registry and any resource manager that resolves loaders through it.
    [[nodiscard]] virtual foundation::Result<void> RegisterLoader(IResourceLoader& loader) = 0;
    [[nodiscard]] virtual IResourceLoader* FindLoader(ResourceType type) = 0;
    [[nodiscard]] virtual const IResourceLoader* FindLoader(ResourceType type) const = 0;
    [[nodiscard]] virtual bool HasLoader(ResourceType type) const = 0;
    [[nodiscard]] virtual foundation::Result<void> Freeze() = 0;
    [[nodiscard]] virtual bool IsFrozen() const noexcept = 0;
};
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Resources/resource_loader.h`.

### Состав типов и операции

#### `IResourceLoaderRegistry` (строка заголовка 10)

Публичные сигнатуры операций:

```cpp
virtual ~IResourceLoaderRegistry() = default;
[[nodiscard]] virtual IResourceLoader* FindLoader(ResourceType type) = 0;
[[nodiscard]] virtual const IResourceLoader* FindLoader(ResourceType type) const = 0;
[[nodiscard]] virtual bool HasLoader(ResourceType type) const = 0;
[[nodiscard]] virtual foundation::Result<void> Freeze() = 0;
[[nodiscard]] virtual bool IsFrozen() const noexcept = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- В заголовке задано ограничение потоков: Threading: mutation must occur on the runtime thread..

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `FindLoader` | `ResourceLoaderRegistry::FindLoader` | `EngineRuntime/Resources/src/resource_loader_registry.cpp:38` |
| `HasLoader` | `ResourceLoaderRegistry::HasLoader` | `EngineRuntime/Resources/src/resource_loader_registry.cpp:58` |
| `Freeze` | `ResourceLoaderRegistry::Freeze` | `EngineRuntime/Resources/src/resource_loader_registry.cpp:63` |
| `IsFrozen` | `ResourceLoaderRegistry::IsFrozen` | `EngineRuntime/Resources/src/resource_loader_registry.cpp:69` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `FindLoader`: `EngineRuntime/Resources/tests/resources_tests.cpp:294`
- `Freeze`: `EngineRuntime/Resources/tests/resources_tests.cpp:1091`

