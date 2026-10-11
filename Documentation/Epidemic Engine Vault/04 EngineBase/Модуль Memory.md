---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["enginebase", "module", "memory"]
source_path: "EngineBase/Memory"
---

# EngineBase / Memory

Модуль: **Memory**. Алоцирование, memory tracking и memory contracts.

**Реализация:** `EngineBase/Memory/`.
**CMake targets:** `EpidemicMemory`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineBase/Memory/allocation_tag.h|allocation_tag.h]]: `EngineBase/Memory/include/Epidemic/Memory/allocation_tag.h`; объявленные типы: `AllocationTag`
- [[11 API/EngineBase/Memory/allocator.h|allocator.h]]: `EngineBase/Memory/include/Epidemic/Memory/allocator.h`; объявленные типы: `IAllocator`, `DefaultAllocator`, `TrackingAllocator`, `IFrameAllocator`
- [[11 API/EngineBase/Memory/imemory_tracker.h|imemory_tracker.h]]: `EngineBase/Memory/include/Epidemic/Memory/imemory_tracker.h`; объявленные типы: `MemoryTagStatistics`, `IMemoryTracker`
- [[11 API/EngineBase/Memory/memory_tracker.h|memory_tracker.h]]: `EngineBase/Memory/include/Epidemic/Memory/memory_tracker.h`; объявленные типы: `MemoryTracker`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineBase/modules/memory|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Назначение

Memory задает базовый контракт выделения памяти, tag based accounting и soft budgets. Модуль не заменяет стандартные контейнеры собственными и не является владельцем gameplay или resource memory policy.

#### Публичная модель

`IAllocator` описывает выделение и освобождение блока с размером, alignment и `AllocationTag`. Нулевой размер является допустимым запросом и материализуется как один байт. Размер больше `PTRDIFF_MAX` считается недопустимым размером одного объекта. Alignment должен быть ненулевой степенью двойки. `Allocate` сообщает invalid size/alignment через `std::length_error`/`std::invalid_argument`, а operational OOM может распространяться как `std::bad_alloc`. `Deallocate` для `nullptr` или заведомо invalid size/alignment является controlled no-op, чтобы не вызывать undefined behavior в aligned delete.

`DefaultAllocator` использует aligned global new/delete. `TrackingAllocator` вызывает upstream allocator ровно один раз и публикует accounting только после ненулевого успешного результата. Exception или `nullptr` от upstream не меняют tracker state. Для корректного освобождения вызывающий код обязан передать те же `bytes`, `alignment` и tag, что использовались при allocation.

`MemoryTracker` хранит current usage, historical peak, число успешных allocation records и optional budget отдельно для каждого нормализованного tag. Byte counters и allocation count saturate at `SIZE_MAX`, не wrap around. `RecordFree` защищен от unsigned underflow и clamps usage к нулю. `ResetStatistics()` сохраняет текущие live bytes и budgets, сбрасывает historical allocation count и начинает новый peak с текущего usage.

`IFrameAllocator` пока является только extension point. Concrete production frame allocator в EngineBase отсутствует. Для будущей реализации `Reset()` является единственной явной lifetime boundary: storage предыдущего frame/batch может инвалидироваться при `Reset()`, но не неявно при обычном `Allocate`/`Deallocate`. При появлении production implementation этот контракт должен получить отдельный lifetime audit.

#### Владение, потоковость и ошибки

`MemoryTracker` является authoritative owner только своей диагностической статистики, budget values и tracking enabled state. Доступ к ним сериализован внутренним mutex. `DefaultAllocator` не хранит mutable owner state. `TrackingAllocator` хранит только ссылки на tracker и upstream, поэтому его concurrent use допустим только настолько, насколько concurrent use разрешен upstream allocator; сам accounting вызов остается thread safe через `IMemoryTracker` implementation.

Tracking enable/disable влияет только на последующие `RecordAllocate`/`RecordFree`. Переключение режима не выполняет ретроактивную реконструкцию allocation history. Если вызывающему коду требуется точный live usage, он не должен выключать tracking между парными record operations.

Invalid `AllocationTag` нормализуется в `Unknown`, поэтому не может выйти за границы массивов статистики. Budgets являются soft limits: `IsOverBudget()` становится true только при `usage > budget`, на точной границе возвращается false.

#### Persistence и lifecycle

Memory accounting не является persistent engine state и не имеет snapshot/restore contract. У `MemoryTracker` нет lifecycle state machine или shutdown phase. `IFrameAllocator::Reset()` является интерфейсным lifetime hook, а не реализованным production lifecycle owner.

#### Интеграция

`RegisterEngineBase()` регистрирует `IMemoryTracker` как service. Upper layers могут читать статистику и устанавливать budgets, но не должны зависеть от concrete `MemoryTracker`.

#### Ограничения и стабильность

Модуль не реализует garbage collector, gameplay object pool, resource eviction или GPU memory manager. Allocator metadata остается caller supplied, поэтому mismatch между исходными и переданными в `Deallocate` допустимыми `bytes`/tag не может быть обнаружен без per-allocation ownership table и является нарушением precondition вызывающей стороны.

#### Карта публичных заголовков

- `allocation_tag.h`: `AllocationTag`.
- `allocator.h`: `IAllocator`, `DefaultAllocator`, `TrackingAllocator`, `IFrameAllocator`.
- `imemory_tracker.h`: `MemoryTagStatistics`, `IMemoryTracker`.
- `memory_tracker.h`: `MemoryTracker`.

Источник контрактов: `Documentation/EngineBase/modules/memory.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `AllocationTag` (`EngineBase/Memory/include/Epidemic/Memory/allocation_tag.h`): `Core`, `Platform`, `Input`, `RHI`, `D3D11`, `Diagnostics`, `Tests`, `Unknown`, `Count`.

### Операции и хранилища состояния

#### `IAllocator`

Объявлен в `EngineBase/Memory/include/Epidemic/Memory/allocator.h:29`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IAllocator() = default;
a decorator must not publish accounting for either case. virtual void *Allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t), AllocationTag tag = AllocationTag::Unknown) = 0;
```

#### `IFrameAllocator`

Объявлен в `EngineBase/Memory/include/Epidemic/Memory/allocator.h:158`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IFrameAllocator() = default;
```

#### `IMemoryTracker`

Объявлен в `EngineBase/Memory/include/Epidemic/Memory/imemory_tracker.h:23`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IMemoryTracker() = default;
```

#### `TrackingAllocator`

Объявлен в `EngineBase/Memory/include/Epidemic/Memory/allocator.h:104`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
// Binds a tracker interface to an upstream allocator implementation. TrackingAllocator(IMemoryTracker &tracker, IAllocator &upstream) noexcept : tracker_(tracker), upstream_(upstream);
// Allocates through the upstream allocator and records exactly one successful allocation. void *Allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t), AllocationTag tag = AllocationTag::Unknown) override;
// Frees through the upstream allocator and mirrors a valid non-null free into the tracker. void Deallocate(void *pointer, std::size_t bytes, std::size_t alignment = alignof(std::max_align_t), AllocationTag tag = AllocationTag::Unknown) noexcept override;
```

#### `DefaultAllocator`

Объявлен в `EngineBase/Memory/include/Epidemic/Memory/allocator.h:53`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
// Allocates storage through aligned operator new after validating the common allocator contract. void *Allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t), AllocationTag = AllocationTag::Unknown) override;
// Releases storage through aligned operator delete. Null or invalid requests are ignored. void Deallocate(void *pointer, std::size_t bytes, std::size_t alignment = alignof(std::max_align_t), AllocationTag = AllocationTag::Unknown) noexcept override;
```

#### `MemoryTracker`

Объявлен в `EngineBase/Memory/include/Epidemic/Memory/memory_tracker.h:17`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `mutable std::mutex mutex_`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineBase/Memory/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineBase/Memory/src/memory_tracker.cpp` (140 строк): `MemoryTracker::MemoryTracker`, `MemoryTracker::SetTrackingEnabled`, `MemoryTracker::IsTrackingEnabled`, `MemoryTracker::RecordAllocate`, `MemoryTracker::RecordFree`, `MemoryTracker::SetBudget`, `MemoryTracker::GetBudget`, `MemoryTracker::GetUsage`, `MemoryTracker::IsOverBudget`, `MemoryTracker::GetStatistics`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.

