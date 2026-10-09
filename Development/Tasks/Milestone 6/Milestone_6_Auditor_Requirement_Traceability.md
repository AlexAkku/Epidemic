# Milestone 6 — Auditor requirement traceability

Source SHA-256: `08ea103776d7e29489fde4e4c1d0d227bc847022d3073026e2b0512eee5fa222`

Every source requirement is preserved verbatim. The `Primary stage` is the stage that owns its acceptance proof.

| Source ID | Primary stage | Evidence family | Original requirement |
|---:|:---:|---|---|
| 1 | MS6.2 | GUI / modes / build isolation | GUI для управления Auditor. |
| 2 | MS6.2 | GUI / modes / build isolation | Кнопка Configure. |
| 3 | MS6.2 | GUI / modes / build isolation | Кнопка Build. |
| 4 | MS6.2 | GUI / modes / build isolation | Кнопка Build + Run. |
| 5 | MS6.2 | GUI / modes / build isolation | Кнопка Run All Tests. |
| 6 | MS6.2 | GUI / modes / build isolation | Запуск одного конкретного теста. |
| 7 | MS6.2 | GUI / modes / build isolation | Выбор нескольких тестов и запуск выбранной группы. |
| 8 | MS6.2 | GUI / modes / build isolation | Запуск заранее определённых групп тестов по тегам и подсистемам. |
| 9 | MS6.2 | GUI / modes / build isolation | Автоматическое обнаружение CTest тестов. |
| 10 | MS6.2 | GUI / modes / build isolation | Каталог всех тестов с именем, группой, тегами, executable, аргументами и source location. |
| 11 | MS6.2 | GUI / modes / build isolation | История результатов каждого теста. |
| 12 | MS6.2 | GUI / modes / build isolation | История времени выполнения каждого теста. |
| 13 | MS6.2 | GUI / modes / build isolation | Статистика стабильности теста по предыдущим запускам. |
| 14 | MS6.2 | GUI / modes / build isolation | Кнопка запуска самой игры. |
| 15 | MS6.2 | GUI / modes / build isolation | Возможность остановить запущенный тест или игру. |
| 16 | MS6.2 | GUI / modes / build isolation | Несколько режимов аудита: Fast, Full, Deep, Memory, Coverage, Stress. |
| 17 | MS6.2 | GUI / modes / build isolation | Отдельная Audit сборка. |
| 18 | MS6.2 | GUI / modes / build isolation | Отдельная Deep Audit сборка. |
| 19 | MS6.2 | GUI / modes / build isolation | Независимая от обычного Debug/Release папка сборки. |
| 20 | MS6.2 | GUI / modes / build isolation | Возможность использовать Auditor, не изменяя production build. |
| 21 | MS6.2 | GUI / modes / build isolation | Возможность вообще не коммитить Auditor в репозиторий Epidemic. |
| 22 | MS6.2 | GUI / modes / build isolation | Возможность держать Auditor отдельным Git repository внутри папки проекта. |
| 23 | MS6.2 | CMake/build intelligence | Автоматическое определение структуры CMake проекта. |
| 24 | MS6.2 | CMake/build intelligence | Сохранение точных команд CMake configure. |
| 25 | MS6.2 | CMake/build intelligence | Сохранение точных команд build. |
| 26 | MS6.2 | CMake/build intelligence | Сохранение compiler flags. |
| 27 | MS6.2 | CMake/build intelligence | Сохранение defines. |
| 28 | MS6.2 | CMake/build intelligence | Сохранение версии компилятора. |
| 29 | MS6.2 | CMake/build intelligence | Сохранение используемого toolchain. |
| 30 | MS6.2 | CMake/build intelligence | Сохранение параметров оптимизации. |
| 31 | MS6.2 | CMake/build intelligence | Сохранение параметров instrumentation. |
| 32 | MS6.2 | CMake/build intelligence | Время CMake configure. |
| 33 | MS6.2 | CMake/build intelligence | Время компиляции. |
| 34 | MS6.2 | CMake/build intelligence | Время линковки. |
| 35 | MS6.2 | CMake/build intelligence | Структурированный сбор compiler warnings. |
| 36 | MS6.2 | CMake/build intelligence | Структурированный сбор compiler errors. |
| 37 | MS6.2 | CMake/build intelligence | Структурированный сбор linker errors. |
| 38 | MS6.2 | CMake/build intelligence | Автоматическая классификация ошибок сборки. |
| 39 | MS6.2 | CMake/build intelligence | Отдельный finding для первой причины падения сборки. |
| 40 | MS6.2 | CMake/build intelligence | Обнаружение проблем типа MSVC C1090/PDB. |
| 41 | MS6.2 | CMake/build intelligence | Фиксация количества параллельно работающих compiler processes при ошибке. |
| 42 | MS6.2 | CMake/build intelligence | Возможность автоматически прогонять reproduction matrix с разной параллельностью сборки. |
| 43 | MS6.3 | managed process / modules / symbols | Независимый запуск каждого процесса самим Auditor. |
| 44 | MS6.3 | managed process / modules / symbols | Сохранение PID и PPID. |
| 45 | MS6.3 | managed process / modules / symbols | Сохранение command line процесса. |
| 46 | MS6.3 | managed process / modules / symbols | Сохранение working directory. |
| 47 | MS6.3 | managed process / modules / symbols | Сохранение environment. |
| 48 | MS6.3 | managed process / modules / symbols | Сохранение exit code. |
| 49 | MS6.3 | managed process / modules / symbols | Отслеживание дерева дочерних процессов. |
| 50 | MS6.3 | managed process / modules / symbols | Сохранение времени создания и завершения процессов. |
| 51 | MS6.3 | managed process / modules / symbols | Список загруженных DLL и EXE modules. |
| 52 | MS6.3 | managed process / modules / symbols | Версии загруженных модулей. |
| 53 | MS6.3 | managed process / modules / symbols | PDB symbolization. |
| 54 | MS6.3 | managed process / modules / symbols | Преобразование адресов в имена функций. |
| 55 | MS6.3 | managed process / modules / symbols | Преобразование адресов в source file и line. |
| 56 | MS6.3 | process/thread/wait/lock telemetry | Внешнее измерение wall clock времени. |
| 57 | MS6.3 | process/thread/wait/lock telemetry | Внешнее измерение CPU time. |
| 58 | MS6.3 | process/thread/wait/lock telemetry | Использование независимого monotonic profiling clock. |
| 59 | MS6.3 | process/thread/wait/lock telemetry | Отдельное понимание Game Time, Simulation Time, Wall Time и Trace Time. |
| 60 | MS6.3 | process/thread/wait/lock telemetry | CPU usage процесса. |
| 61 | MS6.3 | process/thread/wait/lock telemetry | CPU usage всего дерева процессов. |
| 62 | MS6.3 | process/thread/wait/lock telemetry | RSS памяти. |
| 63 | MS6.3 | process/thread/wait/lock telemetry | Peak memory. |
| 64 | MS6.3 | process/thread/wait/lock telemetry | Количество потоков. |
| 65 | MS6.3 | process/thread/wait/lock telemetry | История CPU во времени. |
| 66 | MS6.3 | process/thread/wait/lock telemetry | История использования памяти во времени. |
| 67 | MS6.3 | process/thread/wait/lock telemetry | Thread timeline. |
| 68 | MS6.3 | process/thread/wait/lock telemetry | Состояния RUN и WAIT для потоков. |
| 69 | MS6.3 | process/thread/wait/lock telemetry | Context switches. |
| 70 | MS6.3 | process/thread/wait/lock telemetry | Scheduler activity. |
| 71 | MS6.3 | process/thread/wait/lock telemetry | Thread creation и destruction. |
| 72 | MS6.3 | process/thread/wait/lock telemetry | Время работы каждого worker thread. |
| 73 | MS6.3 | process/thread/wait/lock telemetry | Поиск thread starvation. |
| 74 | MS6.3 | process/thread/wait/lock telemetry | Поиск зависших worker threads. |
| 75 | MS6.3 | process/thread/wait/lock telemetry | Поиск orphan jobs. |
| 76 | MS6.3 | process/thread/wait/lock telemetry | Поиск работы, продолжающейся после уничтожения owner. |
| 77 | MS6.3 | process/thread/wait/lock telemetry | Анализ cancellation latency. |
| 78 | MS6.3 | process/thread/wait/lock telemetry | Анализ late completion. |
| 79 | MS6.3 | process/thread/wait/lock telemetry | Анализ waits. |
| 80 | MS6.3 | process/thread/wait/lock telemetry | Анализ mutex contention. |
| 81 | MS6.3 | process/thread/wait/lock telemetry | Анализ lock contention. |
| 82 | MS6.3 | process/thread/wait/lock telemetry | Определение владельца lock. |
| 83 | MS6.3 | process/thread/wait/lock telemetry | Время удержания lock. |
| 84 | MS6.3 | process/thread/wait/lock telemetry | Определение call stack, где lock был захвачен. |
| 85 | MS6.3 | ETW/WPR/sampling | ETW capture. |
| 86 | MS6.3 | ETW/WPR/sampling | WPR capture. |
| 87 | MS6.3 | ETW/WPR/sampling | Возможность открытия trace в WPA. |
| 88 | MS6.3 | ETW/WPR/sampling | Автоматическая конвертация части ETW данных в текстовый формат. |
| 89 | MS6.3 | ETW/WPR/sampling | xperf отчёты. |
| 90 | MS6.3 | ETW/WPR/sampling | Sampling profiler. |
| 91 | MS6.3 | ETW/WPR/sampling | CPU sampling call stacks. |
| 92 | MS6.3 | ETW/WPR/sampling | Flame graph. |
| 93 | MS6.4 | exact function tracing / function history | Exact function instrumentation. |
| 94 | MS6.4 | exact function tracing / function history | MSVC `/Gh` instrumentation. |
| 95 | MS6.4 | exact function tracing / function history | MSVC `/GH` instrumentation. |
| 96 | MS6.4 | exact function tracing / function history | Собственный `_penter` hook Auditor. |
| 97 | MS6.4 | exact function tracing / function history | Собственный `_pexit` hook Auditor. |
| 98 | MS6.4 | exact function tracing / function history | Запись каждого function enter. |
| 99 | MS6.4 | exact function tracing / function history | Запись каждого function exit. |
| 100 | MS6.4 | exact function tracing / function history | Timestamp каждого входа и выхода. |
| 101 | MS6.4 | exact function tracing / function history | PID каждого вызова. |
| 102 | MS6.4 | exact function tracing / function history | Thread ID каждого вызова. |
| 103 | MS6.4 | exact function tracing / function history | Адрес функции каждого вызова. |
| 104 | MS6.4 | exact function tracing / function history | Exact call count. |
| 105 | MS6.4 | exact function tracing / function history | Exact call tree. |
| 106 | MS6.4 | exact function tracing / function history | Caller → callee graph. |
| 107 | MS6.4 | exact function tracing / function history | Self time функции. |
| 108 | MS6.4 | exact function tracing / function history | Inclusive time функции. |
| 109 | MS6.4 | exact function tracing / function history | Minimum execution time. |
| 110 | MS6.4 | exact function tracing / function history | Maximum execution time. |
| 111 | MS6.4 | exact function tracing / function history | Mean execution time. |
| 112 | MS6.4 | exact function tracing / function history | Median p50. |
| 113 | MS6.4 | exact function tracing / function history | p95. |
| 114 | MS6.4 | exact function tracing / function history | p99. |
| 115 | MS6.4 | exact function tracing / function history | Количество callers функции. |
| 116 | MS6.4 | exact function tracing / function history | Количество callees функции. |
| 117 | MS6.4 | exact function tracing / function history | Распределение функции по потокам. |
| 118 | MS6.4 | exact function tracing / function history | Hot functions. |
| 119 | MS6.4 | exact function tracing / function history | Hot call paths. |
| 120 | MS6.4 | exact function tracing / function history | Самые дорогие функции по total CPU. |
| 121 | MS6.4 | exact function tracing / function history | Самые дорогие функции по self CPU. |
| 122 | MS6.4 | exact function tracing / function history | Функции с наибольшим количеством вызовов. |
| 123 | MS6.4 | exact function tracing / function history | Функции с редкими патологическими spikes. |
| 124 | MS6.4 | exact function tracing / function history | Долговременная база статистики функций. |
| 125 | MS6.4 | exact function tracing / function history | Количество запусков, в которых функция наблюдалась. |
| 126 | MS6.4 | exact function tracing / function history | Количество тестов, которые реально используют функцию. |
| 127 | MS6.4 | exact function tracing / function history | Общее количество вызовов функции за всю историю Auditor. |
| 128 | MS6.4 | exact function tracing / function history | История performance функции. |
| 129 | MS6.4 | exact function tracing / function history | Определение performance regression конкретной функции. |
| 130 | MS6.4 | exact function tracing / function history | Поиск функций, которые вообще никогда не выполнялись. |
| 131 | MS6.4 | exact function tracing / function history | Поиск функций, которые выполнялись только один раз. |
| 132 | MS6.4 | coverage | Code coverage. |
| 133 | MS6.4 | coverage | Function coverage. |
| 134 | MS6.4 | coverage | Line coverage. |
| 135 | MS6.4 | coverage | Branch coverage. |
| 136 | MS6.4 | coverage | Объединение coverage нескольких запусков. |
| 137 | MS6.4 | coverage | Coverage по одному тесту. |
| 138 | MS6.4 | coverage | Coverage по группе тестов. |
| 139 | MS6.4 | coverage | Coverage всего qualification suite. |
| 140 | MS6.4 | coverage | Coverage heatmap. |
| 141 | MS6.4 | coverage | Сравнение существующего source code с реально исполнявшимся кодом. |
| 142 | MS6.4 | static/runtime architecture / rules | Static source analysis. |
| 143 | MS6.4 | static/runtime architecture / rules | Include graph. |
| 144 | MS6.4 | static/runtime architecture / rules | Module dependency graph. |
| 145 | MS6.4 | static/runtime architecture / rules | Namespace dependency graph. |
| 146 | MS6.4 | static/runtime architecture / rules | Public API graph. |
| 147 | MS6.4 | static/runtime architecture / rules | Class dependency graph. |
| 148 | MS6.4 | static/runtime architecture / rules | Поиск циклических зависимостей. |
| 149 | MS6.4 | static/runtime architecture / rules | Поиск forbidden dependencies. |
| 150 | MS6.4 | static/runtime architecture / rules | Поиск слишком больших функций. |
| 151 | MS6.4 | static/runtime architecture / rules | Complexity analysis. |
| 152 | MS6.4 | static/runtime architecture / rules | Поиск подозрительных ownership patterns. |
| 153 | MS6.4 | static/runtime architecture / rules | Поиск unused code. |
| 154 | MS6.4 | static/runtime architecture / rules | Поиск duplicate code. |
| 155 | MS6.4 | static/runtime architecture / rules | Static architecture graph. |
| 156 | MS6.4 | static/runtime architecture / rules | Runtime architecture graph. |
| 157 | MS6.4 | static/runtime architecture / rules | Сопоставление static и runtime архитектуры. |
| 158 | MS6.4 | static/runtime architecture / rules | Отображение количества runtime вызовов между подсистемами. |
| 159 | MS6.4 | static/runtime architecture / rules | Отображение CPU стоимости взаимодействия между подсистемами. |
| 160 | MS6.4 | static/runtime architecture / rules | Поиск неожиданных runtime dependencies. |
| 161 | MS6.4 | static/runtime architecture / rules | Runtime architecture verifier. |
| 162 | MS6.4 | static/runtime architecture / rules | Проверка правил вроде `Renderer must not call QuestSystem`. |
| 163 | MS6.4 | static/runtime architecture / rules | Отдельный rules engine. |
| 164 | MS6.4 | static/runtime architecture / rules | `architecture.yaml`. |
| 165 | MS6.4 | static/runtime architecture / rules | `concurrency.yaml`. |
| 166 | MS6.4 | static/runtime architecture / rules | `persistence.yaml`. |
| 167 | MS6.4 | static/runtime architecture / rules | `performance.yaml`. |
| 168 | MS6.4 | static/runtime architecture / rules | `memory.yaml`. |
| 169 | MS6.4 | static/runtime architecture / rules | `io.yaml`. |
| 170 | MS6.4 | static/runtime architecture / rules | Автоматическое формирование findings по нарушениям правил. |
| 171 | MS6.5 | exceptions/crash/assert/recovery | Exception monitoring. |
| 172 | MS6.5 | exceptions/crash/assert/recovery | Windows Debug API. |
| 173 | MS6.5 | exceptions/crash/assert/recovery | First chance exceptions. |
| 174 | MS6.5 | exceptions/crash/assert/recovery | Second chance exceptions. |
| 175 | MS6.5 | exceptions/crash/assert/recovery | Exception code. |
| 176 | MS6.5 | exceptions/crash/assert/recovery | Exception address. |
| 177 | MS6.5 | exceptions/crash/assert/recovery | Thread, в котором произошло исключение. |
| 178 | MS6.5 | exceptions/crash/assert/recovery | Stack trace исключения. |
| 179 | MS6.5 | exceptions/crash/assert/recovery | Определение обработанного исключения. |
| 180 | MS6.5 | exceptions/crash/assert/recovery | Определение необработанного исключения. |
| 181 | MS6.5 | exceptions/crash/assert/recovery | Поиск постоянно выбрасываемых и ловимых исключений. |
| 182 | MS6.5 | exceptions/crash/assert/recovery | Статистика количества исключений. |
| 183 | MS6.5 | exceptions/crash/assert/recovery | Оценка CPU стоимости repeated exceptions. |
| 184 | MS6.5 | exceptions/crash/assert/recovery | Crash capture. |
| 185 | MS6.5 | exceptions/crash/assert/recovery | Stack trace падения. |
| 186 | MS6.5 | exceptions/crash/assert/recovery | Assert capture. |
| 187 | MS6.5 | exceptions/crash/assert/recovery | Contract violation capture. |
| 188 | MS6.5 | exceptions/crash/assert/recovery | Сохранение recovery path после ошибки. |
| 189 | MS6.5 | memory/safety | Memory allocation tracing. |
| 190 | MS6.5 | memory/safety | Deallocation tracing. |
| 191 | MS6.5 | memory/safety | Allocation stack. |
| 192 | MS6.5 | memory/safety | Размер allocation. |
| 193 | MS6.5 | memory/safety | Lifetime allocation. |
| 194 | MS6.5 | memory/safety | Allocation rate. |
| 195 | MS6.5 | memory/safety | Allocations конкретной функции. |
| 196 | MS6.5 | memory/safety | Allocations конкретного игрового действия. |
| 197 | MS6.5 | memory/safety | Peak heap. |
| 198 | MS6.5 | memory/safety | Поиск memory leaks. |
| 199 | MS6.5 | memory/safety | Use after free detection. |
| 200 | MS6.5 | memory/safety | Double free detection. |
| 201 | MS6.5 | memory/safety | Out of bounds detection. |
| 202 | MS6.5 | memory/safety | Heap corruption detection. |
| 203 | MS6.5 | memory/safety | AddressSanitizer profile. |
| 204 | MS6.5 | memory/safety | Application Verifier profile. |
| 205 | MS6.5 | IO/network/Win32 API | IO tracing. |
| 206 | MS6.5 | IO/network/Win32 API | File open. |
| 207 | MS6.5 | IO/network/Win32 API | File close. |
| 208 | MS6.5 | IO/network/Win32 API | File read. |
| 209 | MS6.5 | IO/network/Win32 API | File write. |
| 210 | MS6.5 | IO/network/Win32 API | File seek. |
| 211 | MS6.5 | IO/network/Win32 API | Memory mapped files. |
| 212 | MS6.5 | IO/network/Win32 API | Directory enumeration. |
| 213 | MS6.5 | IO/network/Win32 API | Привязка IO операции к call stack. |
| 214 | MS6.5 | IO/network/Win32 API | Поиск synchronous IO на Main Thread. |
| 215 | MS6.5 | IO/network/Win32 API | Статистика file IO по функции. |
| 216 | MS6.5 | IO/network/Win32 API | Disk latency. |
| 217 | MS6.5 | IO/network/Win32 API | Network activity. |
| 218 | MS6.5 | IO/network/Win32 API | Отслеживание выбранных Win32 API. |
| 219 | MS6.5 | IO/network/Win32 API | File API calls. |
| 220 | MS6.5 | IO/network/Win32 API | Memory API calls. |
| 221 | MS6.5 | IO/network/Win32 API | Synchronization API calls. |
| 222 | MS6.5 | IO/network/Win32 API | Thread API calls. |
| 223 | MS6.5 | IO/network/Win32 API | Socket API calls. |
| 224 | MS6.5 | IO/network/Win32 API | Window API calls. |
| 225 | MS6.5 | IO/network/Win32 API | При необходимости GPU API calls. |
| 226 | MS6.5 | IO/network/Win32 API | Imported API inventory для EXE/DLL. |
| 227 | MS6.5 | IO/network/Win32 API | Call graph системных API в Deep режиме. |
| 228 | MS6.6 | causal IDs / probes / provenance | Causal tracing. |
| 229 | MS6.6 | causal IDs / probes / provenance | Отдельный `TraceId`. |
| 230 | MS6.6 | causal IDs / probes / provenance | `SpanId`. |
| 231 | MS6.6 | causal IDs / probes / provenance | `ParentSpanId`. |
| 232 | MS6.6 | causal IDs / probes / provenance | `EventId`. |
| 233 | MS6.6 | causal IDs / probes / provenance | `CallId`. |
| 234 | MS6.6 | causal IDs / probes / provenance | `FindingId`. |
| 235 | MS6.6 | causal IDs / probes / provenance | `FunctionId`. |
| 236 | MS6.6 | causal IDs / probes / provenance | `ModuleId`. |
| 237 | MS6.6 | causal IDs / probes / provenance | `EntityId`. |
| 238 | MS6.6 | causal IDs / probes / provenance | `JobId`. |
| 239 | MS6.6 | causal IDs / probes / provenance | `CommandId`. |
| 240 | MS6.6 | causal IDs / probes / provenance | Связывание async работы с породившим её действием. |
| 241 | MS6.6 | causal IDs / probes / provenance | Связывание JobSystem Submit с Execute. |
| 242 | MS6.6 | causal IDs / probes / provenance | Связывание EventBus publish с обработчиками. |
| 243 | MS6.6 | causal IDs / probes / provenance | Связывание command dispatch с выполнением. |
| 244 | MS6.6 | causal IDs / probes / provenance | Связывание coroutine continuation. |
| 245 | MS6.6 | causal IDs / probes / provenance | Связывание async IO. |
| 246 | MS6.6 | causal IDs / probes / provenance | Связывание physics callbacks. |
| 247 | MS6.6 | causal IDs / probes / provenance | Связывание render commands. |
| 248 | MS6.6 | causal IDs / probes / provenance | Полный causal DAG. |
| 249 | MS6.6 | causal IDs / probes / provenance | Отдельный call tree внутри каждого causal node. |
| 250 | MS6.6 | causal IDs / probes / provenance | Разделение понятий «что произошло» и «какие функции это выполнили». |
| 251 | MS6.6 | causal IDs / probes / provenance | Optional Audit probes. |
| 252 | MS6.6 | causal IDs / probes / provenance | Probe для JobSystem. |
| 253 | MS6.6 | causal IDs / probes / provenance | Probe для EventBus. |
| 254 | MS6.6 | causal IDs / probes / provenance | Probe для CommandBus. |
| 255 | MS6.6 | causal IDs / probes / provenance | Probe для async/coroutines. |
| 256 | MS6.6 | causal IDs / probes / provenance | Probe для Persistence transactions. |
| 257 | MS6.6 | causal IDs / probes / provenance | Probe для Entity lifecycle. |
| 258 | MS6.6 | causal IDs / probes / provenance | Возможность применять probes только в shadow Audit build. |
| 259 | MS6.6 | causal IDs / probes / provenance | Отсутствие probes в production build. |
| 260 | MS6.6 | causal IDs / probes / provenance | Маркировка происхождения каждого факта. |
| 261 | MS6.6 | causal IDs / probes / provenance | `external`. |
| 262 | MS6.6 | causal IDs / probes / provenance | `compiler_instrumentation`. |
| 263 | MS6.6 | causal IDs / probes / provenance | `static_analysis`. |
| 264 | MS6.6 | causal IDs / probes / provenance | `engine_probe`. |
| 265 | MS6.6 | causal IDs / probes / provenance | Уровень доверия `observed`. |
| 266 | MS6.6 | causal IDs / probes / provenance | Уровень доверия `derived`. |
| 267 | MS6.6 | causal IDs / probes / provenance | Уровень доверия `declared`. |
| 268 | MS6.6 | causal IDs / probes / provenance | Уровень доверия `heuristic`. |
| 269 | MS6.6 | actions / rolling capture / scenarios | Interactive event log всей игры. |
| 270 | MS6.6 | actions / rolling capture / scenarios | Возможность выбрать конкретное пользовательское действие. |
| 271 | MS6.6 | actions / rolling capture / scenarios | `Action Start`. |
| 272 | MS6.6 | actions / rolling capture / scenarios | `Action End`. |
| 273 | MS6.6 | actions / rolling capture / scenarios | Имя action, например `Pickup Item`. |
| 274 | MS6.6 | actions / rolling capture / scenarios | Выделение exact trace только для выбранного временного интервала. |
| 275 | MS6.6 | actions / rolling capture / scenarios | Профиль функций конкретного action. |
| 276 | MS6.6 | actions / rolling capture / scenarios | Call tree конкретного action. |
| 277 | MS6.6 | actions / rolling capture / scenarios | Exceptions конкретного action. |
| 278 | MS6.6 | actions / rolling capture / scenarios | Allocations конкретного action. |
| 279 | MS6.6 | actions / rolling capture / scenarios | Threads конкретного action. |
| 280 | MS6.6 | actions / rolling capture / scenarios | IO конкретного action. |
| 281 | MS6.6 | actions / rolling capture / scenarios | В будущем state changes конкретного action. |
| 282 | MS6.6 | actions / rolling capture / scenarios | Возможность режима `Arm next input`. |
| 283 | MS6.6 | actions / rolling capture / scenarios | Rolling trace buffer. |
| 284 | MS6.6 | actions / rolling capture / scenarios | Сохранение N секунд до события. |
| 285 | MS6.6 | actions / rolling capture / scenarios | Сохранение N секунд после события. |
| 286 | MS6.6 | actions / rolling capture / scenarios | Автоматический capture при crash. |
| 287 | MS6.6 | actions / rolling capture / scenarios | Автоматический capture при assert. |
| 288 | MS6.6 | actions / rolling capture / scenarios | Автоматический capture при contract violation. |
| 289 | MS6.6 | actions / rolling capture / scenarios | Автоматический capture при слишком длинном frame. |
| 290 | MS6.6 | actions / rolling capture / scenarios | Автоматический capture при подозрении на deadlock. |
| 291 | MS6.6 | actions / rolling capture / scenarios | Автоматический capture при memory error. |
| 292 | MS6.6 | actions / rolling capture / scenarios | Сценарии, не являющиеся unit tests. |
| 293 | MS6.6 | actions / rolling capture / scenarios | Сценарий поднятия предмета. |
| 294 | MS6.6 | actions / rolling capture / scenarios | Сценарий save/load. |
| 295 | MS6.6 | actions / rolling capture / scenarios | Сценарий боя. |
| 296 | MS6.6 | actions / rolling capture / scenarios | Сценарий входа в меню. |
| 297 | MS6.6 | actions / rolling capture / scenarios | Сценарий streaming. |
| 298 | MS6.6 | actions / rolling capture / scenarios | Возможность сохранять Scenario Capture отдельно от тестов. |
| 299 | MS6.7 | findings / dashboard / GUI views | Findings Engine. |
| 300 | MS6.7 | findings / dashboard / GUI views | Performance findings. |
| 301 | MS6.7 | findings / dashboard / GUI views | Concurrency findings. |
| 302 | MS6.7 | findings / dashboard / GUI views | Memory findings. |
| 303 | MS6.7 | findings / dashboard / GUI views | Architecture findings. |
| 304 | MS6.7 | findings / dashboard / GUI views | Exception findings. |
| 305 | MS6.7 | findings / dashboard / GUI views | IO findings. |
| 306 | MS6.7 | findings / dashboard / GUI views | Build findings. |
| 307 | MS6.7 | findings / dashboard / GUI views | Regression findings. |
| 308 | MS6.7 | findings / dashboard / GUI views | Severity findings. |
| 309 | MS6.7 | findings / dashboard / GUI views | Автоматические рекомендации по воспроизведению найденной проблемы. |
| 310 | MS6.7 | findings / dashboard / GUI views | Dashboard после каждого Run. |
| 311 | MS6.7 | findings / dashboard / GUI views | Build result. |
| 312 | MS6.7 | findings / dashboard / GUI views | Test result. |
| 313 | MS6.7 | findings / dashboard / GUI views | Crashes. |
| 314 | MS6.7 | findings / dashboard / GUI views | Exceptions. |
| 315 | MS6.7 | findings / dashboard / GUI views | Warnings. |
| 316 | MS6.7 | findings / dashboard / GUI views | CPU. |
| 317 | MS6.7 | findings / dashboard / GUI views | Memory. |
| 318 | MS6.7 | findings / dashboard / GUI views | Threads. |
| 319 | MS6.7 | findings / dashboard / GUI views | Long waits. |
| 320 | MS6.7 | findings / dashboard / GUI views | Lock contention. |
| 321 | MS6.7 | findings / dashboard / GUI views | Functions observed. |
| 322 | MS6.7 | findings / dashboard / GUI views | Coverage. |
| 323 | MS6.7 | findings / dashboard / GUI views | New findings. |
| 324 | MS6.7 | findings / dashboard / GUI views | Regressions. |
| 325 | MS6.7 | findings / dashboard / GUI views | Вкладка Build. |
| 326 | MS6.7 | findings / dashboard / GUI views | Вкладка Tests. |
| 327 | MS6.7 | findings / dashboard / GUI views | Вкладка Timeline. |
| 328 | MS6.7 | findings / dashboard / GUI views | Вкладка Actions. |
| 329 | MS6.7 | findings / dashboard / GUI views | Вкладка Call Tree. |
| 330 | MS6.7 | findings / dashboard / GUI views | Вкладка Causal Graph. |
| 331 | MS6.7 | findings / dashboard / GUI views | Вкладка Functions. |
| 332 | MS6.7 | findings / dashboard / GUI views | Вкладка Threads. |
| 333 | MS6.7 | findings / dashboard / GUI views | Вкладка Locks & Waits. |
| 334 | MS6.7 | findings / dashboard / GUI views | Вкладка Memory. |
| 335 | MS6.7 | findings / dashboard / GUI views | Вкладка Allocations. |
| 336 | MS6.7 | findings / dashboard / GUI views | Вкладка IO. |
| 337 | MS6.7 | findings / dashboard / GUI views | Вкладка Exceptions. |
| 338 | MS6.7 | findings / dashboard / GUI views | Вкладка Coverage. |
| 339 | MS6.7 | findings / dashboard / GUI views | Вкладка Architecture. |
| 340 | MS6.7 | findings / dashboard / GUI views | Вкладка Findings. |
| 341 | MS6.7 | findings / dashboard / GUI views | Вкладка Compare. |
| 342 | MS6.7 | findings / dashboard / GUI views | Вкладка Agent. |
| 343 | MS6.7 | visualization / compare / regression history | CPU charts. |
| 344 | MS6.7 | visualization / compare / regression history | Memory charts. |
| 345 | MS6.7 | visualization / compare / regression history | Function flame graph. |
| 346 | MS6.7 | visualization / compare / regression history | Thread timeline visualization. |
| 347 | MS6.7 | visualization / compare / regression history | Causal graph visualization. |
| 348 | MS6.7 | visualization / compare / regression history | Module dependency visualization. |
| 349 | MS6.7 | visualization / compare / regression history | Runtime dependency visualization. |
| 350 | MS6.7 | visualization / compare / regression history | Allocation histogram. |
| 351 | MS6.7 | visualization / compare / regression history | Lock contention charts. |
| 352 | MS6.7 | visualization / compare / regression history | IO timeline. |
| 353 | MS6.7 | visualization / compare / regression history | Test duration history. |
| 354 | MS6.7 | visualization / compare / regression history | Performance history. |
| 355 | MS6.7 | visualization / compare / regression history | Exception frequency charts. |
| 356 | MS6.7 | visualization / compare / regression history | Function usage charts. |
| 357 | MS6.7 | visualization / compare / regression history | Compare двух runs. |
| 358 | MS6.7 | visualization / compare / regression history | Baseline vs current. |
| 359 | MS6.7 | visualization / compare / regression history | Новые функции. |
| 360 | MS6.7 | visualization / compare / regression history | Исчезнувшие функции. |
| 361 | MS6.7 | visualization / compare / regression history | Новые runtime dependency edges. |
| 362 | MS6.7 | visualization / compare / regression history | Исчезнувшие dependency edges. |
| 363 | MS6.7 | visualization / compare / regression history | Изменение CPU. |
| 364 | MS6.7 | visualization / compare / regression history | Изменение RAM. |
| 365 | MS6.7 | visualization / compare / regression history | Изменение allocations. |
| 366 | MS6.7 | visualization / compare / regression history | Изменение p95/p99 timings. |
| 367 | MS6.7 | visualization / compare / regression history | Новые exceptions. |
| 368 | MS6.7 | visualization / compare / regression history | Новые waits. |
| 369 | MS6.7 | visualization / compare / regression history | Новые locks. |
| 370 | MS6.7 | visualization / compare / regression history | Изменение coverage. |
| 371 | MS6.7 | visualization / compare / regression history | Regression history. |
| 372 | MS6.1 | SQLite / immutable evidence pipeline / run layout | Persistent SQLite database для GUI и истории. |
| 373 | MS6.1 | SQLite / immutable evidence pipeline / run layout | Одновременно human-readable exports. |
| 374 | MS6.1 | SQLite / immutable evidence pipeline / run layout | JSON. |
| 375 | MS6.1 | SQLite / immutable evidence pipeline / run layout | JSONL. |
| 376 | MS6.1 | SQLite / immutable evidence pipeline / run layout | CSV. |
| 377 | MS6.1 | SQLite / immutable evidence pipeline / run layout | Markdown. |
| 378 | MS6.1 | SQLite / immutable evidence pipeline / run layout | Immutable raw captures. |
| 379 | MS6.1 | SQLite / immutable evidence pipeline / run layout | Отдельный слой normalized events. |
| 380 | MS6.1 | SQLite / immutable evidence pipeline / run layout | Отдельный слой derived graphs. |
| 381 | MS6.1 | SQLite / immutable evidence pipeline / run layout | Отдельный слой metrics. |
| 382 | MS6.1 | SQLite / immutable evidence pipeline / run layout | Отдельный слой findings. |
| 383 | MS6.1 | SQLite / immutable evidence pipeline / run layout | Возможность повторно прогнать новый Analyzer по старому raw capture. |
| 384 | MS6.1 | SQLite / immutable evidence pipeline / run layout | Отдельная папка каждого Run. |
| 385 | MS6.1 | SQLite / immutable evidence pipeline / run layout | `run.json`. |
| 386 | MS6.1 | SQLite / immutable evidence pipeline / run layout | `environment.json`. |
| 387 | MS6.1 | SQLite / immutable evidence pipeline / run layout | `build/`. |
| 388 | MS6.1 | SQLite / immutable evidence pipeline / run layout | `tests/`. |
| 389 | MS6.1 | SQLite / immutable evidence pipeline / run layout | `raw/`. |
| 390 | MS6.1 | SQLite / immutable evidence pipeline / run layout | `normalized/`. |
| 391 | MS6.1 | SQLite / immutable evidence pipeline / run layout | `metrics/`. |
| 392 | MS6.1 | SQLite / immutable evidence pipeline / run layout | `graphs/`. |
| 393 | MS6.1 | SQLite / immutable evidence pipeline / run layout | `findings/`. |
| 394 | MS6.1 | SQLite / immutable evidence pipeline / run layout | `agent/`. |
| 395 | MS6.7 | agent exports / bounded investigation | Agent readable export. |
| 396 | MS6.7 | agent exports / bounded investigation | `INDEX.md`. |
| 397 | MS6.7 | agent exports / bounded investigation | `RUN_SUMMARY.md`. |
| 398 | MS6.7 | agent exports / bounded investigation | `BUILD_DIAGNOSTICS.md`. |
| 399 | MS6.7 | agent exports / bounded investigation | `FAILURES.md`. |
| 400 | MS6.7 | agent exports / bounded investigation | `REGRESSIONS.md`. |
| 401 | MS6.7 | agent exports / bounded investigation | `ACTIONS.md`. |
| 402 | MS6.7 | agent exports / bounded investigation | `EXCEPTIONS.md`. |
| 403 | MS6.7 | agent exports / bounded investigation | `FUNCTION_PROFILE.md`. |
| 404 | MS6.7 | agent exports / bounded investigation | `CALL_GRAPH.md`. |
| 405 | MS6.7 | agent exports / bounded investigation | `THREADS.md`. |
| 406 | MS6.7 | agent exports / bounded investigation | `LOCKS.md`. |
| 407 | MS6.7 | agent exports / bounded investigation | `MEMORY.md`. |
| 408 | MS6.7 | agent exports / bounded investigation | `IO.md`. |
| 409 | MS6.7 | agent exports / bounded investigation | `COVERAGE.md`. |
| 410 | MS6.7 | agent exports / bounded investigation | `ARCHITECTURE.md`. |
| 411 | MS6.7 | agent exports / bounded investigation | `HOT_PATHS.md`. |
| 412 | MS6.7 | agent exports / bounded investigation | `IMPORTED_APIS.md`. |
| 413 | MS6.7 | agent exports / bounded investigation | `STATIC_SCAN.md`. |
| 414 | MS6.7 | agent exports / bounded investigation | `RAW_EVIDENCE.md`. |
| 415 | MS6.7 | agent exports / bounded investigation | `manifest.json`. |
| 416 | MS6.7 | agent exports / bounded investigation | Отдельные agent пакеты для конкретных failed tests. |
| 417 | MS6.7 | agent exports / bounded investigation | Отдельные agent пакеты для конкретных traces. |
| 418 | MS6.7 | agent exports / bounded investigation | Отдельные agent пакеты для конкретных Actions. |
| 419 | MS6.7 | agent exports / bounded investigation | Возможность агенту расследовать проблему по стабильным ID, не читая гигабайты raw trace. |
| 420 | MS6.7 | agent exports / bounded investigation | Автоматическое ограничение agent context только релевантными событиями. |
| 421 | MS6.8 | fault/stress injection | Fault injection. |
| 422 | MS6.8 | fault/stress injection | Искусственное замедление disk IO. |
| 423 | MS6.8 | fault/stress injection | Memory pressure. |
| 424 | MS6.8 | fault/stress injection | CPU pressure. |
| 425 | MS6.8 | fault/stress injection | Allocation failure injection. |
| 426 | MS6.8 | fault/stress injection | Worker scheduling pressure. |
| 427 | MS6.8 | fault/stress injection | Cancellation storms. |
| 428 | MS6.8 | fault/stress injection | Save во время интенсивной нагрузки. |
| 429 | MS6.8 | fault/stress injection | IO cancellation. |
| 430 | MS6.8 | fault/stress injection | Long-running stress tests. |
| 431 | MS6.8 | fault/stress injection | Тысячи повторов save/restore. |
| 432 | MS6.8 | fault/stress injection | Проверка persistent state под нагрузкой. |
| 433 | MS6.8 | fault/stress injection | Проверка async cancellation под нагрузкой. |
| 434 | MS6.8 | fault/stress injection | Проверка controlled degradation. |
| 435 | MS6.8 | fault/stress injection | Проверка causal chains под реальной нагрузкой. |
| 436 | MS6.8 | fault/stress injection | Проверка поведения движка при плохом окружении, а не только happy path. |
| 437 | MS6.9 | reuse in qualification campaigns | Использование Auditor для MS5 qualification. |
| 438 | MS6.9 | reuse in qualification campaigns | Использование Auditor для MS6 concurrency/lifetime qualification. |
| 439 | MS6.9 | reuse in qualification campaigns | Использование Auditor для MS7 causal chain qualification. |
| 440 | MS6.9 | reuse in qualification campaigns | Использование Auditor для MS8 load/degradation qualification. |
| 441 | MS6.9 | reuse in qualification campaigns | Использование Auditor для MS9 whole-engine qualification. |
| 442 | MS6.9 | reuse in qualification campaigns | Использование того же Auditor позже для самой игры. |
| 443 | MS6.9 | end-to-end independent audit capability / zero-law | Возможность открыть действие игрока и проследить путь от input через весь движок и gameplay. |
| 444 | MS6.9 | end-to-end independent audit capability / zero-law | Возможность увидеть каждую вызванную функцию в Deep режиме. |
| 445 | MS6.9 | end-to-end independent audit capability / zero-law | Возможность увидеть каждый переход между API. |
| 446 | MS6.9 | end-to-end independent audit capability / zero-law | Возможность увидеть, сколько миллисекунд занял каждый участок. |
| 447 | MS6.9 | end-to-end independent audit capability / zero-law | Возможность увидеть, какой поток выполнял конкретный участок. |
| 448 | MS6.9 | end-to-end independent audit capability / zero-law | Возможность увидеть, какие allocations произошли. |
| 449 | MS6.9 | end-to-end independent audit capability / zero-law | Возможность увидеть, какие исключения произошли и где были обработаны. |
| 450 | MS6.9 | end-to-end independent audit capability / zero-law | Возможность увидеть, какое IO произошло. |
| 451 | MS6.9 | end-to-end independent audit capability / zero-law | Возможность увидеть, где код ждал lock или scheduler. |
| 452 | MS6.9 | end-to-end independent audit capability / zero-law | Возможность увидеть реальную причинную цепочку действия. |
| 453 | MS6.9 | end-to-end independent audit capability / zero-law | Возможность найти архитектурные нарушения, которых нет в тестах. |
| 454 | MS6.9 | end-to-end independent audit capability / zero-law | Возможность найти работающий, но подозрительно устроенный код. |
| 455 | MS6.9 | end-to-end independent audit capability / zero-law | Возможность найти скрытые performance defects. |
| 456 | MS6.9 | end-to-end independent audit capability / zero-law | Возможность найти мёртвый код. |
| 457 | MS6.9 | end-to-end independent audit capability / zero-law | Возможность найти практически неиспользуемые API. |
| 458 | MS6.9 | end-to-end independent audit capability / zero-law | Возможность доказать, какие API реально используются. |
| 459 | MS6.9 | end-to-end independent audit capability / zero-law | Возможность независимо проверить заявления внутренних тестов Epidemic. |
| 460 | MS6.9 | end-to-end independent audit capability / zero-law | Главный принцип Auditor: считать Epidemic недоверенной системой и по возможности подтверждать её поведение независимыми внешними наблюдениями. |
