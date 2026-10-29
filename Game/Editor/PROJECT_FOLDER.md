# P04.0 / P04.1 production folder backend (proposed delta)

**Patch base:** `ea654e1581e6701fb3d38c6e2220be19b7c604cb` on `v2`.
**Status:** Windows Debug build, public headers and 158 CTest cases passed; integration/GUI and the listed acceptance gaps remain open.

## Ownership / contract

- The existing `EditorProject` remains the single Scene/Prefab/Graph authoring authority.
- `ProjectFolder` uses its existing `ProjectDefinition`, `ProjectSchema`, Command API,
  `project_archive` wire codec, Platform atomic single-file publication and Assets.
  It reads/writes no new Runtime implementation and does not unseal catalog instances.
- The folder layer is in `Game/Editor` (higher-level composition). It owns
  project descriptors, stable source identity and multi-file commit protocol.
- `ProjectFolder::Authoring()` is **read-only**. Use wrapper `Execute/Undo/Redo/Save`
  to avoid writing directly into an immutable committed revision.
- The schema and component registrations are borrowed, just as for EditorProject.
  Callers must keep them alive and stay on the owning thread.

## New portable layout

```
MyGame/
  Project.epidemicproj                    # best-effort, non-authoritative recent descriptor
  ProjectData/
    Commits/0000000000000001.commit       # authoritative, serialized; published LAST
    Revisions/0000000000000001/
      Authoring.epidemic                  # unchanged v2 authoring schema 1.0.0
      Assets.manifest                     # versioned source index (not cooked imports)
  Content/                                # user-owned sources
  Scripts/                                # reserved for P05
  .epidemic/workspace.lock                # OS-managed exclusive writer lock
```

Project filenames, relative paths and metadata are platform-neutral. Project
identity comes from the authoring project, never the install folder or filename.
The commit descriptor contains project/entry IDs, title, generation, relative
immutable file paths and 64-bit *non-cryptographic* content hashes.

## Commit / crash recovery

1. Validate authoring serialization (including retired IDs), source index and
   source hashes. All in-memory modifications remain uncommitted on error.
2. Write immutable `Revisions/N/Authoring.epidemic` via Platform commit.
3. Write immutable `Revisions/N/Assets.manifest` via Platform commit.
4. Re-read both, confirm hashes; atomically publish `Commits/N.commit` **last**.
5. Only after successful marker publication advance `Generation` and the editor
   savepoint; best-effort update the convenience root descriptor.

`Open` searches committed markers newest first. It rejects unsupported future
schemas without silent downgrade, and falls back to an older complete revision
if a newer commit's files are missing/corrupt. **It must not fall back for a
schema-incompatible authoring document**. Unmarked staged/orphan files are
ignored. Old revisions are retained; garbage collection is not implemented.

A process-owned OS writer lock (exclusive Win32 file handle; POSIX flock) prevents
two local editors opening the same folder for mutation. Kernel releases the lock
if the process crashes, although the lock-file directory entry may remain.
This protects cooperating local processes, not malicious filesystem replacement,
network filesystem anomalies, power-loss durability on every storage device or
manual file mutations from another application.

## Public operations

- `Create(root,title,definition,schema)`: root must not exist. Writes generation 1.
- `ImportLegacy(root,legacy,title,schema)`: creates a separate folder, retaining
  legacy retired IDs and not overwriting the source `.epidemic`.
- `Open(root,schema)`: selects a verified committed revision, retains current IDs.
- `Execute/Undo/Redo/Save`: edits use the existing Command API, savepoint advances
  only after the multi-file commit marker. No second mutable authoring tree.
- `IndexSource("Sprites/cat.png",type)`: indexes a file already inside `Content/`.
  **Does not decode PNG**; gives a path-independent AssetId and fingerprints it.
- `ReindexSource(id)`: after source bytes change, increments source generation
  if fingerprint changes; does not create cooked payload or hot-update textures.
- `BuildSourceCatalog()`: fresh, sealed, read-only Runtime Assets metadata snapshot.
- `ResolveMount("Content"/"Scripts"/"ProjectData"/"Cache",relative)`:
  calls existing `ResolveProjectPath` and retains its security limitations.
- `CloneTo(root)`: copies the current **committed** authoring/asset index and
  Content/Scripts into a new root, without cache or lock; preserves project IDs.
  It refuses dirty projects. A clone is not a fork and does not remap identifiers.

## Tests

`EpidemicEditorFolderCreate` => `EpidemicEditorFolderReopen` =>
`EpidemicEditorFolderRecovery` run as separate processes with CTest fixtures.
They exercise create/edit/save, sealed catalog indexing, exclusive lock,
relocation/clone, new-process open/Play, unpublished orphan ignored and
corrupted newest committed authoring falling back to last-good.
All test artifacts stay below `Temp/editor-tests/p04-folder`.

## Important limitations / next packets

1. **No EditorDesktop UI wiring** yet. Existing Open/New/Save menu still handles
   legacy `.epidemic`; call ProjectFolder from C++/headless tests for now.
2. **No production image/audio import or cooked resources**. Source indexing
   only; P04.3 will supply decoders, import recipe, content generation and
   resource publication. Mock P04 spike remains separate and unchanged.
3. **No active ResourceLease generation-swap test.** This P04.0 acceptance item
   must be completed on the real ResourceManager before declaring all of P04.0
   closed. The per-source metadata generation is not a ResourceHandle generation.
4. **No file watchers, async import, progress/cancel, thumbnails, drag/drop or
   Content Browser**; these are P04.2–P04.5.
5. **No user-configured external mounts** (only fixed project-relative mounts);
   external-file links are intentionally not represented as portable dependencies.
6. **No hardened sandbox**: `ResolveProjectPath` can be raced by symlink/reparse
   replacement between checking and opening. Unicode/case-fold collisions beyond
   ASCII, junctions, network shares and handle-based safe open remain open issues.
7. **No asset-specific Undo/Redo, garbage collection or bounded retained history**;
   immutable revision files accumulate until a future tested pruning feature.
8. **The 64 document / 16 MiB authoring limits still apply.** Binary source files
   stay out of the `.epidemic` archive.
9. Build, tests, GUI, DLL-loading, Windows hardening and actual monitor DPI need
   further target-environment verification. Local build/tests are qualified; visible GUI, DLL-loading, Windows hardening and actual monitor DPI are not asserted PASS.

## Migration plan

- Existing `.epidemic` is still readable by old APIs and is never rewritten.
- Use explicit `ProjectFolder::ImportLegacy` for conversion by copying the
  authoring definition and its retired IDs into a separate folder.
- Reject unknown versions; future migration must add an explicit versioned
  converter, not reinterpret old fields.
- Before UI exposure, add an `EditorWorkspace` adapter that selects legacy
  EditorProject or ProjectFolder and routes Save to the correct owner.
