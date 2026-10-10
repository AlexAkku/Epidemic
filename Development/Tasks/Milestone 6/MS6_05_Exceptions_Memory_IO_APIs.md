# MS6.5 — Exceptions, memory safety, IO and OS/API observation

Primary source requirements: 171–227

## Development cadence and stage-exit authority

Follow Layer 3 P.2–P.4 (plus P.5 for MS6.9): implement **large, coherent, end-to-end capability blocks** with their good/bad fixtures; run grouped focused tests **after each complete block**, not after each small edit. At stage end review the complete changed code/test/build surface and affected invariants, fix findings, freeze inputs, then run **one strong consolidated qualification**. Historical PASS/receipts do not establish current-source acceptance. Preserve required **Auditor product** raw/manifest/provenance semantics, but do not create development proof-of-proof infrastructure.

## Forbidden shortcuts

- Do not use README claims as qualification.
- Do not use a PASS string as sole proof.
- Do not hand-edit generated evidence to make a validator green.
- Do not remove a failing detector/fixture to close the stage.
- Do not treat missing telemetry as zero activity.
- Do not change production engine behavior merely to make Auditor observation easier.


## Exceptions/crash

### M6.5.1 Native exception events
Capture first chance, second chance, code, address, thread and stack.

### M6.5.2 Handled/unhandled
Classify only when debugger/control-flow evidence supports it.
Unknown remains unknown.

### M6.5.3 Exception frequency/cost
Count repeated exceptions and associate CPU/time cost where measurement supports it.

### M6.5.4 Crash/assert/contract
Capture crash stack, asserts and contract violations.

### M6.5.5 Recovery path
Record post-error execution/recovery continuation when target survives.

## Memory

### M6.5.6 Allocation/deallocation
Capture allocation address/id, size, timestamp, thread and stack.

### M6.5.7 Lifetime/metrics
Allocation lifetime/rate, function association, Action association, peak heap.

### M6.5.8 Safety detections
Leak, UAF, double free, OOB and heap corruption.

### M6.5.9 External profiles
AddressSanitizer and Application Verifier profiles have explicit run recipes and parsed results.

## IO/network/API

### M6.5.10 File IO
Open, close, read, write, seek, mmap and directory enumeration.

### M6.5.11 IO correlation
Associate IO with call stack, thread, TraceId and ActionId when available.

### M6.5.12 IO analysis
Synchronous main-thread IO, per-function IO stats and disk latency.

### M6.5.13 Network
Record network activity at the supported external observation level.

### M6.5.14 Win32/API families
File, memory, synchronization, thread, socket, window and current graphics API where enabled.

### M6.5.15 Import vs observed calls
PE import inventory is distinct from runtime-observed API calls.

### M6.5.16 Deep API graph
Where capture technology supports runtime API observation, build system-API call graph with evidence references.

## Qualification fixtures

- handled exception;
- unhandled exception/crash;
- repeated exception storm;
- assert/contract event;
- intentional leak;
- UAF/double-free/OOB fixture under supported safety tool;
- synchronous main-thread file IO;
- async/background IO good variant;
- imported-but-never-called API vs actually called API.

## Final-stage adversarial focus

Final controls: handled/unhandled exception and crash case; real allocation lifetime/leak and false-positive controls; IO completion/cancellation/slow path; import-only API versus actually observed API calls; tool/provenance downgrades and unavailable observation never masquerade as true clean findings.

## Exit gate

Source requirements 171–227 qualified.
No memory/API finding may claim stronger provenance than the underlying tool provides.

# Authoritative source-requirement checklist

These source obligations remain authoritative. Checking a box requires the stage proof described above; the checklist itself is not evidence.

- [ ] `SRC-171` — Exception monitoring.
- [ ] `SRC-172` — Windows Debug API.
- [ ] `SRC-173` — First chance exceptions.
- [ ] `SRC-174` — Second chance exceptions.
- [ ] `SRC-175` — Exception code.
- [ ] `SRC-176` — Exception address.
- [ ] `SRC-177` — Thread, в котором произошло исключение.
- [ ] `SRC-178` — Stack trace исключения.
- [ ] `SRC-179` — Определение обработанного исключения.
- [ ] `SRC-180` — Определение необработанного исключения.
- [ ] `SRC-181` — Поиск постоянно выбрасываемых и ловимых исключений.
- [ ] `SRC-182` — Статистика количества исключений.
- [ ] `SRC-183` — Оценка CPU стоимости repeated exceptions.
- [ ] `SRC-184` — Crash capture.
- [ ] `SRC-185` — Stack trace падения.
- [ ] `SRC-186` — Assert capture.
- [ ] `SRC-187` — Contract violation capture.
- [ ] `SRC-188` — Сохранение recovery path после ошибки.
- [ ] `SRC-189` — Memory allocation tracing.
- [ ] `SRC-190` — Deallocation tracing.
- [ ] `SRC-191` — Allocation stack.
- [ ] `SRC-192` — Размер allocation.
- [ ] `SRC-193` — Lifetime allocation.
- [ ] `SRC-194` — Allocation rate.
- [ ] `SRC-195` — Allocations конкретной функции.
- [ ] `SRC-196` — Allocations конкретного игрового действия.
- [ ] `SRC-197` — Peak heap.
- [ ] `SRC-198` — Поиск memory leaks.
- [ ] `SRC-199` — Use after free detection.
- [ ] `SRC-200` — Double free detection.
- [ ] `SRC-201` — Out of bounds detection.
- [ ] `SRC-202` — Heap corruption detection.
- [ ] `SRC-203` — AddressSanitizer profile.
- [ ] `SRC-204` — Application Verifier profile.
- [ ] `SRC-205` — IO tracing.
- [ ] `SRC-206` — File open.
- [ ] `SRC-207` — File close.
- [ ] `SRC-208` — File read.
- [ ] `SRC-209` — File write.
- [ ] `SRC-210` — File seek.
- [ ] `SRC-211` — Memory mapped files.
- [ ] `SRC-212` — Directory enumeration.
- [ ] `SRC-213` — Привязка IO операции к call stack.
- [ ] `SRC-214` — Поиск synchronous IO на Main Thread.
- [ ] `SRC-215` — Статистика file IO по функции.
- [ ] `SRC-216` — Disk latency.
- [ ] `SRC-217` — Network activity.
- [ ] `SRC-218` — Отслеживание выбранных Win32 API.
- [ ] `SRC-219` — File API calls.
- [ ] `SRC-220` — Memory API calls.
- [ ] `SRC-221` — Synchronization API calls.
- [ ] `SRC-222` — Thread API calls.
- [ ] `SRC-223` — Socket API calls.
- [ ] `SRC-224` — Window API calls.
- [ ] `SRC-225` — При необходимости GPU API calls.
- [ ] `SRC-226` — Imported API inventory для EXE/DLL.
- [ ] `SRC-227` — Call graph системных API в Deep режиме.

Source obligations owned by this stage: **57**.
