---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_node.h"
---

# Scene: scene_node.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_node.h`.

**Модуль:** [[05 EngineRuntime/Модуль Scene|Scene]].
**Размер исходного файла:** 1351 байт.
**Подключение:** `#include <Epidemic/Runtime/Scene/scene_node.h>`.

## Объявления типов (лексический индекс)
`SceneNodeId`, `SceneNode`, `hash`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Scene/scene_state.h"

#include <cstdint>
#include <functional>

namespace epidemic::runtime
{
struct SceneNodeId
{
    std::uint64_t value = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value != 0;
    }

    [[nodiscard]] constexpr std::uint64_t Raw() const noexcept
    {
        return value;
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return IsValid();
    }

    [[nodiscard]] constexpr bool operator==(const SceneNodeId&) const noexcept = default;
};

struct SceneNode
{
    SceneNodeId id{};
    SceneNodeId parent_id{};
    SceneAttachmentState attachment_state = SceneAttachmentState::Detached;
    SceneMobility mobility = SceneMobility::Dynamic;
    SceneVisibilityState visibility = SceneVisibilityState::Visible;
    SceneDirtyMask dirty_flags = 0;
    std::uint64_t revision = 0;

    [[nodiscard]] constexpr bool operator==(const SceneNode&) const noexcept = default;
};
} // namespace epidemic::runtime

namespace std
{
template <> struct hash<epidemic::runtime::SceneNodeId>
{
    [[nodiscard]] size_t operator()(epidemic::runtime::SceneNodeId value) const noexcept
    {
        return hash<std::uint64_t>{}(value.Raw());
    }
};
} // namespace std
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Runtime/Scene/scene_state.h`.

### Состав типов и операции

#### `SceneNodeId` (строка заголовка 10)

Публичные поля и значения по умолчанию:

- `std::uint64_t value = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr std::uint64_t Raw() const noexcept;
[[nodiscard]] constexpr explicit operator bool() const noexcept;
[[nodiscard]] constexpr bool operator==(const SceneNodeId&) const noexcept = default;
```

#### `SceneNode` (строка заголовка 32)

Публичные поля и значения по умолчанию:

- `SceneAttachmentState attachment_state = SceneAttachmentState::Detached`
- `SceneMobility mobility = SceneMobility::Dynamic`
- `SceneVisibilityState visibility = SceneVisibilityState::Visible`
- `SceneDirtyMask dirty_flags = 0`
- `std::uint64_t revision = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const SceneNode&) const noexcept = default;
```

#### `hash` (строка заголовка 48)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] size_t operator()(epidemic::runtime::SceneNodeId value) const noexcept;
```

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

Первые наблюдаемые вызовы операций из тестовых исходников:

- `Raw`: `EngineRuntime/Scene/tests/scene_tests.cpp:529`

