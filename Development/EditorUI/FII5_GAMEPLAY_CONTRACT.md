# F-II.5 — native gameplay contract

Implementation starting point: `763dbc3` (F-II.4 recheck). F-II.4 and F-II.5 remain PENDING MANUAL; no manual gate is inferred from builds. Gameplay implementation is complete within the profile below.

Three implementation/checkup boundaries: (1) shared native operations and Schema, (2) fixed-step movement/collision/projectile/waves/input, (3) managed adapter, reproducible Arena sample and final qualification. Implement each batch before its checkup. No Computer Use; no push.

## Ownership and compatibility decision

`Game/Composition/arena::Gameplay` composes the existing Framework EntityService, ComponentStore, PrefabComposer and Runtime Scene. It does not own a second entity collection, transform model, clock, timer thread or VM. Session-only contacts, deadlines and pending destruction are subordinate to PlaySession and canceled at Stop. Runtime Health and currency are exposed component values, not parallel copies of authoring values. The existing broader Combat/Economy services retain their APIs; this small component-based Arena profile does not pretend to implement their advanced reservations/markets/combat profiles.

The descriptor list in native `Operations()` is the single source for Graph Palette and operation identity. Canonical versioned TypeIds are `gameplay.*.v1`. Current VM ABI is i64→i64: units/types are explicit in each descriptor, bools are0/1, coordinates use centiunits, prefab arguments carry the bit pattern of a stable AssetId. This does not turn these scalars into F-II.3 typed Entity references. Operations act on a live generation-checked Entity Graph scope; arbitrary raw runtime handles are not accepted from UI or managed code. Editor/preview execution is unavailable; Graph/Managed require Play. Execution is on the native effect owner thread, maximum256 mutating operations per tick, typed `gameplay.*` failures. Argument serialization reuses VM i64 and existing IPC; no public Engine SDK ABI is replaced.

Components register in the existing Schema before Freeze; Inspector, persisted Prefab assignments and runtime store use its descriptors/validators. New properties are additive defaults, no project format fork. The legacy Native Call retains its narrower validation but delegates damage to the same native gameplay operation.

Game progress is session-only for F-II.5. Editor Save serializes authoring; Stop/Restart discards runtime currency/waves/health. Savegame files, rigid-body impulses, collision response/joints, continuous collision and pathfinding are outside this profile. No external physics dependency is introduced.

## Current verification

После замечаний19 ноября Graph получил отдельные f64 math-ноды с явными conversions: [контракт](FII_NUMERIC_GRAPH.md). Это не расширяет native/managed gameplay call ABI, который остаётся i64→i64. [Исправления приёмки](FII_FREEZE.md), [адресный повтор](FII45_MANUAL_QA.md).

First and second native checkups PASS. Final full native build and CTest162/162 PASS, including Arena Graph/Wait, movement/input, collision/projectile/death/waves, Pause/Step/time scale/Stop and1000 spawn/despawn cycles. Standalone Release smoke PASS; final packaged regression results are recorded in [FII5_REPORT](FII5_REPORT.md). Automated success does not close the manual gates.

## Execution, physics and events

Native key input → existing Play clock → Begin (action transitions/movement/waves/deadlines/projectiles) → existing fixed-update and Runtime Scheduler/Graph VM → End (overlap/damage/death/deferred destroy) → EntityService commit/remove and store synchronization → immutable snapshot. Collision events enqueue Graph work for the next scheduler pass. Graph Wait uses the Runtime scheduler; component deadlines use this same GameClock inside fixed-update, with no independent timer thread. Pause clears held keys/accumulator; Step Tick advances one tick regardless of scale. Native Pump supports scale0.1–4 with the existing250ms/8catch-up policy.

Sweep broadphase and AABB/AABB, Circle/Circle, Circle/AABB narrowphase use symmetric masks, triggers, enter/stay/exit and generation-checked owner identity. AABB stays world-axis-aligned; absolute world scale affects extents, Circle uses the largest scale axis. No rigid-body response/CCD is claimed. Projectiles ignore their owner, hit once, expire, and cancel when owner/target disappears. Impact destroys dead targets and awards native currency. Limits:1000 Entity,8192 contacts,512 events,256 mutations per tick; exceeded budgets yield typed failures. Entity records/Scene nodes/jobs are removed on despawn; journal retention4096 bounds history.

InputBindings are persisted component fields containing virtual key codes. Explicit bindings replace fallback WASD/arrows. Focus/blur/Escape release native input; Ctrl/Alt/Meta combinations remain editor/system shortcuts. Named native EventEntry nodes bind through existing GraphBehavior routes; attach/remove supplies subscription lifetime. Startup does not synthesize collision/damage/timer events.

Lethal impact schedules destruction after one subsequent scheduler pass so Died Graph handlers can run. A debugger Pause retains the dying Entity scope until execution resumes; it does not create a new clock. Explicit Destroy and Stop still cancel scoped work. Dead actors cannot move/fire/spawn, duplicate impacts cannot award another kill. The existing runtime metric `Time scale` is authoritative for every Game view.

## Managed and sample

Private worker RPC v3 adds synchronous native subcalls tagged by request/generation, bounded to256 calls and the existing3s deadline. The host chooses the live Entity scope and executes the same Gameplay::Execute used by Graph. ScriptApi.Gameplay owns no health/time/entity state. Preview returns PlayRequired. Public managed C ABI v1 and Editor IPC v4 are unchanged; incompatible workers fail handshake. Completed native side effects are not silently rolled back if managed code subsequently fails.

`Project → Create F-II Arena Sample` uses native command43(`arena`), one Workspace Execute for ordinary persisted documents, native PNG import/assignment, and a C# source. Import callbacks reject project/generation changes; duplicate sample/source creation preserves existing files. The Graph includes a real typed Scene Entity reference, validity Branch, Repeat, Spawn and Runtime Wait. Player/Turret is one Entity; WaveSpawner adds two independent component waves under the same Runtime. Sprite leases for unplaced templates are acquired lazily. Build & Register exposes a C# component that configures native range/cooldown/damage and invokes shared Health/State.

Both manual gates remain pending: [combined guide](FII45_MANUAL_QA.md). Overall F-II-GATE is not implicitly closed.

## Review: defaults, scheduling and snapshot boundary

Native F5 properties now explicitly publish their real C++ struct defaults at registration. Inspector reads metadata and never constructs a component to guess a value. Authored properties/parameters/instance overrides retain priority. Previously saved Arena prefabs with omitted fields gain editable bindings/collider defaults without recreation. Native types without declared metadata remain unavailable as before.

Determinism is equal state sequences for equal initial state, tick-stamped inputs, seed and scheduler budgets; it does not imply simultaneous completion of independent coroutines. Play gives every ready graph activation its own bounded GraphScheduleOptions quantum before advancing the next game tick; it no longer shares an editor-wide 16-job/512-instruction slice across Entities. The existing max-active and per-activation instruction/native-call limits remain. A controlled five-Entity data-heavy native Damage/Wait/Damage replay checks tick-by-tick Health equality across eight differently paced explicit/autostart runs. The regression demonstrably fails with the former aggregate budget. This is a bounded native profile, not a claim of universal replay qualification for arbitrary asynchronous managed/external inputs.

A Graph breakpoint is per activation. Pausing one activation before a shared Native Call and issuing Step Tick can shift when another activation reaches that breakpoint. A debug-paused RuntimeSnapshot is an observational, possibly partial-tick projection; it is not a committed savegame/replay snapshot. Editor Save still writes authoring only.

No multi-target/AoE transaction is currently exposed by F-II.5. When introduced, one native operation must fix the target set and stable ordering, validate/stage all writes, commit them before publishing damage/death callbacks and expose gameplay checkpoints only after a native frame/transaction boundary. Launching one coroutine per target does not meet this atomicity contract. Do not implement that barrier or a second game loop in Java.

Arena's three Graph-generated enemies spawn at the invoking Player's world transform; the separate WaveSpawner uses its own transform. This is an intentional minimal sample, not a spatial enemy-placement system.
