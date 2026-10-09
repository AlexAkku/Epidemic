# MS6.9 — Auditor self-qualification and Epidemic acceptance audit

Primary source requirements: 437–460  
Final closure for all 1–460.

## Development cadence and stage-exit authority

Follow Layer 3 P.2–P.4 (plus P.5 for MS6.9): implement **large, coherent, end-to-end capability blocks** with their good/bad fixtures; run grouped focused tests **after each complete block**, not after each small edit. At stage end review the complete changed code/test/build surface and affected invariants, fix findings, freeze inputs, then run **one strong consolidated qualification**. Historical PASS/receipts do not establish current-source acceptance. Preserve required **Auditor product** raw/manifest/provenance semantics, but do not create development proof-of-proof infrastructure.

## Forbidden shortcuts

- Do not use README claims as qualification.
- Do not use a PASS string as sole proof.
- Do not hand-edit generated evidence to make a validator green.
- Do not remove a failing detector/fixture to close the stage.
- Do not treat missing telemetry as zero activity.
- Do not change production engine behavior merely to make Auditor observation easier.


## Roadmap relabeling

Original requirements 437–442 mention old milestone numbers.
Do not drop them.

Map capability usage to the new order:

- retrospective Goal/MS5 persistence+determinism verification now;
- new MS7 hardening;
- new MS8 integration/test-truthfulness;
- new MS9 destructive/stress;
- later game usage.

The capability is qualified in MS6; full later campaigns execute in their own milestones.

## Exact final requirements

### M6.9.1 Retrospective independent MS5 check
Select representative persistence/determinism claims and verify them through Auditor external/compiler evidence.
Any contradiction becomes an explicit blocker/input to MS7.

### M6.9.2 Full Deep Action
Demonstrate one Action from input through:
- every observable function in targeted exact scope;
- API transitions;
- per-segment timings;
- threads;
- allocations;
- exceptions;
- IO;
- waits/locks/scheduler;
- causal chain.

### M6.9.3 Architecture outside tests
Controlled rule violation not covered by target tests is still found by Auditor.

### M6.9.4 Suspicious working code
Static/rules fixture with behaviorally working but architecturally suspicious pattern produces a heuristic finding, not a false correctness failure.

### M6.9.5 Hidden performance defect
Known hot-path regression is detected in compatible run comparison.

### M6.9.6 Dead/rare API
Controlled never-called and rarely-called functions are distinguished.

### M6.9.7 Real API usage
A known used API has runtime evidence proving use.

### M6.9.8 Independent test-claim verification
At least one internal test PASS claim is checked against external observation; the fixture includes a deceptive internal PASS variant that external evidence rejects.

### M6.9.9 Zero-law audit
Review final implementation for any analyzer that trusts target declarations where external confirmation was practical.

## Full detector matrix

At minimum bad/good pairs for:
build diagnostics, process tree, symbols, exact trace, sampling, coverage, architecture, exceptions, crash, memory, IO, causal edges, Actions, findings, compare, agent bounds and every injector.

## Epidemic acceptance matrix

Run on one recorded Epidemic source baseline:

1. Fast
2. Full
3. Targeted Exact
4. Memory smoke
5. Coverage smoke
6. Stress smoke

Each produces a RunId, manifest and closure receipt.

## Final traceability gate

The final script/report must reject closure if any source requirement is:
- unmapped;
- NOT_STARTED;
- IMPLEMENTED_UNQUALIFIED;
- mandatory BLOCKED;
- silently deferred.

## Final-stage adversarial focus

Final controls: satisfy all source 1–460 and P.5 fresh-application matrix, independent internal-false-PASS rejection, deployed normal-user GUI/real ETW, replay/export/integrity, clean/dirty detector variants, no cross-milestone claims mistaken for Epidemic engine freeze.

## Exit gate

- source total 460;
- mapped 460;
- qualified 460, allowing only explicitly source-conditional supported cases;
- unmapped 0;
- hidden deferral 0;
- final self-qualification PASS;
- final Epidemic acceptance matrix recorded;
- final status exactly `GOAL6_AUDITOR_COMPLETE`.

## Additional non-waivable final acceptance challenges

The P.5 full qualification runs against a **fresh integrated build of the final source tree**, not an earlier closure receipt. Confirm independently: (a) all 460/460 source requirement IDs and unchanged source wording; (b) each mandatory detector with its current negative and positive controls; (c) corrupted/missing/lost/raw-replay cases; (d) normal Debug/Release versus Audit/Deep isolation; (e) GUI/CLI backend agreement on the same RunIds/operations; (f) normal-user desktop entry, process ownership, cancellation and privileged ETW boundary; (g) stable-ID navigation, bounded exports, compatible comparisons; (h) real performance/fault/stress observation within safe limits; (i) a deceptive internal PASS claim rejected using independently observed evidence; (j) exact final diff and no weakening/removal of existing adversarial tests. Detect and report every unavailable observation instead of manufacturing confidence.

A source-level or focused-test PASS by itself cannot qualify the executable. Do not repeat full system campaigns after every small change: run the P.5 acceptance **once after final freeze**, or repeat its affected sections after a real defect fix. Store one concise stage/milestone summary plus product-required raw/manifests rather than a new verification bureaucracy.

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

# Authoritative source-requirement checklist

These source obligations remain authoritative. Checking a box requires the stage proof described above; the checklist itself is not evidence.

- [ ] `SRC-437` — Использование Auditor для MS5 qualification.
- [ ] `SRC-438` — Использование Auditor для MS6 concurrency/lifetime qualification.
- [ ] `SRC-439` — Использование Auditor для MS7 causal chain qualification.
- [ ] `SRC-440` — Использование Auditor для MS8 load/degradation qualification.
- [ ] `SRC-441` — Использование Auditor для MS9 whole-engine qualification.
- [ ] `SRC-442` — Использование того же Auditor позже для самой игры.
- [ ] `SRC-443` — Возможность открыть действие игрока и проследить путь от input через весь движок и gameplay.
- [ ] `SRC-444` — Возможность увидеть каждую вызванную функцию в Deep режиме.
- [ ] `SRC-445` — Возможность увидеть каждый переход между API.
- [ ] `SRC-446` — Возможность увидеть, сколько миллисекунд занял каждый участок.
- [ ] `SRC-447` — Возможность увидеть, какой поток выполнял конкретный участок.
- [ ] `SRC-448` — Возможность увидеть, какие allocations произошли.
- [ ] `SRC-449` — Возможность увидеть, какие исключения произошли и где были обработаны.
- [ ] `SRC-450` — Возможность увидеть, какое IO произошло.
- [ ] `SRC-451` — Возможность увидеть, где код ждал lock или scheduler.
- [ ] `SRC-452` — Возможность увидеть реальную причинную цепочку действия.
- [ ] `SRC-453` — Возможность найти архитектурные нарушения, которых нет в тестах.
- [ ] `SRC-454` — Возможность найти работающий, но подозрительно устроенный код.
- [ ] `SRC-455` — Возможность найти скрытые performance defects.
- [ ] `SRC-456` — Возможность найти мёртвый код.
- [ ] `SRC-457` — Возможность найти практически неиспользуемые API.
- [ ] `SRC-458` — Возможность доказать, какие API реально используются.
- [ ] `SRC-459` — Возможность независимо проверить заявления внутренних тестов Epidemic.
- [ ] `SRC-460` — Главный принцип Auditor: считать Epidemic недоверенной системой и по возможности подтверждать её поведение независимыми внешними наблюдениями.

Source obligations owned by this stage: **24**.
