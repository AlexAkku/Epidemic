#pragma once

#include "Epidemic/Runtime/Graph/graph_definition.h"
#include "Epidemic/Runtime/Serialization/serializer.h"

#include <utility>

namespace epidemic::runtime {
// Owns a type-ID snapshot from the caller's schema catalog, not a type registry.
// Use the existing typed SerializeToDocument/DeserializeObject boundary.
class GraphDefinitionSerializer final : public ISerializer {
  public:
    explicit GraphDefinitionSerializer(std::vector<graph::TypeId> known_types)
        : known_types_(std::move(known_types)) {
    }

    [[nodiscard]] foundation::StringId GetTypeId() const override;
    [[nodiscard]] SchemaVersion GetSchemaVersion() const override;
    [[nodiscard]] std::type_index GetCppType() const override;

  protected:
    [[nodiscard]] foundation::Result<void> Serialize(const void *object,
                                                     IArchiveWriter &writer) const override;
    [[nodiscard]] foundation::Result<void> Deserialize(IArchiveReader &reader,
                                                       void *object) const override;

  private:
    std::vector<graph::TypeId> known_types_;
};

} // namespace epidemic::runtime
