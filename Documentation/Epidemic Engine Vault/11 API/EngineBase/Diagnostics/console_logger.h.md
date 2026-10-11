---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/Diagnostics/include/Epidemic/Diagnostics/console_logger.h"
---

# Diagnostics: console_logger.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/Diagnostics/include/Epidemic/Diagnostics/console_logger.h`.

**Модуль:** [[04 EngineBase/Модуль Diagnostics|Diagnostics]].
**Размер исходного файла:** 707 байт.
**Подключение:** `#include <Epidemic/Diagnostics/console_logger.h>`.

## Объявления типов (лексический индекс)
`ConsoleLogger`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <Epidemic/Diagnostics/logger.h>

#include <fstream>
#include <mutex>

namespace epidemic::diagnostics
{
// Default best-effort console/file sink. ILogger contains all sink exceptions at the public boundary.
class ConsoleLogger final : public ILogger
{
  private:
    // Formats and writes one structured log message to the console and, when available, the log file.
    void Write(const LogMessage &message) override;

    // Lazily opens the log file on first write. Called under mutex from Write.
    void EnsureLogFileInitialized();

    std::mutex mutex_;
    std::ofstream file_;
    bool file_initialized_{false};
};
} // namespace epidemic::diagnostics
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Diagnostics/logger.h`.

### Состав типов и операции

#### `ConsoleLogger` (строка заголовка 11)

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.

