# Asset pipeline contract

EditorProject remains the only Scene/Prefab/Graph authoring model. ProjectFolder
owns the portable file envelope, source identities and immutable manifest revisions.
EditorWorkspace routes commands, Save, Undo/Redo and Play to the active owner;
its UI never receives a mutable EditorProject or Assets catalog.

| Existing API | P04 responsibility / adapter |
| --- | --- |
| Assets catalog + writer/Seal | Fresh sealed source snapshot, never unsealed or mutated |
| AssetElementReference | AssetId plus asset-scoped element; reserved for typed Sprite slot |
| Serialization / archive 1.0.0 | Existing authoring, versioned descriptor/index and cooked image documents |
| Platform file packages | Bounded reads and atomic individual file publication |
| Resources loader/manager | ImageResourceLoader, immutable ImagePayload, bounded processing and leases |
| Runtime Presentation | Owner-thread CreateTexture/Texture; retained texture ownership for recorded frames |
| Scene/Prefab/PlaySession | Unchanged authoring/gameplay owners; Sprite assignment is P04.3b |
| Streaming | No replacement importer or resource manager is introduced |

## Publication decisions

- Descriptor/commit schema 1.0.0, source index 1.1.0; 1.0.0 indexes remain readable.
  Unsupported versions fail explicitly. Authoring archive wire format is unchanged.
- Commit-last immutable revision files cover authoring and source metadata together.
  Source move journal is separate: uncommitted moves are rolled back under the
  writer lock at Open; successful Save makes them durable. Individual inverse
  steps retire journal entries, allowing recovery itself to be interrupted.
- AssetId is allocated independently of location/content. Rename/reimport retain
  identity; Clone retains project and asset IDs. Cross-project remapping is not
  implemented. UI creates a fresh project identity for a new template instance.
- A Sprite will use a validated specialized asset/optional element slot in P04.3b,
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

Production tests cover PNG alpha, copy/reopen in another process, source/cooked
identity, successful/failed/cancelled reimport, old/new active leases, missing
source Save, move Undo/Redo and recovery. A hidden native smoke exercises texture
upload and repeated thumbnail preview. It does not establish visible UX acceptance.

P04.1 and P04.2 remain partial: external mounts/security/profile fields and the
full Content command/UI matrix are not implemented. Do not mark all four requested
milestones complete on the strength of the PNG MVP.

WebAssembly/Web builds are mandatory before release, after the remaining
functionality. This overrides earlier statements that Web is optional.
