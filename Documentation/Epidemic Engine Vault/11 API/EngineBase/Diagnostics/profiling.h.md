---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/Diagnostics/include/Epidemic/Diagnostics/profiling.h"
---

# Diagnostics: profiling.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/Diagnostics/include/Epidemic/Diagnostics/profiling.h`.

**Модуль:** [[04 EngineBase/Модуль Diagnostics|Diagnostics]].
**Размер исходного файла:** 1973 байт.
**Подключение:** `#include <Epidemic/Diagnostics/profiling.h>`.

## Объявления типов (лексический индекс)
`ProfileEvent`, `IProfileCollector`, `InMemoryProfileCollector`, `ProfileScope`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <Epidemic/Diagnostics/thread_context.h>

#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace epidemic::diagnostics
{
struct ProfileEvent
{
    std::string name;
    std::chrono::nanoseconds duration{};
    std::thread::id thread_id{std::this_thread::get_id()};
    std::string thread_name;
};

class IProfileCollector
{
  public:
    virtual ~IProfileCollector() = default;
    virtual void Record(ProfileEvent event) = 0;
};

class InMemoryProfileCollector final : public IProfileCollector
{
  public:
    void Record(ProfileEvent event) override;
    [[nodiscard]] std::vector<ProfileEvent> Snapshot() const;
    void Reset();

  private:
    mutable std::mutex mutex_;
    std::vector<ProfileEvent> events_;
};

// Replaces the process-wide collector used by subsequently constructed ProfileScope objects.
void SetProfileCollector(std::shared_ptr<IProfileCollector> collector);

[[nodiscard]] std::shared_ptr<IProfileCollector> GetProfileCollector();

void SetProfilingEnabled(bool enabled) noexcept;
[[nodiscard]] bool IsProfilingEnabled() noexcept;

// A scope captures the collector at construction. Replacing/clearing the global collector while a scope
// is active cannot redirect that already-started event to another sink. Collector failures are contained.
class ProfileScope
{
  public:
    explicit ProfileScope(std::string_view scope_name);
    ~ProfileScope() noexcept;

    ProfileScope(const ProfileScope &) = delete;
    ProfileScope &operator=(const ProfileScope &) = delete;

  private:
    std::string name_;
    std::chrono::steady_clock::time_point start_time_{};
    std::shared_ptr<IProfileCollector> collector_;
};
} // namespace epidemic::diagnostics

#define EPIDEMIC_PROFILE_SCOPE(name) ::epidemic::diagnostics::ProfileScope epidemic_profile_scope_##__LINE__(name)
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Diagnostics/thread_context.h`.

### Состав типов и операции

#### `ProfileEvent` (строка заголовка 15)

Публичные поля и значения по умолчанию:

- `std::string name`
- `std::string thread_name`

#### `IProfileCollector` (строка заголовка 23)

Публичные сигнатуры операций:

```cpp
virtual ~IProfileCollector() = default;
virtual void Record(ProfileEvent event) = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `InMemoryProfileCollector` (строка заголовка 30)

Публичные сигнатуры операций:

```cpp
void Record(ProfileEvent event) override;
[[nodiscard]] std::vector<ProfileEvent> Snapshot() const;
void Reset();
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `ProfileScope` (строка заголовка 52)

Публичные сигнатуры операций:

```cpp
explicit ProfileScope(std::string_view scope_name);
~ProfileScope() noexcept;
ProfileScope(const ProfileScope &) = delete;
ProfileScope &operator=(const ProfileScope &) = delete;
```

### Интеграция и безопасность вызовов

- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.
- Для `std::shared_ptr` отдельно проверяйте циклы владения и контракт передачи внешних объектов.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `Record` | `InMemoryProfileCollector::Record` | `EngineBase/Diagnostics/src/profiling.cpp:15` |
| `Snapshot` | `InMemoryProfileCollector::Snapshot` | `EngineBase/Diagnostics/src/profiling.cpp:21` |
| `Reset` | `InMemoryProfileCollector::Reset` | `EngineBase/Diagnostics/src/profiling.cpp:27` |
| `ProfileScope` | `ProfileScope::ProfileScope` | `EngineBase/Diagnostics/src/profiling.cpp:55` |
| `Reset` | `DiagnosticsCounters::Reset` | `EngineBase/Diagnostics/src/counters.cpp:92` |

