---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_payload.h"
---

# Resources: resource_payload.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_payload.h`.

**Модуль:** [[05 EngineRuntime/Модуль Resources|Resources]].
**Размер исходного файла:** 868 байт.
**Подключение:** `#include <Epidemic/Runtime/Resources/resource_payload.h>`.

## Объявления типов (лексический индекс)
`IResourcePayload`, `ByteResourcePayload`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace epidemic::runtime
{
class IResourcePayload
{
  public:
    virtual ~IResourcePayload() = default;

    [[nodiscard]] virtual std::size_t GetSizeBytes() const noexcept = 0;
};

class ByteResourcePayload final : public IResourcePayload
{
  public:
    explicit ByteResourcePayload(std::vector<std::byte> bytes = {}) : bytes_(std::move(bytes))
    {
    }

    [[nodiscard]] std::size_t GetSizeBytes() const noexcept override
    {
        return bytes_.size();
    }

    [[nodiscard]] const std::vector<std::byte>& Bytes() const noexcept
    {
        return bytes_;
    }

  private:
    std::vector<std::byte> bytes_;
};

using ResourcePayloadPtr = std::shared_ptr<const IResourcePayload>;
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Состав типов и операции

#### `IResourcePayload` (строка заголовка 10)

Публичные сигнатуры операций:

```cpp
virtual ~IResourcePayload() = default;
[[nodiscard]] virtual std::size_t GetSizeBytes() const noexcept = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `ByteResourcePayload` (строка заголовка 18)

Публичные сигнатуры операций:

```cpp
) : bytes_(std::move(bytes));
[[nodiscard]] std::size_t GetSizeBytes() const noexcept override;
[[nodiscard]] const std::vector<std::byte>& Bytes() const noexcept;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- Для `std::shared_ptr` отдельно проверяйте циклы владения и контракт передачи внешних объектов.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

Первые наблюдаемые вызовы операций из тестовых исходников:

- `GetSizeBytes`: `EngineRuntime/Resources/tests/resources_tests.cpp:1305`

