---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["enginebase", "module", "r_h_i"]
source_path: "EngineBase/RHI"
---

# EngineBase / RHI

Модуль: **RHI**. Нейтральная RHI-абстракция.

**Реализация:** `EngineBase/RHI/`.
**CMake targets:** `EpidemicRHI`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineBase/RHI/descriptors.h|descriptors.h]]: `EngineBase/RHI/include/Epidemic/RHI/descriptors.h`; объявленные типы: `RhiColor`, `RhiClearDesc`, `RhiDeviceDesc`, `RhiSwapChainDesc`
- [[11 API/EngineBase/RHI/irhi_command_context.h|irhi_command_context.h]]: `EngineBase/RHI/include/Epidemic/RHI/irhi_command_context.h`; объявленные типы: `IRhiCommandContext`
- [[11 API/EngineBase/RHI/irhi_device.h|irhi_device.h]]: `EngineBase/RHI/include/Epidemic/RHI/irhi_device.h`; объявленные типы: `IRhiDevice`
- [[11 API/EngineBase/RHI/irhi_swap_chain.h|irhi_swap_chain.h]]: `EngineBase/RHI/include/Epidemic/RHI/irhi_swap_chain.h`; объявленные типы: `IRhiSwapChain`
- [[11 API/EngineBase/RHI/null_rhi_device.h|null_rhi_device.h]]: `EngineBase/RHI/include/Epidemic/RHI/null_rhi_device.h`
- [[11 API/EngineBase/RHI/pixel_format.h|pixel_format.h]]: `EngineBase/RHI/include/Epidemic/RHI/pixel_format.h`; объявленные типы: `RhiPixelFormat`
- [[11 API/EngineBase/RHI/presentation_surface_handle.h|presentation_surface_handle.h]]: `EngineBase/RHI/include/Epidemic/RHI/presentation_surface_handle.h`; объявленные типы: `PresentationSurfaceHandle`

## Углублённое чтение
- Отдельный исходный `.md` для этого модуля не найден. Читайте source headers и тесты.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Состояния и доменные перечисления

- `RhiPixelFormat` (`EngineBase/RHI/include/Epidemic/RHI/pixel_format.h`): `Unknown`, `R8G8B8A8_UNorm`, `B8G8R8A8_UNorm`.

### Операции и хранилища состояния

#### `IRhiCommandContext`

Объявлен в `EngineBase/RHI/include/Epidemic/RHI/irhi_command_context.h:11`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IRhiCommandContext() = default;
```

#### `IRhiDevice`

Объявлен в `EngineBase/RHI/include/Epidemic/RHI/irhi_device.h:16`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IRhiDevice() = default;
```

#### `IRhiSwapChain`

Объявлен в `EngineBase/RHI/include/Epidemic/RHI/irhi_swap_chain.h:11`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IRhiSwapChain() = default;
```

#### `PresentationSurfaceHandle`

Объявлен в `EngineBase/RHI/include/Epidemic/RHI/presentation_surface_handle.h:8`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
// Wraps a raw native presentation surface pointer. explicit constexpr PresentationSurfaceHandle(void *value) noexcept : value_(value);
// Returns the wrapped raw pointer value. [[nodiscard]] constexpr void *Value() const noexcept;
// Returns the wrapped raw pointer cast to the requested native handle type. template <typename T> [[nodiscard]] constexpr T As() const noexcept;
```

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineBase/RHI/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineBase/RHI/src/null_rhi_device.cpp` (296 строк): `Error::Create`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `rhi.invalid_swap_chain_size`: `EngineBase/RHI/src/null_rhi_device.cpp:236`
- `rhi.frame_already_active`: `EngineBase/RHI/src/null_rhi_device.cpp:102`
- `rhi.clear_outside_frame`: `EngineBase/RHI/src/null_rhi_device.cpp:116`
- `rhi.no_swap_chain`: `EngineBase/RHI/src/null_rhi_device.cpp:129`
- `rhi.no_active_frame`: `EngineBase/RHI/src/null_rhi_device.cpp:142`
- `rhi.empty_device_name`: `EngineBase/RHI/src/null_rhi_device.cpp:217`
- `rhi.invalid_surface_handle`: `EngineBase/RHI/src/null_rhi_device.cpp:229`
- `rhi.invalid_buffer_count`: `EngineBase/RHI/src/null_rhi_device.cpp:243`
- `rhi.invalid_color_format`: `EngineBase/RHI/src/null_rhi_device.cpp:255`
- `rhi.nothing_to_clear`: `EngineBase/RHI/src/null_rhi_device.cpp:268`
- `rhi.invalid_clear_color`: `EngineBase/RHI/src/null_rhi_device.cpp:276`

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `rhi.invalid_swap_chain_size` (`EngineBase/RHI/src/null_rhi_device.cpp:236`)

```cpp
234     {
  235         return epidemic::foundation::Result<void>::Failure(
  236             epidemic::foundation::Error::Create("rhi.invalid_swap_chain_size",
  237                                                 "Swap chain dimensions must be greater than zero"));
  238     }
```

#### `rhi.frame_already_active` (`EngineBase/RHI/src/null_rhi_device.cpp:102`)

```cpp
100         {
  101             return epidemic::foundation::Result<void>::Failure(
  102                 epidemic::foundation::Error::Create("rhi.frame_already_active",
  103                                                     "BeginFrame called while a frame is already active"));
  104         }
```

#### `rhi.clear_outside_frame` (`EngineBase/RHI/src/null_rhi_device.cpp:116`)

```cpp
114         {
  115             return epidemic::foundation::Result<void>::Failure(
  116                 epidemic::foundation::Error::Create("rhi.clear_outside_frame",
  117                                                     "Clear must be called between BeginFrame and EndFrame"));
  118         }
```

#### `rhi.no_swap_chain` (`EngineBase/RHI/src/null_rhi_device.cpp:129`)

```cpp
127         {
  128             return epidemic::foundation::Result<void>::Failure(
  129                 epidemic::foundation::Error::Create("rhi.no_swap_chain",
  130                                                     "RHI command context requires an active swap chain before Clear"));
  131         }
```

#### `rhi.no_active_frame` (`EngineBase/RHI/src/null_rhi_device.cpp:142`)

```cpp
140         {
  141             return epidemic::foundation::Result<void>::Failure(
  142                 epidemic::foundation::Error::Create("rhi.no_active_frame",
  143                                                     "EndFrame called without an active frame"));
  144         }
```

#### `rhi.empty_device_name` (`EngineBase/RHI/src/null_rhi_device.cpp:217`)

```cpp
215     {
  216         return epidemic::foundation::Result<void>::Failure(
  217             epidemic::foundation::Error::Create("rhi.empty_device_name", "RHI device debug name must not be empty"));
  218     }
  219
```

#### `rhi.invalid_surface_handle` (`EngineBase/RHI/src/null_rhi_device.cpp:229`)

```cpp
227     {
  228         return epidemic::foundation::Result<void>::Failure(
  229             epidemic::foundation::Error::Create("rhi.invalid_surface_handle",
  230                                                 "Swap chain surface handle must be valid"));
  231     }
```

#### `rhi.invalid_buffer_count` (`EngineBase/RHI/src/null_rhi_device.cpp:243`)

```cpp
241     {
  242         return epidemic::foundation::Result<void>::Failure(
  243             epidemic::foundation::Error::Create("rhi.invalid_buffer_count",
  244                                                 "Swap chain buffer count must be greater than zero"));
  245     }
```

#### `rhi.invalid_color_format` (`EngineBase/RHI/src/null_rhi_device.cpp:255`)

```cpp
253     default:
  254         return epidemic::foundation::Result<void>::Failure(
  255             epidemic::foundation::Error::Create("rhi.invalid_color_format",
  256                                                 "Swap chain color format must be a supported pixel format"));
  257     }
```
