#pragma once

#include "Epidemic/Runtime/Assets/asset_location_resolver.h"
#include "Epidemic/Runtime/Foundation/asset_element_reference.h"
#include "Epidemic/Runtime/Serialization/serializer.h"

#include <span>

namespace epidemic::runtime {
// Borrowed inventory supplied by the loaded definition owner, not a second
// catalog. The view and its IDs must remain stable for the duration of
// resolution.
struct AssetElementIdsView {
  AssetId asset_id{};
  std::span<const foundation::StringId> elements;
};

// Resolves logical location only, without loading content or returning object
// pointers. Rejects a mismatched inventory, zero/duplicate element IDs and
// dangling references. Allocation/callback exceptions follow the existing
// Assets contract.
[[nodiscard]] foundation::Result<AssetLocation>
ResolveAssetElementReference(const AssetElementReference &reference,
                             const AssetElementIdsView &inventory,
                             const IAssetLocationResolver &assets);

// Native typed adapter for existing Serialization helpers. Schema 1.0.0
// contains exactly asset_id/element_id uint64 fields, with no
// name/path/handle/pointer data.
class AssetElementReferenceSerializer final : public ISerializer {
public:
  [[nodiscard]] foundation::StringId GetTypeId() const override;
  [[nodiscard]] SchemaVersion GetSchemaVersion() const override;
  [[nodiscard]] std::type_index GetCppType() const override;

protected:
  [[nodiscard]] foundation::Result<void>
  Serialize(const void *object, IArchiveWriter &writer) const override;
  [[nodiscard]] foundation::Result<void>
  Deserialize(IArchiveReader &reader, void *object) const override;
};
} // namespace epidemic::runtime
