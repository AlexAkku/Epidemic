---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["enginebase", "module", "input"]
source_path: "EngineBase/Input"
---

# EngineBase / Input

Модуль: **Input**. Raw input, keyboard/mouse snapshots.

**Реализация:** `EngineBase/Input/`.
**CMake targets:** `EpidemicInput`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineBase/Input/iinput_system.h|iinput_system.h]]: `EngineBase/Input/include/Epidemic/Input/iinput_system.h`; объявленные типы: `IInputSystem`
- [[11 API/EngineBase/Input/input_event.h|input_event.h]]: `EngineBase/Input/include/Epidemic/Input/input_event.h`; объявленные типы: `InputEventType`, `InputEvent`
- [[11 API/EngineBase/Input/input_snapshot.h|input_snapshot.h]]: `EngineBase/Input/include/Epidemic/Input/input_snapshot.h`; объявленные типы: `InputSnapshot`
- [[11 API/EngineBase/Input/input_system.h|input_system.h]]: `EngineBase/Input/include/Epidemic/Input/input_system.h`; объявленные типы: `InputSystem`
- [[11 API/EngineBase/Input/key_code.h|key_code.h]]: `EngineBase/Input/include/Epidemic/Input/key_code.h`; объявленные типы: `KeyCode`
- [[11 API/EngineBase/Input/keyboard_state.h|keyboard_state.h]]: `EngineBase/Input/include/Epidemic/Input/keyboard_state.h`; объявленные типы: `KeyboardState`
- [[11 API/EngineBase/Input/mouse_button.h|mouse_button.h]]: `EngineBase/Input/include/Epidemic/Input/mouse_button.h`; объявленные типы: `MouseButton`
- [[11 API/EngineBase/Input/mouse_state.h|mouse_state.h]]: `EngineBase/Input/include/Epidemic/Input/mouse_state.h`; объявленные типы: `MouseState`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineBase/modules/input|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Назначение

Input превращает поток platform events в согласованный immutable snapshot одного кадра. Он отделяет физический ввод от gameplay meaning.

#### Модель

`IInputSystem` принимает одиночные или пакетные `PlatformEvent`, затем `PublishSnapshot()` формирует `InputSnapshot`. Snapshot содержит `KeyboardState`, `MouseState` и sequence событий. Текущее состояние отвечает на вопрос, удерживается ли клавиша сейчас, а events описывают переходы внутри кадра.

`KeyCode` и `MouseButton` являются platform-neutral enums. `InputEvent` не содержит действий игрока, контекста UI или rebinding.

#### Порядок использования

Platform events должны быть переданы до `PublishSnapshot()`. Все consumers одного кадра читают один и тот же snapshot. `Reset()` используется при потере контекста или тестовой переинициализации, а не в конце каждого обычного кадра.

Gameplay layer преобразует snapshot в собственный command set. Например, одна и та же клавиша может означать движение персонажа, управление кораблем или навигацию меню в зависимости от активного контекста.

#### Инварианты и стабильность

Input не опрашивает gameplay systems и не вызывает их callbacks. Snapshot остается валиден до следующей публикации. События, поставленные в очередь после публикации, не меняют уже опубликованный snapshot и становятся видимыми только на следующем `PublishSnapshot()`.

Held state переносится между кадрами, transient press/release, mouse motion и wheel state очищаются в начале следующей публикации. Несколько переходов одной клавиши или кнопки в одном кадре сохраняются в `CurrentEvents()` в исходном порядке, при этом snapshot хранит итоговый held state и факт реально произошедших press/release transitions. Release для уже отпущенной клавиши или кнопки не создаёт ложный transition flag. Focus loss очищает held keyboard/mouse state и capture. `Reset()` очищает published state, очередь и events без синтетических gameplay transitions. Unknown и invalid key/button values игнорируются и не alias-ят валидный storage slot.

Public raw-input contract заморожен.

#### Карта публичных заголовков

Этот раздел служит быстрым индексом объявлений. Семантика и инварианты описаны выше; точные сигнатуры остаются источником истины в public headers.

- `iinput_system.h`: `IInputSystem`.
- `input_event.h`: `InputEventType`, `InputEvent`.
- `input_snapshot.h`: `InputSnapshot`.
- `input_system.h`: `InputSystem`.
- `key_code.h`: `KeyCode`.
- `keyboard_state.h`: `KeyboardState`.
- `mouse_button.h`: `MouseButton`.
- `mouse_state.h`: `MouseState`.

Источник контрактов: `Documentation/EngineBase/modules/input.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `MouseButton` (`EngineBase/Input/include/Epidemic/Input/mouse_button.h`): `Left`, `Right`, `Middle`, `X1`, `X2`, `Count`, `Unknown`.
- `KeyCode` (`EngineBase/Input/include/Epidemic/Input/key_code.h`): `Unknown`, `Backspace`, `Tab`, `Enter`, `Shift`, `Control`, `Alt`, `Pause`, `CapsLock`, `Escape`, `Space`, `PageUp`, `PageDown`, `End` и другие значения.
- `InputEventType` (`EngineBase/Input/include/Epidemic/Input/input_event.h`): `None`, `KeyPressed`, `KeyReleased`, `MouseMoved`, `MouseButtonPressed`, `MouseButtonReleased`, `MouseWheel`, `FocusChanged`, `CaptureChanged`.

### Операции и хранилища состояния

#### `InputSnapshot`

Объявлен в `EngineBase/Input/include/Epidemic/Input/input_snapshot.h:13`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
// Returns whether the owning window had focus when this snapshot was published. [[nodiscard]] bool HasFocus() const noexcept;
// Returns whether the owning window had mouse capture when this snapshot was published. [[nodiscard]] bool HasCapture() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `KeyboardState keyboard`; `MouseState mouse`.

#### `IInputSystem`

Объявлен в `EngineBase/Input/include/Epidemic/Input/iinput_system.h:15`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IInputSystem() = default;
```

#### `MouseState`

Объявлен в `EngineBase/Input/include/Epidemic/Input/mouse_state.h:16`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr std::size_t ButtonCount = static_cast<std::size_t>(MouseButton::Count);
// Returns the last known cursor X position. [[nodiscard]] std::int32_t PositionX() const noexcept;
// Returns the last known cursor Y position. [[nodiscard]] std::int32_t PositionY() const noexcept;
// Returns the accumulated X delta for the current frame. [[nodiscard]] std::int32_t DeltaX() const noexcept;
// Returns the accumulated Y delta for the current frame. [[nodiscard]] std::int32_t DeltaY() const noexcept;
// Returns the accumulated wheel delta for the current frame. [[nodiscard]] std::int32_t WheelDelta() const noexcept;
// Returns whether the specified button is currently held down. [[nodiscard]] bool IsButtonDown(MouseButton button) const noexcept;
// Returns whether the specified button was pressed during the current frame. [[nodiscard]] bool WasPressedThisFrame(MouseButton button) const noexcept;
// Returns whether the specified button was released during the current frame. [[nodiscard]] bool WasReleasedThisFrame(MouseButton button) const noexcept;
// Returns whether the owning window currently has focus. [[nodiscard]] bool HasFocus() const noexcept;
// Returns whether the owning window currently has mouse capture. [[nodiscard]] bool HasCapture() const noexcept;
// Updates the cursor position and accumulates per-frame deltas. void SetPosition(std::int32_t x, std::int32_t y) noexcept;
// Adds one wheel delta contribution to the current frame. void AddWheelDelta(std::int32_t wheel_delta) noexcept;
// Sets the persistent down/up state for one mouse button. void SetButtonDown(MouseButton button, bool down) noexcept;
// Marks a button as newly pressed for the current frame. void MarkPressed(MouseButton button) noexcept;
// Marks a button as newly released for the current frame. void MarkReleased(MouseButton button) noexcept;
// Updates cached focus state. void SetFocus(bool focused) noexcept;
// Updates cached capture state. void SetCapture(bool captured) noexcept;
```

#### `KeyboardState`

Объявлен в `EngineBase/Input/include/Epidemic/Input/keyboard_state.h:14`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
// Returns whether the key is currently held down. [[nodiscard]] bool IsKeyDown(KeyCode key_code) const noexcept;
// Returns whether the key transitioned to pressed during the current frame. [[nodiscard]] bool WasPressedThisFrame(KeyCode key_code) const noexcept;
// Returns whether the key transitioned to released during the current frame. [[nodiscard]] bool WasReleasedThisFrame(KeyCode key_code) const noexcept;
// Sets the persistent down/up state for a key. void SetKeyDown(KeyCode key_code, bool down) noexcept;
// Marks a key as newly pressed for the current frame. void MarkPressed(KeyCode key_code) noexcept;
// Marks a key as newly released for the current frame. void MarkReleased(KeyCode key_code) noexcept;
// Clears only the per-frame pressed/released flags. void ClearTransient() noexcept;
// Clears both persistent and transient key state. void ClearAll() noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `static constexpr std::size_t KeyCount = 256`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineBase/Input/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineBase/Input/src/input_system.cpp` (254 строк): `InputSystem::QueuePlatformEvent`, `InputSystem::QueuePlatformEvents`, `InputSystem::PublishSnapshot`, `InputSystem::Reset`, `InputSystem::CurrentSnapshot`, `InputSystem::CurrentEvents`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.

