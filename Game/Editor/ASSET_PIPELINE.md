# Asset pipeline: P04.0 contract draft

Status: investigation and test-only mock spike. No production project-folder, import,
browser or sprite-asset API is implemented by this document.

## Existing owners

- Assets provides immutable-after-Seal catalog snapshots and a runtime-thread writer.
  Asset metadata includes ID, type, location, dependencies and per-asset version.
- `AssetElementReference` is an asset ID plus an element ID scoped to that asset.
  Existing Support resolves and serializes it without retaining runtime pointers.
- Resources owns leases, bounded processing, eviction and generation-aware handles;
  its mutation contract is runtime-thread only. Streaming is not a general importer.
- EditorProject owns authoring commands and existing archive 1.0.0. Its bounded
  single-file commit uses Platform `CommitFilePackage`; this does not make a
  multi-file project atomic. Binary source files must not be embedded in that archive.
- Windows Presentation decodes embedded resource bytes via WIC in `Renderer::Image`.
  This API accepts a resource ID; external project images require a reviewed adapter
  and renderer-thread lifecycle. Scene View currently has no sprite bindings.

## Proposed constraints for implementation

- Keep authored Scene/Prefab/Graph in EditorProject and source, manifest and cooked
  assets separate. A versioned project descriptor must use relative mount paths.
- Generate a nonzero stable AssetId independently from filenames/content; detect
  collisions and preserve identity on rename, move and reimport. Cross-project remap
  and subasset matching must be explicit and preserve references.
- Publish a fresh sealed catalog generation, never mutate a sealed catalog. Publish
  resource updates on the owner thread and retain last-good data while old leases live.
- Validate canonical paths at I/O boundaries, including symlink/junction escape and
  case collisions. Define crash recovery for multi-file transactions before claiming
  atomic project save. Keep old .epidemic files readable without implicit rewriting.
- Prefer a separately validated typed Sprite asset/element slot; do not encode a
  magic string in a generic property or change archive 1.0.0 interpretation.

## Required before P04.0 acceptance

The test-only `EpidemicEditorP04SpikeWrite/Read` runs in separate processes and
exercises Serialization, Platform atomic single-file commits and sealed Assets with
a fixed ID, mock reimport generation and invalid-candidate last-good preservation.
It does not define a production descriptor or decode PNG. Choose and test the
production schema and multi-file commit/recovery protocol, and resolve resource
publication with in-flight leases.

## Subsequent gates

PNG decoding, browser UI, crash injection and runtime placement binding require
their own gates. WebAssembly/Web builds are mandatory before release and scheduled
after the remaining functionality; no Web build is required in P04.0.
