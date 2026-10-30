# Content Browser and PNG MVP

Open a portable project descriptor from File / Open, or use File / New project
folder. The new-folder dialog takes a name: selecting `Parent/MyGame.epidemicproj`
creates `Parent/MyGame/Project.epidemicproj`. File / Clone project folder copies
committed portable data without changing IDs; save pending edits first.

View / Content Browser opens a native command client for the active ProjectFolder.
It supports path search, stable ID/generation/presence information, PNG import,
reimport, rename/move, duplicate, staged delete/restore, global Undo/Redo, Save and resident thumbnail
preview. Shared command history is bounded to 64 operations and 128 pending journal
steps between saves. History is session-local; recovery journal is durable.

The destination field is relative to Content (including the filename). Clear it
before Import to use the selected external filename. Destination directories must
already exist. Main-window and Browser Undo/Redo use the same project history. Import/duplicate,
staged delete/restore and source moves are undoable; reimport generation publication
remains independent. References displays incoming dependents before deletion.
Show trash exposes retained identities; Restore moves bytes to the original path.

## PNG / resource contract

- Windows WIC; signature/container checks, maximum 128 MiB source, 4096 per side.
- Cooked Serialization document including key, recipe, dimensions and PBGRA8 pixels.
  Its current 16 MiB archive budget also limits cooked pixel bytes (4096 bytes of
  headroom reserved); large images can decode but fail cooking without publication.
- Fixed sRGB, premultiplied BGRA8, top-down, nearest/clamp, no mipmap or trim.
  SHA-256 includes source bytes and canonical recipe/importer/profile version.
- Immutable `.epidemic/Cache/images/<key>.image`; source index schema 1.4.0 stores
  last-good key. Clone copies image artifacts; deleting cache requires Reimport
  from valid source. Preview uses existing Resources then Presentation texture API.
- Corrupt/missing source does not erase references or prevent saving authoring.
  Failed/cancelled reimport retains key/generation and existing resource leases.
- Preview manager budget 64 MiB, unused key registrations are reclaimed after releasing leases;
  textures use at most 128x128 thumbnails. Close/reopen releases session resources.
  Imports and cooked preview reads run on bounded workers. Cancelling prevents
  unpublished import completion; final window/service destruction drains owned work.

## Remaining P04.2 work

Folder tree/breadcrumb, empty-folder commands, grid mode, GUI type/status/tag
filters, multi-selection, cross-project remapping and typed drag/drop are not implemented. The current
browser must not be described as complete P04.2. Typed Prefab Sprite assignment and Scene rendering are available through P04.3b.

## Verification

`EpidemicEditorContentTests` verifies real PNG pixels, cooked data, lease lifetime,
identity, reimport failures/cancellation, source-move history and recovery.
`EpidemicEditorContentReopen` opens a relocated clone in another process.
`EpidemicEditorContentDesktopSmoke` uploads and displays thumbnails in an owned
hidden native window. Manual DPI, focus, appearance and visible UX still require
acceptance; no Computer Use is required by these tests.
