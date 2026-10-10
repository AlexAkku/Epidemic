# MS6.4 — Exact function tracing, coverage and architecture verification

Primary source requirements: 93–170

## Development cadence and stage-exit authority

Follow Layer 3 P.2–P.4 (plus P.5 for MS6.9): implement **large, coherent, end-to-end capability blocks** with their good/bad fixtures; run grouped focused tests **after each complete block**, not after each small edit. At stage end review the complete changed code/test/build surface and affected invariants, fix findings, freeze inputs, then run **one strong consolidated qualification**. Historical PASS/receipts do not establish current-source acceptance. Preserve required **Auditor product** raw/manifest/provenance semantics, but do not create development proof-of-proof infrastructure.

## Forbidden shortcuts

- Do not use README claims as qualification.
- Do not use a PASS string as sole proof.
- Do not hand-edit generated evidence to make a validator green.
- Do not remove a failing detector/fixture to close the stage.
- Do not treat missing telemetry as zero activity.
- Do not change production engine behavior merely to make Auditor observation easier.


## Exact trace requirements

### M6.4.1 Instrumented build
MSVC Deep profile adds `/Gh` and `/GH`.
Auditor hook runtime is compiled without recursive exact instrumentation.

### M6.4.2 Raw event
Each event includes:
sequence, QPC/monotonic timestamp, ENTER/EXIT, PID, TID, address.

### M6.4.3 Overflow
Fixed/managed buffer exhaustion increments a dropped counter.
Dropped > 0 forces `complete=false`.

### M6.4.4 Call reconstruction
Pair ENTER/EXIT per process/thread.
Unmatched records are counted, not silently discarded from health reporting.

### M6.4.5 Function metrics
Calls, callers, callees, self/inclusive, min/max/mean/p50/p95/p99, thread distribution.

### M6.4.6 History
Run count observed, tests using function, lifetime calls, performance history/regression, never/once-used candidates.

## Coverage requirements

### M6.4.7 Coverage dimensions
Function, line, branch.

### M6.4.8 Coverage scopes
Single test, group, full qualification, merged runs.

### M6.4.9 Heatmap/source comparison
Static source inventory can be compared to executed/covered code without equating non-coverage with dead code.

## Architecture requirements

### M6.4.10 Static graphs
Include, module, namespace, public API and class dependencies.

### M6.4.11 Static analyses
Cycles, forbidden dependencies, large functions, complexity, ownership smells, unused and duplicate code.

### M6.4.12 Runtime graph
Module-to-module observed calls with counts and cost.

### M6.4.13 Static/runtime comparison
Unexpected runtime dependency and rarely/never-used API candidates.

### M6.4.14 Rules
Validate and execute:
`architecture.yaml`, `concurrency.yaml`, `persistence.yaml`, `performance.yaml`, `memory.yaml`, `io.yaml`.

## Qualification fixtures

- known exact call tree with recursion and siblings;
- multithreaded call tree;
- forced trace overflow;
- known branch coverage project;
- known include cycle;
- forbidden module edge;
- runtime unexpected edge;
- allowed edge good variant.

## Final-stage adversarial focus

Final controls: real Audit/Deep `/Gh`/`/GH` positive instrumentation and normal Debug/Release negative binary/link check; balanced/unbalanced function enter/exit, overflow and incomplete trace negative cases; known covered/uncovered paths; static/runtime architecture violation and valid-control graph.

## Exit gate

Source requirements 93–170 qualified.
Exact trace health has no unreported loss.

# Authoritative source-requirement checklist

These source obligations remain authoritative. Checking a box requires the stage proof described above; the checklist itself is not evidence.

- [ ] `SRC-093` — Exact function instrumentation.
- [ ] `SRC-094` — MSVC `/Gh` instrumentation.
- [ ] `SRC-095` — MSVC `/GH` instrumentation.
- [ ] `SRC-096` — Собственный `_penter` hook Auditor.
- [ ] `SRC-097` — Собственный `_pexit` hook Auditor.
- [ ] `SRC-098` — Запись каждого function enter.
- [ ] `SRC-099` — Запись каждого function exit.
- [ ] `SRC-100` — Timestamp каждого входа и выхода.
- [ ] `SRC-101` — PID каждого вызова.
- [ ] `SRC-102` — Thread ID каждого вызова.
- [ ] `SRC-103` — Адрес функции каждого вызова.
- [ ] `SRC-104` — Exact call count.
- [ ] `SRC-105` — Exact call tree.
- [ ] `SRC-106` — Caller → callee graph.
- [ ] `SRC-107` — Self time функции.
- [ ] `SRC-108` — Inclusive time функции.
- [ ] `SRC-109` — Minimum execution time.
- [ ] `SRC-110` — Maximum execution time.
- [ ] `SRC-111` — Mean execution time.
- [ ] `SRC-112` — Median p50.
- [ ] `SRC-113` — p95.
- [ ] `SRC-114` — p99.
- [ ] `SRC-115` — Количество callers функции.
- [ ] `SRC-116` — Количество callees функции.
- [ ] `SRC-117` — Распределение функции по потокам.
- [ ] `SRC-118` — Hot functions.
- [ ] `SRC-119` — Hot call paths.
- [ ] `SRC-120` — Самые дорогие функции по total CPU.
- [ ] `SRC-121` — Самые дорогие функции по self CPU.
- [ ] `SRC-122` — Функции с наибольшим количеством вызовов.
- [ ] `SRC-123` — Функции с редкими патологическими spikes.
- [ ] `SRC-124` — Долговременная база статистики функций.
- [ ] `SRC-125` — Количество запусков, в которых функция наблюдалась.
- [ ] `SRC-126` — Количество тестов, которые реально используют функцию.
- [ ] `SRC-127` — Общее количество вызовов функции за всю историю Auditor.
- [ ] `SRC-128` — История performance функции.
- [ ] `SRC-129` — Определение performance regression конкретной функции.
- [ ] `SRC-130` — Поиск функций, которые вообще никогда не выполнялись.
- [ ] `SRC-131` — Поиск функций, которые выполнялись только один раз.
- [ ] `SRC-132` — Code coverage.
- [ ] `SRC-133` — Function coverage.
- [ ] `SRC-134` — Line coverage.
- [ ] `SRC-135` — Branch coverage.
- [ ] `SRC-136` — Объединение coverage нескольких запусков.
- [ ] `SRC-137` — Coverage по одному тесту.
- [ ] `SRC-138` — Coverage по группе тестов.
- [ ] `SRC-139` — Coverage всего qualification suite.
- [ ] `SRC-140` — Coverage heatmap.
- [ ] `SRC-141` — Сравнение существующего source code с реально исполнявшимся кодом.
- [ ] `SRC-142` — Static source analysis.
- [ ] `SRC-143` — Include graph.
- [ ] `SRC-144` — Module dependency graph.
- [ ] `SRC-145` — Namespace dependency graph.
- [ ] `SRC-146` — Public API graph.
- [ ] `SRC-147` — Class dependency graph.
- [ ] `SRC-148` — Поиск циклических зависимостей.
- [ ] `SRC-149` — Поиск forbidden dependencies.
- [ ] `SRC-150` — Поиск слишком больших функций.
- [ ] `SRC-151` — Complexity analysis.
- [ ] `SRC-152` — Поиск подозрительных ownership patterns.
- [ ] `SRC-153` — Поиск unused code.
- [ ] `SRC-154` — Поиск duplicate code.
- [ ] `SRC-155` — Static architecture graph.
- [ ] `SRC-156` — Runtime architecture graph.
- [ ] `SRC-157` — Сопоставление static и runtime архитектуры.
- [ ] `SRC-158` — Отображение количества runtime вызовов между подсистемами.
- [ ] `SRC-159` — Отображение CPU стоимости взаимодействия между подсистемами.
- [ ] `SRC-160` — Поиск неожиданных runtime dependencies.
- [ ] `SRC-161` — Runtime architecture verifier.
- [ ] `SRC-162` — Проверка правил вроде `Renderer must not call QuestSystem`.
- [ ] `SRC-163` — Отдельный rules engine.
- [ ] `SRC-164` — `architecture.yaml`.
- [ ] `SRC-165` — `concurrency.yaml`.
- [ ] `SRC-166` — `persistence.yaml`.
- [ ] `SRC-167` — `performance.yaml`.
- [ ] `SRC-168` — `memory.yaml`.
- [ ] `SRC-169` — `io.yaml`.
- [ ] `SRC-170` — Автоматическое формирование findings по нарушениям правил.

Source obligations owned by this stage: **78**.
