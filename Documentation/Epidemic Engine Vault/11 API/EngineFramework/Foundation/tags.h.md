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
