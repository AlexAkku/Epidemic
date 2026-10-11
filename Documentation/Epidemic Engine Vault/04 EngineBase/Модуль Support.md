---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["enginebase", "module", "support"]
source_path: "EngineBase/Support"
---

# EngineBase / Support

Модуль: **Support**. Композиция сервисов и adapters.

**Реализация:** `EngineBase/Support/`.
**CMake targets:** `EpidemicEngineBaseSupport`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineBase/Support/engine_base_support.h|engine_base_support.h]]: `EngineBase/Support/include/Epidemic/EngineBase/engine_base_support.h`; объявленные типы: `ILogger`, `EngineBaseOptions`, `GraphicsRuntimeOptions`, `GraphicsRuntimeServices`, `FramePlatformEvents`
- [[11 API/EngineBase/Support/graphics_backend.h|graphics_backend.h]]: `EngineBase/Support/include/Epidemic/EngineBase/graphics_backend.h`; объявленные типы: `GraphicsBackend`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineBase/modules/support|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Назначение

`EpidemicEngineBaseSupport` — официальный слой композиции EngineBase. Он объединяет повторяющуюся регистрацию сервисов и frame handlers, чтобы приложения и EngineRuntime не копировали Win32/RHI wiring.

#### Публичные функции

`RegisterEngineBase()` создает базовые services. `RegisterWindowsRuntime()` и `RegisterInputRuntime()` добавляют platform и raw input. `RegisterGraphicsRuntime()` выбирает Null или D3D11 backend. `CreateMainWindow()` и `RegisterMainSwapChain()` связывают окно с presentation. Frame-loop helpers добавляют platform pump, input publication, clear/present и optional throttle.

Support возвращает созданные services или `Result`, а не скрывает их в global state. Объекты по-прежнему принадлежат service container и явным shared owners.

#### Граница

Support может зависеть от всех нижних EngineBase modules, но ни один из них не зависит от Support. Здесь не размещаются resources, renderer, world, gameplay или общая логика приложения. Если функция становится самостоятельной runtime-системой, она должна перейти в EngineRuntime.

#### Стабильность

EngineBase Support считается стабильным. Новые convenience helpers допустимы, но текущие функции не должны превращаться в неявный service locator или монолитный engine bootstrap.

#### Карта публичных заголовков

Этот раздел служит быстрым индексом объявлений. Семантика и инварианты описаны выше; точные сигнатуры остаются источником истины в public headers.

- `engine_base_support.h`: `ILogger`, `EngineBaseOptions`, `GraphicsRuntimeOptions`, `GraphicsRuntimeServices`, `FramePlatformEvents`.
- `graphics_backend.h`: `GraphicsBackend`.

Источник контрактов: `Documentation/EngineBase/modules/support.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `GraphicsBackend` (`EngineBase/Support/include/Epidemic/EngineBase/graphics_backend.h`): `Null`, `D3D11`.

### Операции и хранилища состояния

#### `GraphicsRuntimeServices`

Объявлен в `EngineBase/Support/include/Epidemic/EngineBase/engine_base_support.h:51`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::shared_ptr<rhi::IRhiDevice> device`; `std::shared_ptr<rhi::IRhiCommandContext> command_context`.

#### `FramePlatformEvents`

Объявлен в `EngineBase/Support/include/Epidemic/EngineBase/engine_base_support.h:58`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::vector<platform::PlatformEvent> events`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineBase/Support/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicFoundation`, `EpidemicRHI_D3D11`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineBase/Support/src/engine_base_support.cpp` (572 строк): `Error::Create`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `engine_base.support.service_container_sealed`: `EngineBase/Support/src/engine_base_support.cpp:59`
- `engine_base.support.service_registration_conflict`: `EngineBase/Support/src/engine_base_support.cpp:67`
- `engine_base.support.invalid_graphics_backend`: `EngineBase/Support/src/engine_base_support.cpp:288`
- `engine_base.support.service_registration_failed`: `EngineBase/Support/src/engine_base_support.cpp:441`
- `engine_base.support.window_system_required`: `EngineBase/Support/src/engine_base_support.cpp:357`
- `engine_base.support.window_required`: `EngineBase/Support/src/engine_base_support.cpp:374`
- `engine_base.support.invalid_window_handle`: `EngineBase/Support/src/engine_base_support.cpp:390`
- `engine_base.support.rhi_device_required`: `EngineBase/Support/src/engine_base_support.cpp:397`

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `engine_base.support.service_container_sealed` (`EngineBase/Support/src/engine_base_support.cpp:59`)

```cpp
57     if (services.IsSealed())
   58     {
   59         return foundation::Error::Create("engine_base.support.service_container_sealed",
   60                                          "Cannot register " + std::string(bundle_name) +
   61                                              " after the service container is sealed",
```

#### `engine_base.support.service_registration_conflict` (`EngineBase/Support/src/engine_base_support.cpp:67`)

```cpp
65     if ((services.Contains<TServices>() || ...))
   66     {
   67         return foundation::Error::Create("engine_base.support.service_registration_conflict",
   68                                          "Cannot register " + std::string(bundle_name) +
   69                                              " because one of its service roles is already registered",
```

#### `engine_base.support.invalid_graphics_backend` (`EngineBase/Support/src/engine_base_support.cpp:288`)

```cpp
286     foundation::Result<std::shared_ptr<rhi::IRhiDevice>> device_result =
  287         foundation::Result<std::shared_ptr<rhi::IRhiDevice>>::Failure(
  288             foundation::Error::Create("engine_base.support.invalid_graphics_backend",
  289                                       "Graphics backend value is not recognized"));
  290     switch (options.backend)
```

#### `engine_base.support.service_registration_failed` (`EngineBase/Support/src/engine_base_support.cpp:441`)

```cpp
439         }
  440         return foundation::Result<std::shared_ptr<rhi::IRhiSwapChain>>::Failure(
  441             foundation::Error::Create("engine_base.support.service_registration_failed", exception.what(),
  442                                       "main swap chain"));
  443     }
```

#### `engine_base.support.window_system_required` (`EngineBase/Support/src/engine_base_support.cpp:357`)

```cpp
355     {
  356         return foundation::Result<std::shared_ptr<platform::IWindow>>::Failure(
  357             foundation::Error::Create("engine_base.support.window_system_required",
  358                                       "Main window creation requires a registered window system"));
  359     }
```

#### `engine_base.support.window_required` (`EngineBase/Support/src/engine_base_support.cpp:374`)

```cpp
372     {
  373         return foundation::Result<std::shared_ptr<rhi::IRhiSwapChain>>::Failure(
  374             foundation::Error::Create("engine_base.support.window_required",
  375                                       "Main swap-chain registration requires a window"));
  376     }
```

#### `engine_base.support.invalid_window_handle` (`EngineBase/Support/src/engine_base_support.cpp:390`)

```cpp
388     {
  389         return foundation::Result<std::shared_ptr<rhi::IRhiSwapChain>>::Failure(
  390             foundation::Error::Create("engine_base.support.invalid_window_handle",
  391                                       "Main swap-chain registration requires a valid native window handle"));
  392     }
```

#### `engine_base.support.rhi_device_required` (`EngineBase/Support/src/engine_base_support.cpp:397`)

```cpp
395     {
  396         return foundation::Result<std::shared_ptr<rhi::IRhiSwapChain>>::Failure(
  397             foundation::Error::Create("engine_base.support.rhi_device_required",
  398                                       "Main swap-chain registration requires a registered RHI device"));
  399     }
```
