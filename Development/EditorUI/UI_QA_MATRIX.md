# Phase U qualification

Sources: `Temp/!Tasks/U_Phase.md`, `Temp/!Tasks/UI_Roadmap.md`; owner contract revision and manual qualification dated **2026-11-05**. **U-GATE PASS under the owner-revised contract (2026-11-05).** Manual acceptance and required build/test/architecture checks passed. Historical analyzer diagnostics and later release qualification stay explicitly nonblocking; their raw statuses are retained. L0 is authorized by the owner and begins after this gate.

## Accepted contract

The owner's numbered 01–24 checklist is distinct from the original U-A01–U-A20 numbering. Twenty checks passed directly; four initially reported failures were reviewed and accepted by the owner. This is owner-provided manual evidence, not a claim that automation performed those interactions.

| Owner checks | Result |
|---|---|
| 01–08 | PASS: launch, New, Recent/reopen, default layout, navigation, central editors, context, tool split/merge. |
| 09–10 | ACCEPTED revised requirement: move/detach a complete Tool Window group between Windows windows; individual instrument tabs need not move into arbitrary groups/documents. Central documents detach individually. |
| 11–22 | PASS: reorder/cancel drag, layout restart, second monitor, resize/scaling, viewport, Explorer/import picker, planned controls, Inspector/long text, Cyrillic/clipboard, shortcuts, dirty-close choices, detached close. |
| 23 | ACCEPTED requirement retirement: old Win32 user interface and compatibility-launch path are no longer required. Engine functionality/data/tests must survive L0. |
| 24 | ACCEPTED nonblocking condition: remaining ETW Broker. Ownership verified below. |

Canonical U-A08 docking is accepted under whole-group movement; U-A09 layout is accepted from owner check 12. U-A20 no longer requires the retired legacy UI. No agreed limitation remains a blocking FAIL. These changes supersede the earlier individual-tab/legacy-UI requirements without claiming that unsupported individual-tab operations work.

## Automatic checks and focused input regressions

- Full engine last passed **165/165**, zero skipped. Missing four process drivers restored under `tools/ci`; durable-store negatives assert the intended exit codes. Fresh final full profile passed in 441.65 s; LastTest.log independently confirms 165 passed and zero skipped.
- Native Editor regressions last passed **30/30**. Common typed IPC/native lifecycle/history/Unicode/stable-ID/locking/dirty-close contracts pass; adversarial IPC twelve cases, exact catalogue/field coverage and C++-only/managed-off build pass.
- Own open Platform 253.31033.145 product: 353 hash-locked Maven runtime artifacts, no installed IDEA product files. Release EXE smoke covers 46 panels/77 forms, independent Tool Window HWND, native crash/reconnect preserving project/view identity, zero fatal diagnostics and shutdown surfaces=0.
- Startup correction: pinned Apache DirectoryLock excludes only the current JVM and its immediate native jpackage parent, preserving genuine owner checks (including more distant ancestors). Windows uses upstream loopback TCP transport and disposable `system-tcp-v1`; config/container/preferences persist and legacy AF_UNIX files are left untouched. Actual EXE second-launch activation and forced-crash/stale-profile restart pass.
- Final public JBR build and restart smoke **PASS, 129.72 s** (`u-jbr-product`). Runtime is public vanilla **JBR 21.0.10+1-b1163.108**, downloaded separately and checked against the pinned publisher SHA512; compiler remains Microsoft OpenJDK 21.0.7+6-LTS. No installed IDEA runtime is copied. Runtime legal files ship with the image.
- Focused Computer Use before owner acceptance: maximized 1920×1032 workspace on 1920×1080 desktop; actual New/Recent/Rename/history/Save/reopen, dirty-close Cancel/Save, clipboard and independent Graph-window close passed. Ctrl+Z/Y with native viewport focus changed native revision 2→3→4, preserving Document ID; Ctrl+S committed the Cyrillic name, verified after reopen. Actual split retains hidden redundant group headings. These are real input observations; programmatic prints remain separate.
- Product productivity catalogue fixes Ctrl+A fatal diagnostics; Platform shortcuts share action instances and respect editable-text focus. File selection now uses Platform FileChooser API and registered modality/focus. Standard OpenJDK fallback path selection/Enter/Escape passed; JBR enables the upstream native chooser path. Owner check 16 accepts file selection, not future F1 import mutation.
- Latest completed interactive session had zero SEVERE entries and clean shutdown, surfaces=0. No complete Computer Use rerun is required after owner qualification.

Final U batch completed in 948.63 s: common 102.08 s; Release product 143.19 s; restart 51.08 s; IPC/spec PASS; native regressions 30/30 (92.95 s); C++-only PASS; full profile 165/165 (441.65 s); source hygiene PASS. Architecture self-tests and public-header compile separately PASS, 37.73 s. Raw batch exit remains 1 for the unchanged nine baseline analyzer findings and NOT RUN clean-machine release qualification; neither is silently converted to a test PASS.

Repeatable batch: `tools/editor-ui-host/Run-U8-Checks.ps1 -JdkRoot <Microsoft-JDK21>`. It retains raw failures; its nonzero analyzer/release-qualification result is not relabelled an automatic PASS. U-GATE classification distinguishes the required build/test results from the expressly accepted baseline diagnostics and later release checks.

## Broker ownership

Observed `EpidemicAuditorEtwBroker` is an **Auto** Windows service, running from `C:/Program Files/EpidemicAuditorEtwBroker/EpidemicAuditorEtwBroker.exe --service <owner SID>`. Its PID is owned by the service host, not the Editor process tree. It predates this test instance. Remaining alive after Editor closes is the independent service lifecycle, not evidence of an Editor child leak. No service/ACL/security setting was changed and no unrelated process was terminated.

## Known nonblocking limits / backlog

- Whole Tool Window groups move; arbitrary individual tool-tab movement is outside the accepted U contract.
- Earlier input testing observed default selected tabs after a restart; the owner's check 12 accepts the intended saved-layout workflow. Full arbitrary automatic content reconstruction is not inferred from that acceptance.
- Nine historical unsuppressed diagnostics in unchanged headers: four signed-shift counts (`scene_state.h:23–26`), two move exception specifications (`scene_composition.h:38/40`), two integer/float conversions (`renderer.h:96`), one passedByValue (`runtime_bridge.h:504`). No own C++ adapter findings remained. Preserve raw analyzer status; new regressions remain blocking.
- Deprecated Platform descriptor-loading warnings, IDE-container watcher notification, clean-machine/runtime deployment, complete transitive-license review and long-run resource budgets remain separate release/backlog work. No numerical budget was invented.
- F0–F13 backend feature parity is not a consequence of U acceptance. Planned features stay disabled until their commands and regressions pass. No new panels are authorized by this qualification.

## L0 / F

**L0 PASS.** Removed Win32 window/control/browser/child-viewport wrappers and their executable/compatibility targets; removed competing Swing/U1 owners, preserving shared validation and relocating the live JNI adapter unchanged into the host. Full post-L0 build/CTest + architecture/public headers + IPC/spec + C++-only + actual EXE restart PASS (`l0-full-qualification`, 463.22 s). Full CTest: **161/161, zero failures/skips**; five retired UI-specific cases removed and one PNG/GPU lease test added, all semantic test sources retained. Common contracts + Release EXE smoke PASS (`l0-host-regression`, 170.83 s).

The new GPU test's async callable-transfer warning was fixed; affected fixture/tests passed again (3/3), with no remaining own-source findings. Raw analyzers still report the same nine unchanged-header diagnostics. Generic Verify-Changed also misclassifies deleted headers as needing a compiled consumer; its BLOCKED result is retained. With zero surviving changed C++ headers, the identical clang-tidy/Cppcheck policies were run directly for all three surviving changed translation units; no pipeline check/configuration was altered.

Cleanup removed 18 verified obsolete build/report directories (20.10 GiB logical artifact size) and 27 stale files/directories of retired targets inside current build trees. Current builds/dependencies, canonical fixtures and game directories are preserved; cleanup rejected project/save files and reparse points.

**F0 implementation complete; F0-GATE PARTIAL pending owner visual acceptance**
(2026-11-06, continuation from `719970a`). No Computer Use was used.

Selection now carries native kind/document/element/component/part and selection
revision; hierarchy, context readers and document focus use the same owner.
The scalar Inspector edits registered bool/i64/double/string through native
InspectorCommand with revision checks and one transaction per committed edit.
Draft/validation/Escape behavior, exact integers/UTF-8, Undo/Redo and save/reopen
are exercised. Unsupported fields/references remain read-only.

Per-document dirty state comes from native snapshots. Save flushes drafts and
awaits actual native success. Dirty tab and floating-window close offer Save All /
Discard Document / Cancel; in-flight edit/Save completion is ordered before close.
Discard restores only that document from the native durable savepoint; other
documents and asset/history state remain. Closing a project closes all peer
document copies, including native Scene surfaces in independent Windows windows.

PASS: common native/UI contracts; private IPC v2 adversarial cases (14); catalog;
managed-off C++ build; full engine CTest **161/161, zero failed/skipped**
(`Temp/U/f0-engine-tests.xml`); architecture self-test; public-header checks.
Earlier raw 160/161 is retained as `f0-engine-tests-before-path-fix.xml`: only the
new discard fixture's relative output path failed. Fixed to the existing absolute
test directory; focused Core and then full CTest passed.

PASS: latest-source Platform smoke, actual floating HWND close events with
programmatically supplied dialog choices, busy tab Cancel, immediate Save→close,
reconnect and project close with peer Scene views; surfaces=0, fatal diagnostics=0.
Final Release packaging + actual EXE live-owner activation/crash/restart PASS
(`f0-final-product`, 140.69 s). No physical mouse/visual acceptance is claimed.

Deep analysis completed: 2 changed headers, 44 translation units, 88 checks,
935.28 s. Raw result remains FAIL, without modified rules/suppressions:
- Previously classified nine U/L0 findings: scene_state signed bitwise (4),
  SceneCompositionInstance move exception specification (2), Renderer integer/
  float conversion (2), RuntimeBridge queue_policy passedByValue (1).
- Wider changed-header consumer sweep additionally reaches unchanged
  easily-swappable filesystem-role parameter pairs in image_resource.cpp
  PrepareImportedPng and project_folder.cpp RestoreCommit / CopyTree /
  ImportLegacy / ResolveMount. Existing signatures and comments match HEAD;
  no new suppression is added.
- Unchanged project_folder_tests.cpp:125 signed bitwise and :140 exception
  escaping main are also reached by the expanded consumer sweep; retained as
  historical test-code backlog, not new F0 findings.
- MSVC 14.51 xfilesystem_abi.h EnumCastOutOfRange on filesystem flag value 33,
  reached through unchanged content_hot_reload.cpp: external-library/toolchain
  diagnostic, not silently excluded. Retain for toolchain/backlog investigation.
- Raw output: `Temp/U/logs/f0-deep.*` and per-translation-unit tidy/cppcheck logs.
  These failures are not reported as analyzer PASS.

NOT RUN: owner visual acceptance of this F0 batch. The complete procedure is in
`FI_FEATURE_MATRIX.md` (Owner verification: complete F0). F1 has not started.
Picking/gizmos, asset mutation/import, Graph authoring and continuous Game View
remain later phases; live Game View is F-II/F8 under the current task.
