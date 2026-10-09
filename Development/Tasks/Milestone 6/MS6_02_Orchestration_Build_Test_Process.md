# MS6.2 — GUI/backend orchestration, CMake/build and CTest control

Primary source requirements: 1–42

## Historical implementation and qualification (not current-source PASS)

An earlier workspace run qualified the MS6.2 orchestration behaviors, including owned-process Stop/Cancel and GUI/headless backend parity. Its prior `_goal6_handoff/ms6_2/` receipt and `tools/ms6/orchestration_closure.py` ownership chain are **obsolete after repository cleanup** and are not required to be recreated. Existing regression tests (including `test_build_run_stop`) and the actual backend are the current verification surface. At final current-source qualification, rerun the applicable owned-process cancellation, build/test orchestration, GUI/headless parity and integration cases; do not trust historical PASS text or regenerate obsolete evidence containers.

## Development cadence and stage-exit authority

Follow Layer 3 P.2–P.4 (plus P.5 for MS6.9): implement **large, coherent, end-to-end capability blocks** with their good/bad fixtures; run grouped focused tests **after each complete block**, not after each small edit. At stage end review the complete changed code/test/build surface and affected invariants, fix findings, freeze inputs, then run **one strong consolidated qualification**. Historical PASS/receipts do not establish current-source acceptance. Preserve required **Auditor product** raw/manifest/provenance semantics, but do not create development proof-of-proof infrastructure.

## Forbidden shortcuts

- Do not use README claims as qualification.
- Do not use a PASS string as sole proof.
- Do not hand-edit generated evidence to make a validator green.
- Do not remove a failing detector/fixture to close the stage.
- Do not treat missing telemetry as zero activity.
- Do not change production engine behavior merely to make Auditor observation easier.


## Required user operations

- Configure
- Build
- Build + Run
- Run All Tests
- Run one test
- Run selected tests
- Run group by label/subsystem
- Discover Tests
- Start Game
- Stop

The same backend operation must be callable without GUI.

## EXE acceptance gate

PASS только если:

1. Clean build создаёт `EpidemicAuditor.exe`.
2. `.exe` запускается на Windows 11 без ручного запуска Python/PowerShell.
3. Открывается GUI.
4. Через GUI можно:
   - выбрать Epidemic root;
   - Configure;
   - Build;
   - Discover Tests;
   - запустить один тест;
   - запустить группу;
   - запустить игру;
   - открыть завершённый Run.
5. `.exe` может автоматически запускать необходимые helper processes.
6. Закрытие приложения корректно завершает принадлежащие ему процессы либо явно сохраняет detached run согласно contract.
7. Fresh-machine/deployment smoke подтверждает, что пользовательский entry point — именно `.exe`.
8. Финальный MS6 gate проверяет наличие и запускаемость `EpidemicAuditor.exe`.

Без этого `GOAL6_AUDITOR_COMPLETE` запрещён.

## Exact requirements

### M6.2.1 Test discovery
CTest discovery stores:
`TestId`, name, labels, executable, args, cwd, timeout and source location when derivable.

### M6.2.2 Test history
For each compatible TestId store result history, duration history and stability statistics.

### M6.2.3 Mode selection
GUI/backend expose Fast, Full, Deep, Memory, Coverage and Stress.
Targeted Exact is available as a Deep subprofile with an explicit target scope.

### M6.2.4 Build isolation
Separate audit build directories.
Normal engine builds do not inherit Auditor instrumentation options.

### M6.2.5 Project discovery
Detect CMake source root and record why it was selected.
Ambiguous roots require explicit user choice rather than guessing.

### M6.2.6 Build receipt
Store exact:
- configure command;
- build command;
- compiler flags;
- defines;
- compiler version/path;
- toolchain;
- optimization;
- instrumentation;
- parallelism;
- configure/compile/link timings where available.

### M6.2.7 Structured diagnostics
Normalize warnings, compiler errors and linker errors.
Create one build finding for the first actionable root cause while retaining all diagnostics.

### M6.2.8 PDB/parallel class
Controlled fixture reproduces a PDB/C1090-style or equivalent parallel-build failure class.
Record compiler process count and parallelism at failure.

### M6.2.9 Reproduction matrix
Run at least two configured parallelism levels as separate attempts with separate Run/Build IDs.

### M6.2.10 Stop/cancel
Stop must terminate only the managed run/process tree and record termination provenance/reason.

## Positive fixtures

- known-good tiny CMake project configure/build;
- known-good CTest catalog with labels;
- one selected test;
- selected group;
- all tests;
- start/stop synthetic game process.

## Negative fixtures

- CMake configure failure;
- compiler syntax error;
- linker unresolved external;
- missing executable;
- PDB/parallel failure class;
- stop tree with live child process.

## Final-stage adversarial focus

Final controls: fresh configure/build/test discovery with selected/single/all tests, GUI/headless parity, nonzero build/test failure preserved, timeout/stop cancels only owned descendants, PDB/parallel failure classification, no game run after Build+Run cancellation.

## Exit gate

Source requirements 1–42 all have PASS evidence.
GUI and headless receipts point to the same backend operation IDs.

# Authoritative source-requirement checklist

These source obligations remain authoritative. Checking a box requires the stage proof described above; the checklist itself is not evidence.

- [ ] `SRC-001` — GUI для управления Auditor.
- [ ] `SRC-002` — Кнопка Configure.
- [ ] `SRC-003` — Кнопка Build.
- [ ] `SRC-004` — Кнопка Build + Run.
- [ ] `SRC-005` — Кнопка Run All Tests.
- [ ] `SRC-006` — Запуск одного конкретного теста.
- [ ] `SRC-007` — Выбор нескольких тестов и запуск выбранной группы.
- [ ] `SRC-008` — Запуск заранее определённых групп тестов по тегам и подсистемам.
- [ ] `SRC-009` — Автоматическое обнаружение CTest тестов.
- [ ] `SRC-010` — Каталог всех тестов с именем, группой, тегами, executable, аргументами и source location.
- [ ] `SRC-011` — История результатов каждого теста.
- [ ] `SRC-012` — История времени выполнения каждого теста.
- [ ] `SRC-013` — Статистика стабильности теста по предыдущим запускам.
- [ ] `SRC-014` — Кнопка запуска самой игры.
- [ ] `SRC-015` — Возможность остановить запущенный тест или игру.
- [ ] `SRC-016` — Несколько режимов аудита: Fast, Full, Deep, Memory, Coverage, Stress.
- [ ] `SRC-017` — Отдельная Audit сборка.
- [ ] `SRC-018` — Отдельная Deep Audit сборка.
- [ ] `SRC-019` — Независимая от обычного Debug/Release папка сборки.
- [ ] `SRC-020` — Возможность использовать Auditor, не изменяя production build.
- [ ] `SRC-021` — Возможность вообще не коммитить Auditor в репозиторий Epidemic.
- [ ] `SRC-022` — Возможность держать Auditor отдельным Git repository внутри папки проекта.
- [ ] `SRC-023` — Автоматическое определение структуры CMake проекта.
- [ ] `SRC-024` — Сохранение точных команд CMake configure.
- [ ] `SRC-025` — Сохранение точных команд build.
- [ ] `SRC-026` — Сохранение compiler flags.
- [ ] `SRC-027` — Сохранение defines.
- [ ] `SRC-028` — Сохранение версии компилятора.
- [ ] `SRC-029` — Сохранение используемого toolchain.
- [ ] `SRC-030` — Сохранение параметров оптимизации.
- [ ] `SRC-031` — Сохранение параметров instrumentation.
- [ ] `SRC-032` — Время CMake configure.
- [ ] `SRC-033` — Время компиляции.
- [ ] `SRC-034` — Время линковки.
- [ ] `SRC-035` — Структурированный сбор compiler warnings.
- [ ] `SRC-036` — Структурированный сбор compiler errors.
- [ ] `SRC-037` — Структурированный сбор linker errors.
- [ ] `SRC-038` — Автоматическая классификация ошибок сборки.
- [ ] `SRC-039` — Отдельный finding для первой причины падения сборки.
- [ ] `SRC-040` — Обнаружение проблем типа MSVC C1090/PDB.
- [ ] `SRC-041` — Фиксация количества параллельно работающих compiler processes при ошибке.
- [ ] `SRC-042` — Возможность автоматически прогонять reproduction matrix с разной параллельностью сборки.

Source obligations owned by this stage: **42**.
