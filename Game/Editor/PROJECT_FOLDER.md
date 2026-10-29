# Portable project folder contract

ProjectFolder owns the file envelope; EditorProject remains the only authoring
model and is exposed read-only. All Scene/Prefab/Graph mutations use the existing
Command API through Execute/Undo/Redo. Schema/component registrations are borrowed
and must outlive the folder and its PlaySession.

## Layout and publication

- `Project.epidemicproj`: convenience descriptor, not the recovery authority.
- `ProjectData/Commits/N.commit`: Serialization descriptor schema 1.0.0, published last.
- `ProjectData/Revisions/N/Authoring.epidemic`: unchanged authoring archive 1.0.0,
  including retired ID reservations.
- `ProjectData/Revisions/N/Assets.manifest`: source index schema 1.1.0; the previous
  1.0.0 remains readable. Source records include stable ID/type, relative location,
  fingerprint/generation and optional immutable cooked SHA-256 key.
- `Content/`, `Scripts/`: portable sources. Binary data stays outside authoring.
- `.epidemic/Cache/images/`: derived cooked images; Clone copies these artifacts.
- `.epidemic/workspace.lock`: process-owned OS writer lock, not a stale PID marker.
- `.epidemic/source-moves.journal`: bounded source-move recovery protocol.

Save validates authoring/index, stages immutable files through Platform atomic
packages, reads them back, and publishes the commit marker last. Only then does
it advance the authoring/source savepoint. Missing/changed sources remain visible
as diagnostics and do not invalidate otherwise-saveable authoring. Source bytes
are user-owned and are not themselves rolled back by an authoring snapshot.

Open holds the writer lock while selecting/recovering the newest intact committed
revision. An unsupported future schema or incompatible game schema is a controlled
failure, not a silent downgrade. Unmarked files are ignored. Uncommitted source
moves roll back; ambiguous recovery preserves files and reports an error.

## Operations

Create requires a nonexistent destination. ImportLegacy copies the old archive
and retired IDs into a separate new folder, without changing the original. Clone
preserves identities and requires a committed project; it copies Content, Scripts,
current immutable revision, marker and cooked images. It is not a cross-project
remapping operation. Relocating the complete project retains relative paths.

EditorWorkspace routes commands, Undo/Redo, Save and Play to the active folder or
legacy EditorProject. New-folder and Clone menu dialogs take the desired folder
name (the `.epidemicproj` suffix is removed); Open selects the actual descriptor.
Project-level dirty state includes asset metadata, not just authoring documents.
Move Undo/Redo is separate from authoring Undo/Redo; see CONTENT_BROWSER_AND_PNG.md.

## Acceptance gaps

Full P04.1 is not complete: configurable external/link mounts, profile/feature
manifest fields, complete Unicode collision policy, handle-bound race-resistant
I/O and crash injection at every publication boundary are not implemented.
Existing symlinks/reparse components, traversal/device paths and overwrites are
rejected, but this is not a sandbox against hostile concurrent filesystem changes.

Revision garbage collection is not implemented. Authoring still has its existing
64 document / 16 MiB limits. ImportLegacy is available through the backend, not a
native migration wizard. Visible UI, actual monitor DPI and network-filesystem
behavior require manual acceptance.

Tests: FolderCreate/Reopen/Recovery cover separate-process commit/clone/legacy
migration, lock, orphan rejection and corrupted-revision fallback. ContentTests,
ContentReopen and ContentDesktopSmoke cover the real PNG/resource/preview path.
All test files are under Temp.
