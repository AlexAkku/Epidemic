---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_loader.h"
---

# Resources: resource_loader.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_loader.h`.

**Модуль:** [[05 EngineRuntime/Модуль Resources|Resources]].
**Размер исходного файла:** 826 байт.
**Подключение:** `#include <Epidemic/Runtime/Resources/resource_loader.h>`.

## Объявления типов (лексический индекс)
`ResourceLoadArtifact`, `IResourceLoader`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Resources/resource_dependency_graph.h"
#include "Epidemic/Runtime/Resources/resource_payload.h"
#include "Epidemic/Runtime/Resources/resource_request.h"
#include "Epidemic/Runtime/Resources/resource_type.h"

#include <vector>

namespace epidemic::runtime
{
struct ResourceLoadArtifact
{
    ResourceId resource_id{};
    ResourceType type{};
    ResourcePayloadPtr payload{};
    std::vector<ResourceDependency> dependencies;
};

class IResourceLoader
{
  public:
    virtual ~IResourceLoader() = default;

    [[nodiscard]] virtual ResourceType GetResourceType() const = 0;
    [[nodiscard]] virtual foundation::Result<ResourceLoadArtifact> Load(ResourceRequest request) = 0;
};
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Resources/resource_dependency_graph.h`, `Epidemic/Runtime/Resources/resource_payload.h`, `Epidemic/Runtime/Resources/resource_request.h`, `Epidemic/Runtime/Resources/resource_type.h`.

### Состав типов и операции

#### `ResourceLoadArtifact` (строка заголовка 13)

Публичные поля и значения по умолчанию:

- `std::vector<ResourceDependency> dependencies`

#### `IResourceLoader` (строка заголовка 21)

Публичные сигнатуры операций:

```cpp
virtual ~IResourceLoader() = default;
[[nodiscard]] virtual ResourceType GetResourceType() const = 0;
[[nodiscard]] virtual foundation::Result<ResourceLoadArtifact> Load(ResourceRequest request) = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

Первые наблюдаемые вызовы операций из тестовых исходников:

- `GetResourceType`: `EngineRuntime/Resources/tests/resources_tests.cpp:294`

