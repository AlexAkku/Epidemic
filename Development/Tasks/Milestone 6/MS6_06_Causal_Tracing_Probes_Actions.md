# MS6.6 — Causal tracing, shadow probes, Actions and scenarios

Primary source requirements: 228–298

## Development cadence and stage-exit authority

Follow Layer 3 P.2–P.4 (plus P.5 for MS6.9): implement **large, coherent, end-to-end capability blocks** with their good/bad fixtures; run grouped focused tests **after each complete block**, not after each small edit. At stage end review the complete changed code/test/build surface and affected invariants, fix findings, freeze inputs, then run **one strong consolidated qualification**. Historical PASS/receipts do not establish current-source acceptance. Preserve required **Auditor product** raw/manifest/provenance semantics, but do not create development proof-of-proof infrastructure.

## Forbidden shortcuts

- Do not use README claims as qualification.
- Do not use a PASS string as sole proof.
- Do not hand-edit generated evidence to make a validator green.
- Do not remove a failing detector/fixture to close the stage.
- Do not treat missing telemetry as zero activity.
- Do not change production engine behavior merely to make Auditor observation easier.


## Causal identity

### M6.6.1 Required IDs
TraceId, SpanId, ParentSpanId, EventId, CallId, FindingId, FunctionId, ModuleId, EntityId, JobId, CommandId.

### M6.6.2 Explicit cross-thread edges
Implement correlation for:
- Job submit -> execute;
- EventBus publish -> handler;
- command dispatch -> execution;
- coroutine/async continuation;
- async IO;
- physics callback;
- render command when a correlation mechanism exists.

### M6.6.3 No temporal invention
A missing correlation ID produces `edge_unresolved`, not a guessed causal edge.

### M6.6.4 DAG and call tree separation
Causal graph answers "what caused what".
Call tree answers "which functions executed it".
Both reference each other through IDs but remain distinct structures.

## Shadow probes

### M6.6.5 Probe families
JobSystem, EventBus, CommandBus, async/coroutines, persistence transactions, entity lifecycle and semantic state changes.

### M6.6.6 Build isolation
Probe symbols/dependencies are absent from production build verification.

### M6.6.7 Provenance
All probe facts are `engine_probe`; external facts remain separate.

## Actions

### M6.6.8 Action model
ActionId, name, start/end in trace clock, RunId.

### M6.6.9 Action query
Returns function profile, call tree, exceptions, allocations, threads, IO, causal graph and state changes.

### M6.6.10 Arm next input
A configured next input creates the Action interval automatically.

## Rolling/autocapture

### M6.6.11 Rolling buffer
Configurable N seconds before/after.

### M6.6.12 Automatic triggers
Crash, assert, contract violation, long frame, suspected deadlock, memory error.

## Scenarios

### M6.6.13 Non-CTest scenarios
Pickup item, save/load, combat, menu and streaming each get ScenarioId and independent capture.

## Qualification fixtures

- job -> worker -> event -> handler chain;
- missing correlation negative case;
- coroutine continuation;
- persistence transaction probe;
- entity lifecycle/state-change probe;
- Action marked manually;
- Arm-next-input;
- rolling crash capture with pre-event evidence.

## Final-stage adversarial focus

Final controls: known multi-thread causal DAG with explicit handoff IDs; unrelated temporally adjacent events stay disconnected; dropped parent causes incomplete linkage; Audit-only shadow probes; Action start/stop/rolling pre-trigger scenario proves causal provenance without fabricated edge.

## Exit gate

Source requirements 228–298 qualified.
At least one multi-thread causal DAG exactly matches a controlled expected graph.

# Authoritative source-requirement checklist

These source obligations remain authoritative. Checking a box requires the stage proof described above; the checklist itself is not evidence.

- [ ] `SRC-228` — Causal tracing.
- [ ] `SRC-229` — Отдельный `TraceId`.
- [ ] `SRC-230` — `SpanId`.
- [ ] `SRC-231` — `ParentSpanId`.
- [ ] `SRC-232` — `EventId`.
- [ ] `SRC-233` — `CallId`.
- [ ] `SRC-234` — `FindingId`.
- [ ] `SRC-235` — `FunctionId`.
- [ ] `SRC-236` — `ModuleId`.
- [ ] `SRC-237` — `EntityId`.
- [ ] `SRC-238` — `JobId`.
- [ ] `SRC-239` — `CommandId`.
- [ ] `SRC-240` — Связывание async работы с породившим её действием.
- [ ] `SRC-241` — Связывание JobSystem Submit с Execute.
- [ ] `SRC-242` — Связывание EventBus publish с обработчиками.
- [ ] `SRC-243` — Связывание command dispatch с выполнением.
- [ ] `SRC-244` — Связывание coroutine continuation.
- [ ] `SRC-245` — Связывание async IO.
- [ ] `SRC-246` — Связывание physics callbacks.
- [ ] `SRC-247` — Связывание render commands.
- [ ] `SRC-248` — Полный causal DAG.
- [ ] `SRC-249` — Отдельный call tree внутри каждого causal node.
- [ ] `SRC-250` — Разделение понятий «что произошло» и «какие функции это выполнили».
- [ ] `SRC-251` — Optional Audit probes.
- [ ] `SRC-252` — Probe для JobSystem.
- [ ] `SRC-253` — Probe для EventBus.
- [ ] `SRC-254` — Probe для CommandBus.
- [ ] `SRC-255` — Probe для async/coroutines.
- [ ] `SRC-256` — Probe для Persistence transactions.
- [ ] `SRC-257` — Probe для Entity lifecycle.
- [ ] `SRC-258` — Возможность применять probes только в shadow Audit build.
- [ ] `SRC-259` — Отсутствие probes в production build.
- [ ] `SRC-260` — Маркировка происхождения каждого факта.
- [ ] `SRC-261` — `external`.
- [ ] `SRC-262` — `compiler_instrumentation`.
- [ ] `SRC-263` — `static_analysis`.
- [ ] `SRC-264` — `engine_probe`.
- [ ] `SRC-265` — Уровень доверия `observed`.
- [ ] `SRC-266` — Уровень доверия `derived`.
- [ ] `SRC-267` — Уровень доверия `declared`.
- [ ] `SRC-268` — Уровень доверия `heuristic`.
- [ ] `SRC-269` — Interactive event log всей игры.
- [ ] `SRC-270` — Возможность выбрать конкретное пользовательское действие.
- [ ] `SRC-271` — `Action Start`.
- [ ] `SRC-272` — `Action End`.
- [ ] `SRC-273` — Имя action, например `Pickup Item`.
- [ ] `SRC-274` — Выделение exact trace только для выбранного временного интервала.
- [ ] `SRC-275` — Профиль функций конкретного action.
- [ ] `SRC-276` — Call tree конкретного action.
- [ ] `SRC-277` — Exceptions конкретного action.
- [ ] `SRC-278` — Allocations конкретного action.
- [ ] `SRC-279` — Threads конкретного action.
- [ ] `SRC-280` — IO конкретного action.
- [ ] `SRC-281` — В будущем state changes конкретного action.
- [ ] `SRC-282` — Возможность режима `Arm next input`.
- [ ] `SRC-283` — Rolling trace buffer.
- [ ] `SRC-284` — Сохранение N секунд до события.
- [ ] `SRC-285` — Сохранение N секунд после события.
- [ ] `SRC-286` — Автоматический capture при crash.
- [ ] `SRC-287` — Автоматический capture при assert.
- [ ] `SRC-288` — Автоматический capture при contract violation.
- [ ] `SRC-289` — Автоматический capture при слишком длинном frame.
- [ ] `SRC-290` — Автоматический capture при подозрении на deadlock.
- [ ] `SRC-291` — Автоматический capture при memory error.
- [ ] `SRC-292` — Сценарии, не являющиеся unit tests.
- [ ] `SRC-293` — Сценарий поднятия предмета.
- [ ] `SRC-294` — Сценарий save/load.
- [ ] `SRC-295` — Сценарий боя.
- [ ] `SRC-296` — Сценарий входа в меню.
- [ ] `SRC-297` — Сценарий streaming.
- [ ] `SRC-298` — Возможность сохранять Scenario Capture отдельно от тестов.

Source obligations owned by this stage: **71**.
