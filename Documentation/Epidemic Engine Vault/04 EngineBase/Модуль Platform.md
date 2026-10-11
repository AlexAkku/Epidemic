---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["enginebase", "module", "platform"]
source_path: "EngineBase/Platform"
---

# EngineBase / Platform

Модуль: **Platform**. Win32 runtime, окна и события.

**Реализация:** `EngineBase/Platform/`.
**CMake targets:** `EpidemicPlatform`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineBase/Platform/idynamic_library.h|idynamic_library.h]]: `EngineBase/Platform/include/Epidemic/Platform/idynamic_library.h`; объявленные типы: `IDynamicLibrary`
- [[11 API/EngineBase/Platform/iplatform_runtime.h|iplatform_runtime.h]]: `EngineBase/Platform/include/Epidemic/Platform/iplatform_runtime.h`; объявленные типы: `ProcessInfo`, `IPlatformRuntime`
- [[11 API/EngineBase/Platform/iwindow.h|iwindow.h]]: `EngineBase/Platform/include/Epidemic/Platform/iwindow.h`; объявленные типы: `IWindow`
- [[11 API/EngineBase/Platform/iwindow_system.h|iwindow_system.h]]: `EngineBase/Platform/include/Epidemic/Platform/iwindow_system.h`; объявленные типы: `WindowCreateInfo`, `IWindowSystem`
- [[11 API/EngineBase/Platform/native_window_handle.h|native_window_handle.h]]: `EngineBase/Platform/include/Epidemic/Platform/native_window_handle.h`; объявленные типы: `NativeWindowHandle`
- [[11 API/EngineBase/Platform/platform_event.h|platform_event.h]]: `EngineBase/Platform/include/Epidemic/Platform/platform_event.h`; объявленные типы: `PlatformEventType`, `PlatformEvent`
- [[11 API/EngineBase/Platform/windows_platform_runtime.h|windows_platform_runtime.h]]: `EngineBase/Platform/include/Epidemic/Platform/windows_platform_runtime.h`; объявленные типы: `WindowsPlatformRuntime`, `Impl`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineBase/modules/platform|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Назначение

Platform изолирует Win32 и предоставляет движку окна, системное время, process information, dynamic libraries и поток platform events. Остальные модули не включают Windows headers и не обрабатывают `HWND` напрямую.

#### Публичная модель

`IPlatformRuntime` описывает runtime платформы, high-resolution clock, event pump, exit request и загрузку dynamic library. `IWindowSystem` создает `IWindow` и выдает события. `NativeWindowHandle` переносит native handle через типобезопасную границу. `PlatformEvent` описывает resize, focus, keyboard, mouse и закрытие окна.

`WindowsPlatformRuntime` является официальной реализацией. Window lifetime выражается `shared_ptr<IWindow>`; закрытие native window отражается в объекте и event stream.

#### Использование

Composition root вызывает `RegisterWindowsRuntime()`, затем `CreateMainWindow()`. Каждый кадр platform phase вызывает `PumpEvents()` и передает drained events Input и другим consumers.

`LoadDynamicLibrary()` возвращает `Result`, а `IDynamicLibrary::FindSymbol()` не раскрывает конкретный Win32 module handle.

#### Инварианты и ограничения

Platform не знает о gameplay input actions, renderer, swap chain или UI. Он сообщает размер и native surface, но не выполняет graphics resize самостоятельно. Модуль Windows-only и считается стабильным.

#### Карта публичных заголовков

Этот раздел служит быстрым индексом объявлений. Семантика и инварианты описаны выше; точные сигнатуры остаются источником истины в public headers.

- `idynamic_library.h`: `IDynamicLibrary`.
- `iplatform_runtime.h`: `ProcessInfo`, `IPlatformRuntime`.
- `iwindow.h`: `IWindow`.
- `iwindow_system.h`: `WindowCreateInfo`, `IWindowSystem`.
- `native_window_handle.h`: `NativeWindowHandle`.
- `platform_event.h`: `PlatformEventType`, `PlatformEvent`.
- `windows_platform_runtime.h`: `WindowsPlatformRuntime`.

Источник контрактов: `Documentation/EngineBase/modules/platform.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `PlatformEventType` (`EngineBase/Platform/include/Epidemic/Platform/platform_event.h`): `None`, `WindowCloseRequested`, `WindowResized`, `WindowFocusChanged`, `WindowMinimized`, `WindowRestored`, `KeyPressed`, `KeyReleased`, `MouseMoved`, `MouseButtonPressed`, `MouseButtonReleased`, `MouseWheel`, `MouseCaptureChanged`.

### Операции и хранилища состояния

#### `IWindow`

Объявлен в `EngineBase/Platform/include/Epidemic/Platform/iwindow.h:13`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IWindow() = default;
```

#### `IDynamicLibrary`

Объявлен в `EngineBase/Platform/include/Epidemic/Platform/idynamic_library.h:13`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IDynamicLibrary() = default;
```

#### `IWindowSystem`

Объявлен в `EngineBase/Platform/include/Epidemic/Platform/iwindow_system.h:26`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IWindowSystem() = default;
```

#### `IPlatformRuntime`

Объявлен в `EngineBase/Platform/include/Epidemic/Platform/iplatform_runtime.h:33`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IPlatformRuntime() = default;
```

#### `WindowsPlatformRuntime`

Объявлен в `EngineBase/Platform/include/Epidemic/Platform/windows_platform_runtime.h:14`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `struct Impl`; `std::unique_ptr<Impl> impl_`.

#### `NativeWindowHandle`

Объявлен в `EngineBase/Platform/include/Epidemic/Platform/native_window_handle.h:8`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
// Wraps a raw native window pointer value. explicit constexpr NativeWindowHandle(void *value) noexcept : value_(value);
// Returns the wrapped raw pointer value. [[nodiscard]] constexpr void *Value() const noexcept;
// Returns the wrapped raw pointer cast to the requested native handle type. template <typename T> [[nodiscard]] constexpr T As() const noexcept;
```

#### `WindowCreateInfo`

Объявлен в `EngineBase/Platform/include/Epidemic/Platform/iwindow_system.h:16`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::string title = "Epidemic Engine v1.0"`; `std::uint32_t client_width = 1280`; `std::uint32_t client_height = 720`; `bool visible = true`.

#### `ProcessInfo`

Объявлен в `EngineBase/Platform/include/Epidemic/Platform/iplatform_runtime.h:26`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `epidemic::foundation::Path executable_path`; `epidemic::foundation::Path working_directory`; `std::vector<std::string> arguments`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineBase/Platform/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineBase/Platform/src/Windows/windows_platform_runtime.cpp` (1490 строк): `Error::Create`, `WindowsPlatformRuntime::WindowsPlatformRuntime`, `WindowsPlatformRuntime::Shutdown`, `WindowsPlatformRuntime::Name`, `WindowsPlatformRuntime::GetProcessInfo`, `WindowsPlatformRuntime::Now`, `Clock::now`, `WindowsPlatformRuntime::LoadDynamicLibrary`, `WindowsPlatformRuntime::PumpEvents`, `WindowsPlatformRuntime::IsExitRequested`
- `EngineBase/Platform/src/Windows/windows_file_package.cpp` (123 строк): `Error::Create`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `platform.empty_symbol_name`: `EngineBase/Platform/src/Windows/windows_platform_runtime.cpp:300`
- `platform.symbol_not_found`: `EngineBase/Platform/src/Windows/windows_platform_runtime.cpp:310`
- `platform.runtime_shutdown`: `EngineBase/Platform/src/Windows/windows_platform_runtime.cpp:1402`
- `platform.exit_requested`: `EngineBase/Platform/src/Windows/windows_platform_runtime.cpp:1062`
- `platform.invalid_window_size`: `EngineBase/Platform/src/Windows/windows_platform_runtime.cpp:1068`
- `platform.window_id_exhausted`: `EngineBase/Platform/src/Windows/windows_platform_runtime.cpp:1075`
- `platform.create_window_failed`: `EngineBase/Platform/src/Windows/windows_platform_runtime.cpp:1101`
- `platform.register_window_class_failed`: `EngineBase/Platform/src/Windows/windows_platform_runtime.cpp:1180`
- `platform.empty_library_path`: `EngineBase/Platform/src/Windows/windows_platform_runtime.cpp:1408`
- `platform.relative_library_path`: `EngineBase/Platform/src/Windows/windows_platform_runtime.cpp:1428`
- `platform.load_library_failed`: `EngineBase/Platform/src/Windows/windows_platform_runtime.cpp:1439`
- `platform.file.`: `EngineBase/Platform/src/Windows/windows_file_package.cpp:35`
- `platform.file.exception`: `EngineBase/Platform/src/Windows/windows_file_package.cpp:121`

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `platform.empty_symbol_name` (`EngineBase/Platform/src/Windows/windows_platform_runtime.cpp:300`)

```cpp
298         {
  299             return epidemic::foundation::Result<void *>::Failure(
  300                 epidemic::foundation::Error::Create("platform.empty_symbol_name", "Dynamic library symbol name must not be empty"));
  301         }
  302
```

#### `platform.symbol_not_found` (`EngineBase/Platform/src/Windows/windows_platform_runtime.cpp:310`)

```cpp
308             return epidemic::foundation::Result<void *>::Failure(
  309                 epidemic::foundation::Error::Create(
  310                     "platform.symbol_not_found",
  311                     "Failed to resolve symbol '" + std::string(symbol_name) + "' in library '" + name_ + "' (" +
  312                         FormatWindowsErrorMessage(error_code) + ")"));
```

#### `platform.runtime_shutdown` (`EngineBase/Platform/src/Windows/windows_platform_runtime.cpp:1402`)

```cpp
1400     {
 1401         return epidemic::foundation::Result<DynamicLibraryPtr>::Failure(
 1402             epidemic::foundation::Error::Create("platform.runtime_shutdown", "Platform runtime is shut down"));
 1403     }
 1404
```

#### `platform.exit_requested` (`EngineBase/Platform/src/Windows/windows_platform_runtime.cpp:1062`)

```cpp
1060         {
 1061             return epidemic::foundation::Result<WindowPtr>::Failure(
 1062                 epidemic::foundation::Error::Create("platform.exit_requested", "Platform runtime exit is already requested"));
 1063         }
 1064
```

#### `platform.invalid_window_size` (`EngineBase/Platform/src/Windows/windows_platform_runtime.cpp:1068`)

```cpp
1066         {
 1067             return epidemic::foundation::Result<WindowPtr>::Failure(
 1068                 epidemic::foundation::Error::Create("platform.invalid_window_size",
 1069                                                     "Window client size must be greater than zero"));
 1070         }
```

#### `platform.window_id_exhausted` (`EngineBase/Platform/src/Windows/windows_platform_runtime.cpp:1075`)

```cpp
1073         {
 1074             return epidemic::foundation::Result<WindowPtr>::Failure(
 1075                 epidemic::foundation::Error::Create("platform.window_id_exhausted", "Platform window id space is exhausted"));
 1076         }
 1077
```

#### `platform.create_window_failed` (`EngineBase/Platform/src/Windows/windows_platform_runtime.cpp:1101`)

```cpp
1099             return epidemic::foundation::Result<WindowPtr>::Failure(
 1100                 epidemic::foundation::Error::Create(
 1101                     "platform.create_window_failed",
 1102                     "Failed to create Win32 window '" + title + "' (" + FormatWindowsErrorMessage(error_code) + ")"));
 1103         }
```

#### `platform.register_window_class_failed` (`EngineBase/Platform/src/Windows/windows_platform_runtime.cpp:1180`)

```cpp
1178             return epidemic::foundation::Result<int>::Failure(
 1179                 epidemic::foundation::Error::Create(
 1180                     "platform.register_window_class_failed",
 1181                     "Failed to register Win32 window class (" + FormatWindowsErrorMessage(error_code) + ")"));
 1182         }
```

#### `platform.empty_library_path` (`EngineBase/Platform/src/Windows/windows_platform_runtime.cpp:1408`)

```cpp
1406     {
 1407         return epidemic::foundation::Result<DynamicLibraryPtr>::Failure(
 1408             epidemic::foundation::Error::Create("platform.empty_library_path", "Dynamic library path must not be empty"));
 1409     }
 1410
```
