# MS6.3 — Managed processes, symbols, ETW and concurrency observation

Primary source requirements: 43–92

## Development cadence and stage-exit authority

Follow Layer 3 P.2–P.4 (plus P.5 for MS6.9): implement **large, coherent, end-to-end capability blocks** with their good/bad fixtures; run grouped focused tests **after each complete block**, not after each small edit. At stage end review the complete changed code/test/build surface and affected invariants, fix findings, freeze inputs, then run **one strong consolidated qualification**. Historical PASS/receipts do not establish current-source acceptance. Preserve required **Auditor product** raw/manifest/provenance semantics, but do not create development proof-of-proof infrastructure.

## Forbidden shortcuts

- Do not use README claims as qualification.
- Do not use a PASS string as sole proof.
- Do not hand-edit generated evidence to make a validator green.
- Do not remove a failing detector/fixture to close the stage.
- Do not treat missing telemetry as zero activity.
- Do not change production engine behavior merely to make Auditor observation easier.


## Exact data contracts

### Process
`ProcessRecord`:
PID, PPID, command line, cwd, environment hash/snapshot, start/end, exit code, parent RunId.

### Module
`ModuleRecord`:
path, base, size, version, binary hash, PDB identity.

### Thread
`ThreadRecord`:
ThreadId, OS TID, process, create/end, observed RUN/WAIT intervals.

### Wait/lock
`WaitRecord`:
thread, resource/lock identity when available, wait start/end, owner when available, acquisition/blocked stack.

## Exact requirements

### M6.3.1 Process tree
Child/grandchild processes are retained even if the root exits first.

### M6.3.2 Symbolization
Known fixture address resolves to expected function and source line.
Unknown symbols retain raw address/module evidence.

### M6.3.3 External timing
Wall time, CPU time and independent monotonic trace time are distinct fields.

### M6.3.4 Resource sampling
CPU, process-tree CPU, RSS, peak memory and thread count over time.

### M6.3.5 Thread lifecycle/timeline
Creation/destruction, RUN/WAIT, context switches and scheduler activity.

### M6.3.6 Concurrency analyzers
Detect candidates for starvation, hung workers, orphan jobs, work-after-owner-destruction, cancellation latency and late completion.

### M6.3.7 Wait/lock analysis
Wait/mutex/lock contention, owner, hold duration and acquisition stack where observable.

### M6.3.8 ETW/WPR
Store ETL raw; record WPR profile/command; make capture openable in WPA.

### M6.3.9 ETW normalization/xperf
Selected ETW/xperf facts are converted to normalized structured data with raw references.

### M6.3.10 Sampling profiler
CPU sample stacks produce flame-graph compatible data.

## Qualification fixtures

Bad/good pairs for:
- orphan child;
- worker starvation;
- long-held mutex;
- contention without deadlock;
- cancellation delayed vs timely;
- late completion vs timely completion.

## Final-stage adversarial focus

Final controls: real zero-loss ETW/WPA/xperf import and current-source thread/process lifetime match, corrupted/dropped trace as negative; bad PID reuse and WCT/SetEvent correlation rejected; symbol/source lines externally verified; good/bad mutex ownership/contended wait variants; no invented acquisition stack.

## Exit gate

Source requirements 43–92 qualified.
All concurrency heuristics retain confidence/provenance and raw references.

# Authoritative source-requirement checklist

These source obligations remain authoritative. Checking a box requires the stage proof described above; the checklist itself is not evidence.

- [ ] `SRC-043` — Независимый запуск каждого процесса самим Auditor.
- [ ] `SRC-044` — Сохранение PID и PPID.
- [ ] `SRC-045` — Сохранение command line процесса.
- [ ] `SRC-046` — Сохранение working directory.
- [ ] `SRC-047` — Сохранение environment.
- [ ] `SRC-048` — Сохранение exit code.
- [ ] `SRC-049` — Отслеживание дерева дочерних процессов.
- [ ] `SRC-050` — Сохранение времени создания и завершения процессов.
- [ ] `SRC-051` — Список загруженных DLL и EXE modules.
- [ ] `SRC-052` — Версии загруженных модулей.
- [ ] `SRC-053` — PDB symbolization.
- [ ] `SRC-054` — Преобразование адресов в имена функций.
- [ ] `SRC-055` — Преобразование адресов в source file и line.
- [ ] `SRC-056` — Внешнее измерение wall clock времени.
- [ ] `SRC-057` — Внешнее измерение CPU time.
- [ ] `SRC-058` — Использование независимого monotonic profiling clock.
- [ ] `SRC-059` — Отдельное понимание Game Time, Simulation Time, Wall Time и Trace Time.
- [ ] `SRC-060` — CPU usage процесса.
- [ ] `SRC-061` — CPU usage всего дерева процессов.
- [ ] `SRC-062` — RSS памяти.
- [ ] `SRC-063` — Peak memory.
- [ ] `SRC-064` — Количество потоков.
- [ ] `SRC-065` — История CPU во времени.
- [ ] `SRC-066` — История использования памяти во времени.
- [ ] `SRC-067` — Thread timeline.
- [ ] `SRC-068` — Состояния RUN и WAIT для потоков.
- [ ] `SRC-069` — Context switches.
- [ ] `SRC-070` — Scheduler activity.
- [ ] `SRC-071` — Thread creation и destruction.
- [ ] `SRC-072` — Время работы каждого worker thread.
- [ ] `SRC-073` — Поиск thread starvation.
- [ ] `SRC-074` — Поиск зависших worker threads.
- [ ] `SRC-075` — Поиск orphan jobs.
- [ ] `SRC-076` — Поиск работы, продолжающейся после уничтожения owner.
- [ ] `SRC-077` — Анализ cancellation latency.
- [ ] `SRC-078` — Анализ late completion.
- [ ] `SRC-079` — Анализ waits.
- [ ] `SRC-080` — Анализ mutex contention.
- [ ] `SRC-081` — Анализ lock contention.
- [ ] `SRC-082` — Определение владельца lock.
- [ ] `SRC-083` — Время удержания lock.
- [ ] `SRC-084` — Определение call stack, где lock был захвачен.
- [ ] `SRC-085` — ETW capture.
- [ ] `SRC-086` — WPR capture.
- [ ] `SRC-087` — Возможность открытия trace в WPA.
- [ ] `SRC-088` — Автоматическая конвертация части ETW данных в текстовый формат.
- [ ] `SRC-089` — xperf отчёты.
- [ ] `SRC-090` — Sampling profiler.
- [ ] `SRC-091` — CPU sampling call stacks.
- [ ] `SRC-092` — Flame graph.

Source obligations owned by this stage: **50**.
