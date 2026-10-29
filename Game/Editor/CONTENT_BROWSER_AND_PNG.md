# P04.2/P04.3a incremental backend — status and scope

Base: `v2` commit `87715cb8cc6303a8d8d5905854921fbfa84fe236`.

## Implemented

- Read-only, sorted Content Browser view of `ProjectFolder::Sources()` with path-prefix, case-insensitive ASCII search, type filter, folder discovery, presence indicator and generation metadata.
- Move/rename indexed content source within an existing Content folder. Preserves stable AssetId, rejects traversal, destination collision and overwrite, marks project dirty; committed by `ProjectFolder::Save()`. This is a filesystem operation, not EditorProject Undo.
- Real PNG signature, WIC decode to bounded 32-bit premultiplied BGRA pixels on Windows; 4096 pixels per side and 128 MiB source-file maximum. Invalid/oversized/corrupt inputs rejected before indexing.
- Index existing project PNG and copy/import external PNG to an existing Content folder, using `ProjectFolder::IndexSource()` and its source fingerprint; no `.rc`/EXE rebuild.
- Headless tests for filtering/move/reopen, rejecting malformed/truncated PNG and Windows decoding of a real 1x1 RGBA PNG with premultiplied-alpha verification and persisted AssetId.

## Not yet implemented — do not mark P04.2 or P04.3 COMPLETE

- Content Browser GUI, thumbnails, drag-and-drop, tags, multi-select, soft delete/undo, references graph, import progress.
- Cooked textures, actual GPU sprite view, atlas/tilesets, WebP, WAV/Ogg, fonts, live reimport, Scene Sprite binding.
- Filesystem security and transactions across source moves, journalled rollback, externally linked assets and cancellation.
- The new `DecodedPng` byte buffer is an import/preview primitive only; it is not a ResourceLease, cooked artifact, or renderer texture.

## Integration contract

`ProjectFolder` owns source identity. `BrowseContent` is detached and must not become a second asset registry. UI commands later call these APIs through the existing `EditorWorkspace` integration. WIC decode runs on calling thread today: schedule through bounded worker jobs before exposing large imports from UI.

## Test

Configure your existing Windows build directory, build `EpidemicEditorContentTests` and run `ctest -R EpidemicEditorContent`. Also run `EpidemicEditorFolder` and complete native Debug build to catch ABI or linker problems. Current Windows Debug build, public headers and 158 CTest cases passed in the local qualification. This does not establish the missing GUI/cooked/resource acceptance gates.
