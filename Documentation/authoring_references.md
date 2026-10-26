# Authored asset element references

`AssetElementReference` in RuntimeFoundation contains existing `AssetId` and
`StringId` values. IDs belong to the definition, not its instances. Element IDs
are scoped to an asset. Authoring owners allocate/preserve opaque identity tokens
independently of display names and paths; renaming/moving must not rehash a new
name into an existing identity. This does not change the engine's ID algorithms.

Runtime Support composes the existing Assets location resolver and Serialization.
Its typed serializer stores two uint64 fields under type
`epidemic.asset_element_reference`, schema 1.0.0. The existing serialization
helpers enforce C++ type, document type and schema. Deserialization stages both
IDs before publishing them, so failure leaves the destination unchanged.

The definition owner supplies an `AssetElementIdsView` after loading content.
Resolution checks asset ownership, nonzero/unique element IDs, element existence,
then delegates asset lookup to Assets. Zero IDs, duplicate/colliding IDs and
dangling references fail explicitly. Inventories are borrowed for one call and
are never retained. Resolution returns a logical location, not a resident object.
Instance selection, prefab overrides and instance-to-instance links are separate
future contracts. Handles must be reacquired through Resources after eviction.

The fixture uses two prefab instances, an existing Resources loader/leases and
an existing SerializedDocument archive. It preserves a reference through display
rename, asset move, catalog recreation and resource eviction/reload, and verifies
stale handle rejection. The current Serialization backend is in-memory; this
package does not add a disk wire format, graph runtime or full prefab system.
