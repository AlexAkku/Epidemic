---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["enginebase", "module", "r_h_i__d3_d11"]
source_path: "EngineBase/RHI_D3D11"
---

# EngineBase / RHI_D3D11

Модуль: **RHI_D3D11**. D3D11 backend и device integration.

**Реализация:** `EngineBase/RHI_D3D11/`.
**CMake targets:** `EpidemicRHI_D3D11`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineBase/RHI_D3D11/d3d11_rhi_device.h|d3d11_rhi_device.h]]: `EngineBase/RHI_D3D11/include/Epidemic/RHI_D3D11/d3d11_rhi_device.h`

## Углублённое чтение
- Отдельный исходный `.md` для этого модуля не найден. Читайте source headers и тесты.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineBase/RHI_D3D11/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicFoundation`, `EpidemicPlatform`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp` (791 строк): `Error::Create`, `ID3D11Device::QueryInterface`, `IDXGIAdapter::GetParent`, `D3D11RhiSwapChain::Create`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `rhi.invalid_color_format`: `EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:113`
- `rhi.d3d11.device_lost`: `EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:141`
- `rhi.d3d11.swap_chain_missing`: `EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:267`
- `rhi.d3d11.recreate_required`: `EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:470`
- `rhi.d3d11.present_failed`: `EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:241`
- `rhi.invalid_swap_chain_size`: `EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:259`
- `rhi.d3d11.resize_failed`: `EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:290`
- `rhi.invalid_surface_handle`: `EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:411`
- `rhi.d3d11.get_dxgi_device_failed`: `EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:421`
- `rhi.d3d11.get_adapter_failed`: `EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:430`
- `rhi.d3d11.get_factory_failed`: `EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:438`
- `rhi.d3d11.create_swap_chain_failed`: `EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:457`
- `rhi.d3d11.get_back_buffer_failed`: `EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:485`
- `rhi.d3d11.create_render_target_failed`: `EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:500`
- `rhi.frame_already_active`: `EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:555`
- `rhi.clear_outside_frame`: `EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:569`
- `rhi.nothing_to_clear`: `EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:576`
- `rhi.invalid_clear_color`: `EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:584`
- `rhi.d3d11.no_swap_chain`: `EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:592`
- `rhi.no_active_frame`: `EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:613`
- `rhi.d3d11.debug_layer_unavailable`: `EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:770`
- `rhi.d3d11.create_device_failed`: `EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:775`
- `rhi.d3d11.injected_failure`: `EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:782`

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `rhi.invalid_color_format` (`EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:113`)

```cpp
111 
  112     return epidemic::foundation::Result<DXGI_FORMAT>::Failure(
  113         epidemic::foundation::Error::Create("rhi.invalid_color_format",
  114                                             "D3D11 backend requires a known DXGI-compatible color format"));
  115 }
```

#### `rhi.d3d11.device_lost` (`EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:141`)

```cpp
139     const auto reason = state.device_lost_reason == S_OK ? DXGI_ERROR_DEVICE_REMOVED : state.device_lost_reason;
  140     return epidemic::foundation::Error::Create(
  141         "rhi.d3d11.device_lost",
  142         "backend=D3D11, operation=" + std::string(operation) + " failed because the D3D11 device was lost (" +
  143             FormatHResult(reason) + ")",
```

#### `rhi.d3d11.swap_chain_missing` (`EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:267`)

```cpp
265         {
  266             return epidemic::foundation::Result<void>::Failure(
  267                 epidemic::foundation::Error::Create("rhi.d3d11.swap_chain_missing",
  268                                                     "D3D11 swap chain is not initialized", descriptor_.debug_name));
  269         }
```

#### `rhi.d3d11.recreate_required` (`EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:470`)

```cpp
468         {
  469             return epidemic::foundation::Result<void>::Failure(
  470                 epidemic::foundation::Error::Create("rhi.d3d11.recreate_required",
  471                                                     "D3D11 swap chain has no valid presentation buffers",
  472                                                     descriptor_.debug_name));
```

#### `rhi.d3d11.present_failed` (`EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:241`)

```cpp
239         {
  240             return epidemic::foundation::Result<void>::Failure(MakeD3D11OperationError(
  241                 *state_, "rhi.d3d11.present_failed", "IDXGISwapChain::Present", result, descriptor_.debug_name));
  242         }
  243
```

#### `rhi.invalid_swap_chain_size` (`EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:259`)

```cpp
257         {
  258             return epidemic::foundation::Result<void>::Failure(
  259                 epidemic::foundation::Error::Create("rhi.invalid_swap_chain_size",
  260                                                     "Swap chain resize dimensions must be greater than zero",
  261                                                     descriptor_.debug_name));
```

#### `rhi.d3d11.resize_failed` (`EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:290`)

```cpp
288         if (FAILED(result))
  289         {
  290             const auto failure = MakeD3D11OperationError(*state_, "rhi.d3d11.resize_failed",
  291                                                          "IDXGISwapChain::ResizeBuffers", result,
  292                                                          descriptor_.debug_name);
```

#### `rhi.invalid_surface_handle` (`EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:411`)

```cpp
409         {
  410             return epidemic::foundation::Result<void>::Failure(
  411                 epidemic::foundation::Error::Create("rhi.invalid_surface_handle",
  412                                                     "D3D11 swap chain requires a live Win32 window handle",
  413                                                     descriptor_.debug_name));
```

#### `rhi.d3d11.get_dxgi_device_failed` (`EngineBase/RHI_D3D11/src/d3d11_rhi_device.cpp:421`)

```cpp
419         {
  420             return epidemic::foundation::Result<void>::Failure(
  421                 MakeD3D11Error("rhi.d3d11.get_dxgi_device_failed", "ID3D11Device::QueryInterface(IDXGIDevice)", result,
  422                                descriptor_.debug_name));
  423         }
```
