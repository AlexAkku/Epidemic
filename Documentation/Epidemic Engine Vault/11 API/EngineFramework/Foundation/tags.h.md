---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/tags.h"
---

# Foundation: tags.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/tags.h`.

**Модуль:** [[06 EngineFramework/Модуль Foundation|Foundation]].
**Размер исходного файла:** 3019 байт.
**Подключение:** `#include <Epidemic/GameFramework/Foundation/tags.h>`.

## Объявления типов (лексический индекс)
`GameplayTagRegistry`, `Entry`, `GameplayTagSet`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/GameFramework/Foundation/ids.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace epidemic::gameplay
{
class GameplayTagRegistry
{
  public:
    struct Entry
    {
        TagId id{};
        TagId parent{};
        std::string canonical_name;
    };

    [[nodiscard]] foundation::Result<TagId> Register(std::string_view canonical_name);
    void Freeze() noexcept { frozen_ = true; }

    [[nodiscard]] bool IsFrozen() const noexcept { return frozen_; }
    [[nodiscard]] std::size_t Size() const noexcept { return entries_.size(); }
    [[nodiscard]] const Entry* Find(TagId id) const noexcept;
    [[nodiscard]] bool Matches(TagId candidate, TagId required) const noexcept;

  private:
    [[nodiscard]] foundation::Result<TagId> RegisterSingle(std::string_view canonical_name, TagId parent);

    std::unordered_map<TagId, Entry> entries_;
    bool frozen_ = false;
};

class GameplayTagSet
{
  public:
    void Add(TagId tag)
    {
        if (!tag.IsValid())
        {
            return;
        }
        const auto position = std::lower_bound(tags_.begin(), tags_.end(), tag);
        if (position == tags_.end() || *position != tag)
        {
            tags_.insert(position, tag);
        }
    }

    void Remove(TagId tag)
    {
        const auto position = std::lower_bound(tags_.begin(), tags_.end(), tag);
        if (position != tags_.end() && *position == tag)
        {
            tags_.erase(position);
        }
    }

    [[nodiscard]] bool HasExact(TagId tag) const noexcept
    {
        return std::binary_search(tags_.begin(), tags_.end(), tag);
    }

    [[nodiscard]] bool HasMatching(TagId required, const GameplayTagRegistry& registry) const noexcept
    {
        for (const auto tag : tags_)
        {
            if (registry.Matches(tag, required))
            {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] bool HasAll(const GameplayTagSet& required, const GameplayTagRegistry& registry) const noexcept
    {
        for (const auto tag : required.tags_)
        {
            if (!HasMatching(tag, registry))
            {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] bool HasAny(const GameplayTagSet& required, const GameplayTagRegistry& registry) const noexcept
    {
        for (const auto tag : required.tags_)
        {
            if (HasMatching(tag, registry))
            {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] const std::vector<TagId>& Values() const noexcept { return tags_; }

    [[nodiscard]] bool operator==(const GameplayTagSet&) const noexcept = default;

  private:
    std::vector<TagId> tags_;
};
} // namespace epidemic::gameplay
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/GameFramework/Foundation/ids.h`.

### Состав типов и операции

#### `GameplayTagRegistry` (строка заголовка 14)

Внутреннее состояние (не часть публичного API): `std::unordered_map<TagId, Entry> entries_`, `bool frozen_ = false`.

Публичные сигнатуры операций:

```cpp
[[nodiscard]] foundation::Result<TagId> Register(std::string_view canonical_name);
void Freeze() noexcept;
[[nodiscard]] bool IsFrozen() const noexcept;
[[nodiscard]] std::size_t Size() const noexcept;
[[nodiscard]] const Entry* Find(TagId id) const noexcept;
[[nodiscard]] bool Matches(TagId candidate, TagId required) const noexcept;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

#### `Entry` (строка заголовка 17)

Публичные поля и значения по умолчанию:

- `std::string canonical_name`

#### `GameplayTagSet` (строка заголовка 39)

Публичные сигнатуры операций:

```cpp
void Add(TagId tag);
void Remove(TagId tag);
[[nodiscard]] bool HasExact(TagId tag) const noexcept;
[[nodiscard]] bool HasMatching(TagId required, const GameplayTagRegistry& registry) const noexcept;
[[nodiscard]] bool HasAll(const GameplayTagSet& required, const GameplayTagRegistry& registry) const noexcept;
[[nodiscard]] bool HasAny(const GameplayTagSet& required, const GameplayTagRegistry& registry) const noexcept;
[[nodiscard]] const std::vector<TagId>& Values() const noexcept;
[[nodiscard]] bool operator==(const GameplayTagSet&) const noexcept = default;
```

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `Register` | `GameplayTagRegistry::Register` | `EngineFramework/BaseInfrastructure/Foundation/src/gameplay_foundation.cpp:32` |
| `Find` | `GameplayTagRegistry::Find` | `EngineFramework/BaseInfrastructure/Foundation/src/gameplay_foundation.cpp:115` |
| `Matches` | `GameplayTagRegistry::Matches` | `EngineFramework/BaseInfrastructure/Foundation/src/gameplay_foundation.cpp:121` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `Register`: `EngineFramework/DevelopmentInfrastructure/Tests/foundation_tests.cpp:269`
- `Size`: `EngineFramework/DevelopmentInfrastructure/Tests/foundation_tests.cpp:270`
- `Find`: `EngineFramework/DevelopmentInfrastructure/Tests/foundation_tests.cpp:303`
- `Freeze`: `EngineFramework/DevelopmentInfrastructure/Tests/foundation_tests.cpp:316`
- `Matches`: `EngineFramework/DevelopmentInfrastructure/Tests/foundation_tests.cpp:331`
- `Add`: `EngineFramework/DevelopmentInfrastructure/Tests/foundation_tests.cpp:340`
- `Values`: `EngineFramework/DevelopmentInfrastructure/Tests/foundation_tests.cpp:343`
- `HasAll`: `EngineFramework/DevelopmentInfrastructure/Tests/foundation_tests.cpp:346`
- `Remove`: `EngineFramework/DevelopmentInfrastructure/Tests/foundation_tests.cpp:347`

