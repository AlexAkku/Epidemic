#include "Epidemic/Runtime/Assets/asset_services.h"
#include "Epidemic/Runtime/Resources/resource_services.h"
#include "Epidemic/Runtime/Serialization/serialization_services.h"
#include "Epidemic/Runtime/Support/asset_element_reference.h"

#include <array>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

using namespace epidemic;
using namespace epidemic::runtime;
namespace {
void Check(bool value, const char *message) {
  if (!value) {
    std::fputs(message, stderr);
    std::fputc('\n', stderr);
    throw std::runtime_error(message);
  }
}
template <typename T>
void ErrorIs(const foundation::Result<T> &value, std::string_view code) {
  Check(!value && value.GetError().HasCode(code),
        "unexpected authored reference failure");
}
struct PrefabPayload final : IResourcePayload {
  PrefabPayload(foundation::StringId element, std::string label)
      : elements{element}, name(std::move(label)) {}
  std::size_t GetSizeBytes() const noexcept override {
    return sizeof(PrefabPayload);
  }
  std::array<foundation::StringId, 1> elements;
  std::string name;
};
class PrefabLoader final : public IResourceLoader {
public:
  const foundation::StringId element_id =
      foundation::StringId::FromString("identity.prefab.element.1");
  std::string element_name = "Entry";
  ResourceType GetResourceType() const override {
    return {foundation::StringId::FromString("fixture.prefab")};
  }
  foundation::Result<ResourceLoadArtifact>
  Load(ResourceRequest request) override {
    return foundation::Result<ResourceLoadArtifact>::Success(
        {request.resource_id,
         GetResourceType(),
         std::make_shared<PrefabPayload>(element_id, element_name),
         {}});
  }
};
struct PrefabInstance {
  AssetElementReference reference;
  ResourceLease lease;
  int counter{};
};
AssetMetadata Metadata(AssetId id, std::string path) {
  AssetMetadata result;
  result.id = id;
  result.type = {foundation::StringId::FromString("fixture.prefab")};
  result.location.path = std::move(path);
  result.state = AssetState::Validated;
  result.content_hash = 1;
  result.version = 1;
  return result;
}
AssetElementReference ReadReference(const SerializedDocument &document) {
  // Fresh services: no source objects, loader or writer state is used to decode
  // IDs.
  auto serialization = CreateSerializationServices().Value();
  auto reader = serialization.archives->CreateReader(document).Value();
  AssetElementReference result;
  Check(DeserializeObject(AssetElementReferenceSerializer{}, *reader, result)
            .HasValue(),
        "reference reload");
  return result;
}
void RoundTrip() {
  PrefabLoader
      loader; // Registry borrows this; it outlives the resource services.
  auto resources = CreateResourceServices().Value();
  auto assets = CreateAssetServices().Value();
  const auto asset_id = AssetId::FromString("identity.prefab.1");
  const auto metadata = Metadata(asset_id, "prefabs/original.prefab");
  Check(assets.writer->RegisterAsset(metadata).HasValue(), "register prefab");
  Check(assets.writer->Seal().HasValue(), "seal catalog");
  Check(resources.loaders->RegisterLoader(loader).HasValue(),
        "register prefab loader");
  Check(resources.loaders->Freeze().HasValue(), "freeze loaders");
  // Fixture-local one-to-one mapping; persistent references do not store
  // ResourceId.
  const ResourceRequest request{ResourceId{asset_id.value},
                                loader.GetResourceType()};
  const AssetElementReference reference{asset_id, loader.element_id};
  PrefabInstance first{reference,
                       resources.manager->RequestLease(request).Value(), 11};
  PrefabInstance second{reference,
                        resources.manager->RequestLease(request).Value(), 22};
  Check(resources.manager->ProcessPendingLoads().Value().loaded_resources == 1,
        "shared definition load");
  Check(first.lease.resource == second.lease.resource,
        "instances share definition residency");
  first.counter = 17;
  Check(second.counter == 22 && first.reference == second.reference,
        "independent instance state");
  std::weak_ptr<const IResourcePayload> old_payload;
  {
    const auto payload = std::dynamic_pointer_cast<const PrefabPayload>(
        resources.manager->GetPayload(first.lease.resource));
    Check(payload && payload->name == "Entry", "original element name");
    old_payload = payload;
    Check(ResolveAssetElementReference(first.reference,
                                       {asset_id, payload->elements},
                                       *assets.location_resolver)
                  .Value()
                  .path == "prefabs/original.prefab",
          "resolve original reference");
  }
  SerializedDocument saved;
  {
    auto serialization = CreateSerializationServices().Value();
    saved = SerializeToDocument(AssetElementReferenceSerializer{},
                                first.reference, *serialization.archives)
                .Value();
  }
  const auto old_handle = first.lease.resource;
  Check(resources.manager->Release(first.lease).HasValue(),
        "release first instance lease");
  Check(resources.manager->IsReady(second.lease.resource),
        "second instance keeps definition resident");
  Check(resources.manager->Release(second.lease).HasValue(),
        "release second instance lease");
  Check(resources.manager->Evict(request.resource_id).HasValue(),
        "unload prefab");
  Check(old_payload.expired(), "old definition was destroyed");
  Check(!resources.manager->ValidateHandle(old_handle),
        "stale handle after eviction");
  Check(!resources.manager->GetPayload(old_handle),
        "stale handle exposes no payload");

  assets = CreateAssetServices()
               .Value(); // Catalog reload, not an in-place identity rewrite.
  Check(
      assets.writer->RegisterAsset(Metadata(asset_id, "shared/renamed.prefab"))
          .HasValue(),
      "move/rename asset");
  Check(assets.writer->Seal().HasValue(), "seal reloaded catalog");
  loader.element_name =
      "Begin"; // Identity remains definition-owned, independent of this label.
  first.reference = ReadReference(saved);
  second.reference = ReadReference(saved);
  Check(first.reference == reference && second.reference == reference,
        "round-trip preserves both IDs");
  first.lease = resources.manager->RequestLease(request).Value();
  second.lease = resources.manager->RequestLease(request).Value();
  Check(resources.manager->ProcessPendingLoads().Value().loaded_resources == 1,
        "reload shared definition");
  Check(first.lease.resource != old_handle,
        "reload changes runtime generation");
  Check(!resources.manager->ValidateHandle(old_handle),
        "stale generation remains invalid after reload");
  {
    const auto payload = std::dynamic_pointer_cast<const PrefabPayload>(
        resources.manager->GetPayload(first.lease.resource));
    Check(payload && payload->name == "Begin", "renamed element loaded");
    for (const auto &instance : {first, second}) {
      Check(ResolveAssetElementReference(instance.reference,
                                         {asset_id, payload->elements},
                                         *assets.location_resolver)
                    .Value()
                    .path == "shared/renamed.prefab",
            "saved reference survives rename/move/reload");
    }
  }
  Check(first.counter == 17 && second.counter == 22,
        "reload preserves separate instance state");
  Check(resources.manager->Release(first.lease).HasValue(),
        "release reloaded first lease");
  Check(resources.manager->Release(second.lease).HasValue(),
        "release reloaded second lease");
}
void InvalidReferences() {
  auto assets = CreateAssetServices().Value();
  const auto asset = AssetId::FromString("identity.prefab.1");
  const auto element =
      foundation::StringId::FromString("identity.prefab.element.1");
  const AssetElementReference reference{asset, element};
  const std::array elements{element};
  Check(assets.writer->RegisterAsset(Metadata(asset, "original.prefab"))
            .HasValue(),
        "register valid metadata");
  ErrorIs(assets.writer->RegisterAsset(Metadata(asset, "collision.prefab")),
          "asset.duplicate");
  Check(assets.location_resolver->Resolve(asset).Value().path ==
            "original.prefab",
        "collision preserves original");
  ErrorIs(ResolveAssetElementReference({}, {asset, elements},
                                       *assets.location_resolver),
          "authoring.invalid_reference");
  ErrorIs(ResolveAssetElementReference(reference, {{}, elements},
                                       *assets.location_resolver),
          "authoring.asset_mismatch");
  const std::array collision{element, element};
  ErrorIs(ResolveAssetElementReference(reference, {asset, collision},
                                       *assets.location_resolver),
          "authoring.element_collision");
  const std::array invalid{foundation::StringId{}};
  ErrorIs(ResolveAssetElementReference(reference, {asset, invalid},
                                       *assets.location_resolver),
          "authoring.invalid_element");
  ErrorIs(ResolveAssetElementReference(
              {asset, foundation::StringId::FromString("missing")},
              {asset, elements}, *assets.location_resolver),
          "authoring.element_not_found");
  const auto missing = AssetId::FromString("missing.asset");
  ErrorIs(ResolveAssetElementReference({missing, element}, {missing, elements},
                                       *assets.location_resolver),
          "asset.not_found");
  Check(ResolveAssetElementReference(reference, {asset, elements},
                                     *assets.location_resolver)
            .HasValue(),
        "invalid references do not mutate accepted content");
}
void InvalidDocuments() {
  const AssetElementReference expected{
      AssetId::FromString("identity.prefab.1"),
      foundation::StringId::FromString("identity.element.1")};
  AssetElementReferenceSerializer serializer;
  auto serialization = CreateSerializationServices().Value();
  ErrorIs(SerializeToDocument(serializer, AssetElementReference{},
                              *serialization.archives),
          "authoring.invalid_reference");
  ErrorIs(SerializeToDocument(serializer, ResourceHandle{},
                              *serialization.archives),
          "serialization.cpp_type_mismatch");
  for (int mode = 0; mode < 6; ++mode) {
    auto writer = serialization.archives->CreateWriter();
    Check(writer->WriteUInt64("asset_id", expected.asset_id.Raw()).HasValue(),
          "write asset field");
    if (mode == 0)
      Check(writer->WriteUInt64("element_id", 0).HasValue(),
            "write invalid ID");
    else if (mode == 1)
      Check(writer->WriteString("element_id", "wrong type").HasValue(),
            "write wrong type");
    else if (mode != 2)
      Check(writer->WriteUInt64("element_id", expected.element_id.Raw())
                .HasValue(),
            "write element field");
    const auto type = mode == 3
                          ? foundation::StringId::FromString("wrong.document")
                          : serializer.GetTypeId();
    const auto version =
        mode == 4 ? SchemaVersion{2, 0, 0} : serializer.GetSchemaVersion();
    const auto document = writer->Finalize(type, version).Value();
    auto reader = serialization.archives->CreateReader(document).Value();
    auto target = expected;
    if (mode == 5) {
      int wrong_target = 9;
      ErrorIs(DeserializeObject(serializer, *reader, wrong_target),
              "serialization.cpp_type_mismatch");
      Check(wrong_target == 9, "wrong C++ destination untouched");
    } else {
      const auto result = DeserializeObject(serializer, *reader, target);
      Check(!result, "invalid document accepted");
      if (mode == 0)
        ErrorIs(result, "authoring.invalid_reference");
      if (mode == 3)
        ErrorIs(result, "serialization.type_mismatch");
      if (mode == 4)
        ErrorIs(result, "serialization.schema_mismatch");
      Check(target == expected, "failed decode mutated reference");
    }
  }
}
} // namespace
int main() {
  try {
    RoundTrip();
    InvalidReferences();
    InvalidDocuments();
    return 0;
  } catch (...) {
    std::fputs("authored reference tests failed\n", stderr);
    return 1;
  }
}
