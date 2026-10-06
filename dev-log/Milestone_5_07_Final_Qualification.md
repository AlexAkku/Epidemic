# Part 7/7 — 5.7 Final qualification

Дата pipeline-ревизии: 2026-10-05

Этот файл является самодостаточным execution-slice для текущего этапа. Он сохраняет полные numbered requirements данного этапа и повторяет только те общие ограничения, которые нужны для безопасного выполнения без чтения остальных шести частей. Предыдущие этапы считаются входным contract только если их PASS зафиксирован в machine-readable/task state; при красном предыдущем gate этот этап не начинается.

---

# P. Pipeline execution contract

## P.1. Requirement fidelity

Authoritative semantic detail находится в numbered requirements ниже. Packet map не заменяет их и не имеет права объявлять PASS, если хотя бы один входящий `M5.x.y` не выполнен.

Legacy command blocks внутри сохранённого плана трактуются как **proof endpoints / expected gates**, а не как требование модели вручную оркестрировать конкретные инструменты или читать большие логи. Механический запуск проходит через project execution contract и quiet local wrappers; semantic meaning команды/validator/test при этом не меняется.

## P.2. Packet completion schema

Каждый operational packet закрывается только при наличии:

- `objective_status`: PASS/BLOCKED;
- `requirement_ids`: полный диапазон входящих `M5.x.y`;
- `finite_surface`: IDs записей/сценариев/дефектов, если применимо;
- `changed_scope`: ownership-local files/contracts;
- `proof`: exact validator/test/evidence identities;
- `remaining_gaps`: пусто либо explicit blockers;
- `handoff`: краткие stable facts для следующего packet.

Нельзя закрывать packet narrative summary без machine proof.

## P.3. Operational packets for this part

### P7-A — Final ledger contract
Requirements: `M5.7.1–M5.7.3`.

Цель: generated per-module Goal 5 ledger, aggregator и adversarial fake-PASS protection.

### P7-B — Freeze/CTest integration
Requirements: `M5.7.4–M5.7.8`.

Цель: existing freeze-contract integration, workflow enforcement, labels и manifest; второго validation pipeline не создаётся.

### P7-C — Exact-tree qualification campaign
Requirements: `M5.7.9–M5.7.14`.

Цель: clean validators/matrix, focused Debug/Release, repetition и portability detector на одном exact tree.

### P7-D — No-debt/final gate
Requirements: `M5.7.15–M5.7.20`.

Цель: generator idempotence, note independence, zero remaining Goal 5 gaps, reconciled defect registry, regenerated final ledger и единственный final PASS/BLOCKED verdict.

## P.4. Work sizing inside a packet

Если packet surface всё ещё велик (например сотни candidates), он делится только по stable finite IDs или ownership boundaries. Sub-slice не получает отдельный semantic definition; он наследует requirement IDs packet и возвращает reviewed records в один canonical contract.

Разрешённые slice boundaries:
- module/owner;
- candidate kind;
- participant set;
- scenario ID;
- defect ID;
- deterministic risk category.

Запрещённые boundaries:
- случайные file batches;
- «первые N найденных» как acceptance;
- tool-specific batches;
- разные LLM workers с независимыми semantic decisions.

## P.5. Proof discipline

Mechanical evidence подтверждает requirement, но не определяет его semantics. PASS output хранится вне reasoning context; FAIL должен сводиться к первой значимой причине и exact evidence location. Полные логи нужны только для расследования конкретного failure.

## P.6. Resume discipline

После завершения packet/sub-slice structured task state хранит requirement IDs, reviewed finite IDs, change-set, proof identities и blockers. При неизменных inputs закрытый packet не пересматривается после compaction.

---

# 1. Общие правила выполнения

## 1.1. Работа строго последовательная

Milestone 5 выполняется только в порядке:

1. Admission.
2. 5.1 Persistence ownership map.
3. 5.2 Durable representation.
4. 5.3 Unified save point.
5. 5.4 Whole-engine restore.
6. 5.5 Deterministic runner.
7. 5.6 Determinism qualification.
8. 5.7 Final qualification.

Нельзя начинать следующий этап, если предыдущий имеет `BLOCKED`, `UNCLASSIFIED`, stale generated artifacts или validator failures.

## 1.2. Один рабочий baseline

После каждого этапа:

1. Все generated canonical artifacts перегенерируются штатным generator.
2. Повторный generator run должен дать zero diff.
3. Validators должны быть green.
4. Relevant tests должны быть green.
5. Изменения фиксируются одним serial checkpoint commit или delta.
6. Следующий этап начинается только от этого состояния.

## 1.3. Defect workflow

Любая найденная production problem проходит один порядок:

1. Создать stable defect ID.
2. Зафиксировать reproducer.
3. Добавить failing regression.
4. Подтвердить failure на текущем code.
5. Выполнить минимальный production fix.
6. Получить regression PASS.
7. Прогнать affected module/integration tests.
8. Прогнать affected Goal 5 validator.
9. Обновить machine-readable contract.
10. Не менять unrelated code.

Рекомендуемые ID prefixes:

`G5-MAP-*`
`G5-SAVE-*`
`G5-SCHEMA-*`
`G5-STORAGE-*`
`G5-BARRIER-*`
`G5-RESTORE-*`
`G5-RECON-*`
`G5-DET-*`

## 1.4. Новые machine-readable artifacts

Все Goal 5 canonical files размещаются в `docs/freeze`.

Планируемый набор:

```text
docs/freeze/goal5_persistence_candidates.json
docs/freeze/goal5_persistence_inventory.py
docs/freeze/goal5_persistence_contract.json
docs/freeze/goal5_persistence_contract.py
docs/freeze/goal5_persistence_gaps.json

docs/freeze/goal5_participant_contract.json
docs/freeze/goal5_participant_contract.py
docs/freeze/goal5_schema_contract.json
docs/freeze/goal5_schema_contract.py

docs/freeze/goal5_save_generation_contract.json
docs/freeze/goal5_save_generation_contract.py

docs/freeze/goal5_restore_contract.json
docs/freeze/goal5_restore_contract.py

docs/freeze/goal5_determinism_candidates.json
docs/freeze/goal5_determinism_inventory.py
docs/freeze/goal5_determinism_contract.json
docs/freeze/goal5_determinism_contract.py

docs/freeze/goal5_evidence_quality.py
docs/freeze/goal5_ledger.json
docs/freeze/goal5_contract.py
```

Необязательно создавать каждый файл заранее.

Файл появляется только на своём этапе.

Каждый Python validator должен поддерживать минимум:

```text
--check
--self-test
```

Generator, если нужен, должен отдельно поддерживать deterministic regeneration.

## 1.5. Stable evidence identity

Goal 5 evidence не должен опираться только на line number.

Для executable evidence хранятся минимум:

```text
test_target
test_symbol или test_case
contract_assertion
source_path
optional_line
```

Line используется как navigation hint.

Если line изменился, evidence остаётся валидным по stable test identity.

## 1.6. CI policy

Новый validator включается в CI только после того, как:

1. Его own positive fixture PASS.
2. Все negative fixtures действительно FAIL.
3. Current production tree PASS.
4. Generator idempotent.
5. Validator не читает roadmap/Milestone Markdown.
6. Validator не auto-repairs canonical state в `--check`.

До этого validator запускается локально как development gate.

---

---

# 9. Stage 5.7: Final Milestone 5 qualification

# 5.7.A. Build final ledger

## M5.7.1. Создать `goal5_ledger.json`

Generated only.

For every discovered production module:

```text
module
persistence_applicability
persistence_status
determinism_applicability
determinism_status
candidate_ids
participant_ids
scenario_evidence
defect_ids
```

Allowed result:

```text
PASS
N/A
BLOCKED
```

`N/A` requires reviewed rationale in source contract.

## M5.7.2. Создать `goal5_contract.py`

Aggregator checks all prior Goal 5 contracts.

It must not duplicate deep logic.

It verifies:

1. Every lower validator PASS.
2. No BLOCKED.
3. No unclassified candidate.
4. No gap report entries left except explicitly allowed future-goal items.
5. Whole-engine tests registered.
6. Required CTest labels exist.
7. Required evidence targets exist.
8. Ledger generated from contracts.
9. Ledger idempotent.
10. No roadmap Markdown dependency.

## M5.7.3. Add Goal 5 adversarial self-test

One self-test entry point should include broken fixtures for:

Missing persistent owner.
Orphan participant.
Migration gap.
Mismatched save generation.
Restore dependency cycle.
Transient runtime identity persisted.
Missing digest contributor.
Unreviewed `steady_clock`.
Unordered observable output.
Generic evidence.
Missing test target.
Fake PASS in ledger.

---

# 5.7.B. Integrate with current freeze-contract

## M5.7.4. Update `ci_gate_contract.py`

Semantic CI validator must require Goal 5 checks once Goal 5 is ready for enforcement.

Expected workflow commands:

```powershell
python docs/freeze/goal5_admission.py --check
python docs/freeze/goal5_persistence_inventory.py --check
python docs/freeze/goal5_persistence_contract.py --check
python docs/freeze/goal5_participant_contract.py --check
python docs/freeze/goal5_schema_contract.py --check
python docs/freeze/goal5_save_generation_contract.py --check
python docs/freeze/goal5_restore_contract.py --check
python docs/freeze/goal5_determinism_inventory.py --check
python docs/freeze/goal5_determinism_contract.py --check
python docs/freeze/goal5_evidence_quality.py --check
python docs/freeze/goal5_contract.py --check
```

Self-tests also required.

## M5.7.5. Update GitHub workflow

Add Goal 5 validators to existing `freeze-contract` job.

Do not create unrelated workflow unless timeout/ownership justifies later split.

System tests remain in Full profiles through CTest.

## M5.7.6. Update `build.ps1` validator list

Goal 5 validators and self-tests become part of:

```powershell
.\build.ps1 -Mode Validators
```

No hidden manual command required for local admission.

---

# 5.7.C. CTest taxonomy

## M5.7.7. Verify labels

Required labels after Goal 5:

```text
persistence-local
persistence-wire
persistence-storage
persistence-whole-engine
determinism
determinism-save-load
goal5-regression
```

Not every old test needs a new label.

New Goal 5 suites must have explicit labels.

## M5.7.8. Update `ctest_manifest`

Record new tests only after names/labels/timeouts stable.

Use normal manifest generation procedure.

Then:

```powershell
python docs/freeze/ctest_manifest.py --check
python docs/freeze/ctest_manifest.py --self-test
```

No hand editing canonical CTest JSON.

---

# 5.7.D. Final exact-tree qualification

## M5.7.9. Clean validators run

```powershell
.\build.ps1 -Mode Validators -Clean
```

Expected:

All old freeze validators green.
All Goal 5 validators green.
All self-tests green.

## M5.7.10. Clean full Windows matrix

```powershell
.\build.ps1 -Mode Matrix -Configuration Both -Clean
```

Expected:

Base Debug PASS.
Base Release PASS.
Runtime Debug PASS.
Runtime Release PASS.
Full Debug PASS.
Full Release PASS.
Public header self-containment PASS.
Exact CTest manifests PASS.
Warnings as errors PASS.

## M5.7.11. Goal 5 focused Debug rerun

```powershell
ctest --test-dir build/ae2/full-debug -C Debug -L persistence-wire --output-on-failure
ctest --test-dir build/ae2/full-debug -C Debug -L persistence-storage --output-on-failure
ctest --test-dir build/ae2/full-debug -C Debug -L persistence-whole-engine --output-on-failure
ctest --test-dir build/ae2/full-debug -C Debug -L determinism --output-on-failure
ctest --test-dir build/ae2/full-debug -C Debug -L determinism-save-load --output-on-failure
```

## M5.7.12. Goal 5 focused Release rerun

Equivalent commands against `full-release`.

## M5.7.13. Repetition

Determinism D1:

At least 10 runs required scenarios.

Whole restore:

At least 3 consecutive runs.

Storage process-boundary smoke:

At least 3 consecutive fresh-process runs.

This is Milestone 5 qualification, not full Goal 9 flakiness campaign.

## M5.7.14. ClangCL portability check

Existing required public-header ClangCL remains.

Additionally, if deterministic headless target builds with ClangCL without platform expansion, run it as detector.

A mismatch must be investigated for UB or hidden nondeterminism.

It is not D4 blocker unless D4 explicitly adopted.

## M5.7.15. Verify generators idempotent

Run every Goal 5 generator once.

Capture hashes только generated Goal 5 canonical artifacts, например:

```powershell
$before = Get-ChildItem docs/freeze/goal5_*.json -File |
    Sort-Object FullName |
    ForEach-Object { "$($_.FullName)|$((Get-FileHash $_.FullName -Algorithm SHA256).Hash)" }
```

Run every Goal 5 generator второй раз без изменения source/contracts.

Capture `$after` тем же способом и сравнить:

```powershell
Compare-Object $before $after
```

Expected output: empty.

Дополнительно убедиться, что второй generation run сам сообщает zero rebases/repairs, если generator ведёт такие counters.

Нельзя использовать общий `git diff --exit-code` как единственную проверку idempotence, потому что в рабочем tree могут находиться legitimate production changes текущего milestone.

No hidden rebasing/repair.

## M5.7.16. Verify no note dependency

Search executable CI scopes again.

No Milestone/roadmap Markdown as source of truth.

## M5.7.17. Verify no remaining Goal 5 gaps

Check generated gap report:

```text
MISSING_PARTICIPANT = 0
MISSING_SCHEMA_POLICY = 0
MISSING_DURABLE_STORAGE = 0
MISSING_DEPENDENCY_DECISION = 0
MISSING_RECONCILIATION = 0
MISSING_TEST_EVIDENCE = 0
MISSING_DETERMINISM_POLICY = 0
SUSPECT_TRANSIENT_IDENTITY = 0
CONFIRMED_DEFECT unresolved = 0
```

## M5.7.18. Verify defect registry

Every Goal 5 confirmed defect:

Stable ID.
REVIEWED.
Reproducer.
Permanent regression.
Valid CTest target.
Valid test symbol.
No unresolved P0/P1.

## M5.7.19. Generate final Goal 5 ledger

Run generator.

Expected:

```text
100% discovered production modules have persistence decision
100% discovered production modules have determinism decision
BLOCKED = 0
UNCLASSIFIED = 0
```

## M5.7.20. Final gate

```powershell
python docs/freeze/goal5_contract.py --check
python docs/freeze/goal5_contract.py --self-test
```

Only if PASS:

Milestone 5 can be marked complete.

---

---

# Stage-specific scope guard

## 10.7. 5.7

Expected production changes:

Zero.

Only wiring validators/CI/manifests and fixing qualification regressions.

---

---

# 11. Stop conditions requiring design review

Execution must stop before continuing if any condition occurs.

## 11.1. New production module appears necessary

Reason:

Changes production inventory and local-ready baseline.

Required action:

Separate design decision and local audit.

## 11.2. SaveGame core needs direct gameplay major dependency

This violates current architecture intent.

Do not add dependency.

Move adapter to IntegrationLayer.

## 11.3. Runtime needs Framework dependency

Forbidden.

Move coordinator upward.

## 11.3a. Integration needs a new Runtime major dependency

Do not add it silently.

First try a narrow injected port.

If direct dependency remains necessary, treat the allowlist expansion as a reviewed architecture delta and extend architecture adversarial fixtures so the permission remains exact rather than broad.

## 11.4. Whole save cannot obtain consistent cut without broad global lock

Do not immediately introduce engine-wide mutex.

First compare epoch/snapshot-isolation alternatives.

## 11.5. Restore requires fallible operation after semantic commit with no retry state

This is a correctness defect.

Design explicit reconciliation ownership before proceeding.

## 11.6. Historical save version cannot be migrated unambiguously

Do not fabricate migration.

Decide compatibility cutoff and document unsupported version.

## 11.7. Determinism requires renderer/backend bit identity

Re-evaluate deterministic domain.

Do not accidentally make graphics backend state part of semantic contract.

## 11.8. Wall-clock budget affects semantic commit order

Treat as confirmed determinism risk.

Add explicit deterministic budget/commit policy.

## 11.9. Scanner produces unmanageable false positives

Do not whitelist broad directories blindly.

Improve symbol/context classification while preserving fail-closed coverage.

## 11.10. A validator passes by auto-repairing data

Forbidden.

`--check` must never mutate canonical state.

Generator and validator remain separate operations.

---

---

# 12. Concrete new test targets planned

Names are planned defaults.

If a target with equivalent responsibility already exists by execution time, reuse it rather than duplicate.

## 12.1. Existing targets to extend

```text
EpidemicGameFrameworkSaveGameTests
EpidemicGameFrameworkCoreIntegrationTests
EpidemicRuntimePersistenceTests
EpidemicGameFrameworkRuntimeBridgeTests
```

## 12.2. New Truth targets

Recommended:

```text
EpidemicPersistenceWireTruthTests
EpidemicWholeEngineRestoreTruthTests
EpidemicDeterminismTruthTests
EpidemicDeterminismSaveLoadTruthTests
```

Responsibilities:

`EpidemicPersistenceWireTruthTests`:
wire + storage + process reopen.

`EpidemicWholeEngineRestoreTruthTests`:
fresh composition restore and reconciliation.

`EpidemicDeterminismTruthTests`:
runner D1/D2 control scenarios.

`EpidemicDeterminismSaveLoadTruthTests`:
control versus save/load equivalence.

Do not create one target per tiny requirement.

---

---

# 13. Exact CMake integration policy

## 13.1. Existing module tests

Add labels with `set_tests_properties` after `epidemic_add_test`.

Example shape:

```cmake
set_tests_properties(
    EpidemicGameFrameworkSaveGameTests
    PROPERTIES LABELS "persistence-local"
)
```

Preserve existing labels if any.

## 13.2. Truth helper

Current `epidemic_add_truth_test` applies `truth`.

For additional labels:

```cmake
set_tests_properties(
    EpidemicWholeEngineRestoreTruthTests
    PROPERTIES LABELS "truth;persistence-whole-engine"
)
```

Be careful:

A later `set_tests_properties(... LABELS ...)` replaces label set.

Always include `truth` explicitly.

## 13.3. Timeouts

Assign explicit timeout to new whole-engine/process tests.

Do not rely on global default where scenario legitimately needs more time.

Keep under freeze timeout budget.

---

---

# 14. Evidence and validator quality rules

## 14.1. Forbidden evidence

Cannot be sole evidence:

`#include`.
`using`.
Type declaration.
Service object construction.
`return 0`.
`main()`.
Generic smoke pass.
File existence.
Comment.
Rationale without executable behavior where behavior is required.

## 14.2. Required evidence for behavior

Example:

```json
{
  "test_target": "EpidemicWholeEngineRestoreTruthTests",
  "test_symbol": "RestoreRebuildsRuntimeBridgeWithoutReusingOldHandles",
  "assertion": "old runtime observation is rejected and new mapping is consistent"
}
```

## 14.3. Validator self-test rule

Every newly introduced acceptance rule requires at least one negative fixture proving it can fail.

A validator with only happy-path tests is not accepted.

---

---

# 15. Final Milestone 5 no-debt checklist

Milestone 5 cannot close until every item is true.

Final closure 2026-10-06: M5.7.1–M5.7.20 PASS. Exact-tree machine receipt:
`_goal5_handoff/final_qualification.json`; first-to-last audit:
`_goal5_handoff/final_requirements_audit.json`. Execution summary:
`docs/freeze/goal5_stage57_execution.md`. Known MS5 code/evidence debt=0.

## Persistence inventory

[x] Production discovery complete.
[x] Every production module persistence applicability reviewed.
[x] Every snapshot/checkpoint candidate classified.
[x] No unknown persistence candidate.
[x] No duplicate authoritative owner.
[x] Runtime transient identity not durable.

## Participant coverage

[x] Every Framework persistent owner/checkpoint covered.
[x] Runtime persistence plane mapped.
[x] No overlapping participant ownership.
[x] Participant dependencies acyclic.
[x] Required/optional semantics reviewed.

## Schema and wire

[x] Current schema known for every participant.
[x] Historical support policy explicit.
[x] No artificial migration fixture.
[x] Supported historical versions have fixtures.
[x] Wire format bounded/versioned.
[x] Corruption/truncation rejected.

## Storage

[x] Production durable storage path exists.
[x] Failed write preserves last valid save.
[x] Fresh-process reopen works.
[x] Storage errors controlled.

## Save generation

[x] One logical save point.
[x] Framework SaveGame included.
[x] Runtime Persistence included.
[x] Time included.
[x] Pending accepted work has save policy.
[x] Failed save never publishes valid-looking incomplete package.

## Restore

[x] Fresh composition restore.
[x] Production dependency order.
[x] Derived indexes valid.
[x] RuntimeBridge rebuilt.
[x] Old transient handles rejected.
[x] Precommit failures atomic.
[x] Postcommit reconciliation retry-safe.
[x] Simulation resumes only when ready.

## Deterministic runner

[x] Deterministic state coverage complete.
[x] Canonical digest.
[x] Semantic event trace.
[x] Reproduction metadata.
[x] Fresh-process support.

## Determinism audit

[x] All candidates reviewed.
[x] Unordered semantic output canonicalized.
[x] Randomness explicit.
[x] Wall-clock budgets reviewed.
[x] Same-time ordering explicit.
[x] Floating semantic policy explicit.

## Required scenarios

[x] Control continuation.
[x] Save/load equivalence.
[x] Streaming.
[x] Deferred/budget.
[x] Combat/effects.
[x] Perception/knowledge/AI.
[x] Economy/process/resources.
[x] Runtime materialization.

## Qualification

[x] D1 PASS.
[x] D2 PASS.
[x] Goal 5 validators PASS.
[x] Goal 5 self-tests PASS.
[x] Full Windows Debug/Release matrix PASS.
[x] Public-header self-containment PASS.
[x] Exact CTest manifests PASS.
[x] Goal 5 generators idempotent.
[x] Gap report empty.
[x] Unresolved Goal 5 P0/P1 = 0.
[x] Unresolved persistence/determinism code debt = 0.

---


# 16. Milestone 5 completion state

Only after M5.7.20 passes record:

Recorded 2026-10-06: `PERSISTENCE_QUALIFIED`, `DETERMINISM_QUALIFIED`, `GOAL5_COMPLETE`.

```text
PERSISTENCE_QUALIFIED
DETERMINISM_QUALIFIED
GOAL5_COMPLETE
```

Do not assign:

```text
SYSTEM_READY
FREEZE_READY
FROZEN
```

Those statuses require later goals.

After `GOAL5_COMPLETE`, next allowed campaign step is Goal 6:

memory/lifetime and concurrency/async qualification.

---


# 17. Final execution order, condensed

This is the literal serial order.

```text
M5.0   Admission
  |
  v
M5.1   Discover persistence surface
  |
  v
M5.1   Review/classify all candidates
  |
  v
M5.1   Validate persistence map
  |
  v
M5.2   Build participant coverage graph
  |
  v
M5.2   Implement missing participants
  |
  v
M5.2   Freeze schema/migration policy
  |
  v
M5.2   Implement whole-image codec
  |
  v
M5.2   Implement durable storage boundary
  |
  v
M5.2   Qualify wire/storage/reopen
  |
  v
M5.3   Classify save-sensitive pending work
  |
  v
M5.3   Define consistent-cut mechanism
  |
  v
M5.3   Implement cross-plane save coordination
  |
  v
M5.3   Fault-test atomic save generation
  |
  v
M5.4   Freeze restore phase/dependency contract
  |
  v
M5.4   Build whole-engine restore harness
  |
  v
M5.4   Qualify RuntimeBridge rematerialization
  |
  v
M5.4   Qualify reconciliation failures/retry
  |
  v
M5.5   Define semantic deterministic domain
  |
  v
M5.5   Build canonical digest and event trace
  |
  v
M5.5   Build deterministic scenario runner
  |
  v
M5.5   Prove runner with adversarial self-tests
  |
  v
M5.6   Discover nondeterminism candidates
  |
  v
M5.6   Review every candidate
  |
  v
M5.6   Fix confirmed determinism defects
  |
  v
M5.6   Run scenarios A-H
  |
  v
M5.6   D1/D2 repetition
  |
  v
M5.7   Generate Goal 5 ledger
  |
  v
M5.7   Integrate validators into freeze-contract
  |
  v
M5.7   Update CTest manifest
  |
  v
M5.7   Clean Windows matrix
  |
  v
M5.7   Focused persistence/determinism reruns
  |
  v
M5.7   Idempotence/no-gap/no-debt checks
  |
  v
GOAL5_COMPLETE
```

No phase is parallel.

No later phase starts on a red earlier gate.

No final PASS is based on a roadmap Markdown file.

No production fix is accepted without executable regression.

