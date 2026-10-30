# Asset pipeline contract

EditorProject remains the only Scene/Prefab/Graph authoring model. ProjectFolder
owns the portable file envelope, source identities and immutable manifest revisions.
EditorWorkspace routes commands, Save, Undo/Redo and Play to the active owner;
its UI never receives a mutable EditorProject or Assets catalog.

| Existing API | P04 responsibility / adapter |
| --- | --- |
| Assets catalog + writer/Seal | Fresh sealed source snapshot, never unsealed or mutated |
| AssetElementReference | AssetId plus asset-scoped element; specialized Sprite binding validates texture/region |
| Serialization / archive 1.0.0 | Existing authoring, versioned descriptor/index and cooked image documents |
| Platform file packages | Bounded reads and atomic individual file publication |
| Resources loader/manager | ImageResourceLoader, immutable ImagePayload, bounded processing and leases |
| Runtime Presentation | Owner-thread CreateTexture/Texture; retained texture ownership for recorded frames |
| Scene/Prefab/PlaySession | Prefab roots bind to existing runtime Scene nodes; Sprite resource/session integration remains partial |
| Streaming | No replacement importer or resource manager is introduced |

## Publication decisions

- Descriptor/commit schema 1.0.0, source index 1.3.0; 1.0/1.1/1.2 indexes remain readable.
  Unsupported versions fail explicitly. Authoring archive wire format is unchanged.
- Commit-last immutable revision files cover authoring and source metadata together.
  Source move journal is separate: uncommitted moves are rolled back under the
  writer lock at Open; successful Save makes them durable. Individual inverse
  steps retire journal entries, allowing recovery itself to be interrupted.
- AssetId is allocated independently of location/content. Rename/reimport retain
  identity; Clone retains project and asset IDs. Cross-project remapping is not
  implemented. UI creates a fresh project identity for a new template instance.
- A Sprite uses a validated specialized asset/optional element slot,
  not an untyped property string or a Runtime resource handle in authoring.
- Image resource identity derives from an immutable SHA-256 cooked key. Publishing
  another key leaves existing Resources leases and sealed catalogs valid. The
  old payload is retained until its leases are released; preview swaps only after
  successful load/upload. No live game-instance propagation is added.
- PNG recipe v1 is fixed: Windows WIC, sRGB interpretation, top-down premultiplied
  BGRA8, nearest filtering, clamp, no mipmaps/trim. Key includes source bytes and
  canonical recipe/importer/profile version. Cooked image uses Serialization;
  thumbnails reduce the same decoded pixels, not another decoder.
- Legacy .epidemic stays explicitly readable. ImportLegacy creates a separate
  folder copy including retired IDs; no implicit conversion of the original.
- Project mount paths reject traversal, device paths and existing reparse/symlink
  components. This is not a handle-bound sandbox against hostile TOCTOU attacks.
  External linked mounts, Unicode collision policy and target-profile manifests
  remain P04.1 acceptance gaps.

## Current acceptance boundary

Play binds each placed Prefab root Entity handle (full128-bit EntityId) to the
actual node created by Runtime Scene, including parent TRS. PlayContext world
transform operations read/write that registry. Entity destruction invalidates
queries and hides its runtime node. This is an identity adapter, not a second
Scene or Entity store; internal Prefab child Entities are not implicitly assigned
new spatial nodes. Authoring snapshots remain independent.

ContentHotReload has one owned asynchronous slot, including cancelled work until
it finishes. Disabling automatic watching cancels automatic work; explicit jobs
remain available through a bounded queue. Reset/Cancel request cancellation without waiting; Poll reaps ready
work even while disabled. Final service destruction joins its worker; WIC decode
itself is not preempted. No detached worker outlives its owner. Epoch/path/source
generation checks discard stale completion and schedule the current source again.

Every newly observed source is fingerprinted against persisted metadata. Stat
changes debounce over two observations; unchanged stat signatures are audited
on a five-second due interval, round-robin under the same single-worker limit.
This is eventual fingerprint detection, not a promise that all assets in a large
project are checked within five seconds. Failed sources retry after the interval
and cannot monopolize the queue. Missing source recovery keeps its AssetId.

The worker streams SHA-256 and the source-index FNV hash in one pass. A Windows
read lease denies write/delete from fingerprint/decode through owner publication
or discard. Busy files fail safely and retry; source writers may need to retry
while this short lease exists. Prepared publication is metadata-only and checks
the source identity; it does not hash or decode on the UI thread. Unchanged audits
skip decoding when the cooked blob exists. Explicit Reimport forces a rebuild.
EditorWorkspace::ReimportPng now acknowledges queueing; completion/errors arrive
through PollContent, including when automatic watching is disabled. Authoring
preview may update during Play; the running game's snapshot/resource generation
is independent and stays stable until the next Play. Stop does not change the
watch preference. Initial
Copy/Import and the explicit synchronous CookProjectPng API remain synchronous.

ImageResourceLoader no longer has a 256-key lifetime ceiling. A loader dedicated
to one ResourceManager calls CollectUnused after preview lease release: eviction
rejects in-use/pending resources, preserving their keys and old payload leases.
Released registrations are reclaimed and may be registered again with the same
identity. This does not change Runtime resource-slot/generation semantics or erase
its historical slot records. Resource pixel budgets still apply.

Production tests cover PNG alpha, copy/reopen in another process, source/cooked
identity, successful/failed/cancelled reimport, old/new active leases, missing
source Save, move Undo/Redo and recovery. A hidden native smoke exercises texture
upload and repeated thumbnail preview. It does not establish visible UX acceptance.

P04.1 and P04.2 remain partial: external mounts/security/profile fields and the
full Content command/UI matrix are not implemented. Do not mark all four requested
milestones complete on the strength of the PNG MVP.

WebAssembly/Web builds are mandatory before release, after the remaining
functionality. This overrides earlier statements that Web is optional.
