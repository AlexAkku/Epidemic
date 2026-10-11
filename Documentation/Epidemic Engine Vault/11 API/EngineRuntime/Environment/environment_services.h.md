---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Environment/include/Epidemic/Runtime/Environment/environment_services.h"
---

# Environment: environment_services.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/environment_services.h`.

**Модуль:** [[05 EngineRuntime/Модуль Environment|Environment]].
**Размер исходного файла:** 622 байт.
**Подключение:** `#include <Epidemic/Runtime/Environment/environment_services.h>`.

## Объявления типов (лексический индекс)
`EnvironmentOptions`, `EnvironmentServices`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Environment/environment_runtime.h"

#include <memory>

namespace epidemic::runtime
{
struct EnvironmentOptions
{
    std::shared_ptr<const IEnvironmentUpdatePolicy> update_policy;
};

struct EnvironmentServices
{
    std::shared_ptr<IEnvironmentRuntime> runtime;
    std::shared_ptr<IEnvironmentQuery> query;
    std::shared_ptr<IEnvironmentWriter> writer;
};

[[nodiscard]] foundation::Result<EnvironmentServices> CreateEnvironmentServices(const EnvironmentOptions& options = {});
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Environment/environment_runtime.h`.

### Состав типов и операции

#### `EnvironmentOptions` (строка заголовка 10)

Публичные поля и значения по умолчанию:

- `std::shared_ptr<const IEnvironmentUpdatePolicy> update_policy`

#### `EnvironmentServices` (строка заголовка 15)

Публичные поля и значения по умолчанию:

- `std::shared_ptr<IEnvironmentRuntime> runtime`
- `std::shared_ptr<IEnvironmentQuery> query`
- `std::shared_ptr<IEnvironmentWriter> writer`

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- Для `std::shared_ptr` отдельно проверяйте циклы владения и контракт передачи внешних объектов.

