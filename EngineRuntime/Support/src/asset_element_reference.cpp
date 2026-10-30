#include "Epidemic/Runtime/Support/asset_element_reference.h"

#include <unordered_set>

namespace epidemic::runtime {
namespace {
foundation::Error ReferenceError(std::string_view code) {
    return foundation::Error::Create(code, "authored asset element reference rejected");
}
} // namespace

foundation::Result<AssetLocation> ResolveAssetElementReference(
    const AssetElementReference &reference, const AssetElementIdsView &inventory, const IAssetLocationResolver &assets) {
    if (!reference.IsValid())
        return foundation::Result<AssetLocation>::Failure(ReferenceError("authoring.invalid_reference"));
    if (inventory.asset_id != reference.asset_id)
        return foundation::Result<AssetLocation>::Failure(ReferenceError("authoring.asset_mismatch"));
    std::unordered_set<foundation::StringId> ids;
    for (const auto id : inventory.elements) {
        if (!id.IsValid())
            return foundation::Result<AssetLocation>::Failure(ReferenceError("authoring.invalid_element"));
        if (!ids.insert(id).second)
            return foundation::Result<AssetLocation>::Failure(ReferenceError("authoring.element_collision"));
    }
    if (!ids.contains(reference.element_id))
        return foundation::Result<AssetLocation>::Failure(ReferenceError("authoring.element_not_found"));
    return assets.Resolve(reference.asset_id);
}

foundation::StringId AssetElementReferenceSerializer::GetTypeId() const {
    return foundation::StringId::FromString("epidemic.asset_element_reference");
}
SchemaVersion AssetElementReferenceSerializer::GetSchemaVersion() const { return {1, 0, 0}; }
std::type_index AssetElementReferenceSerializer::GetCppType() const { return typeid(AssetElementReference); }

foundation::Result<void> AssetElementReferenceSerializer::Serialize(const void *object, IArchiveWriter &writer) const {
    const auto &reference = *static_cast<const AssetElementReference *>(object);
    if (!reference.IsValid())
        return foundation::Result<void>::Failure(ReferenceError("authoring.invalid_reference"));
    auto asset = writer.WriteUInt64("asset_id", reference.asset_id.Raw());
    if (!asset) return asset;
    return writer.WriteUInt64("element_id", reference.element_id.Raw());
}

foundation::Result<void> AssetElementReferenceSerializer::Deserialize(IArchiveReader &reader, void *object) const {
    const auto asset = reader.ReadUInt64("asset_id");
    if (!asset) return foundation::Result<void>::Failure(asset.GetError());
    const auto element = reader.ReadUInt64("element_id");
    if (!element) return foundation::Result<void>::Failure(element.GetError());
    const AssetElementReference candidate{AssetId{foundation::StringId{asset.Value()}},
                                           foundation::StringId{element.Value()}};
    if (!candidate.IsValid())
        return foundation::Result<void>::Failure(ReferenceError("authoring.invalid_reference"));
    *static_cast<AssetElementReference *>(object) = candidate;
    return foundation::Result<void>::Success();
}
} // namespace epidemic::runtime
