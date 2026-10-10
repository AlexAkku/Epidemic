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
