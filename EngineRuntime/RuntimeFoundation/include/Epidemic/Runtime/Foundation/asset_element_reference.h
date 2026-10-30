#pragma once

#include "Epidemic/Runtime/Foundation/runtime_ids.h"

namespace epidemic::runtime {
// Definition identity, never an instance/slot/generation or a display path.
// The authoring owner preserves both IDs when renaming/moving content.
// Element IDs are scoped to their asset; duplicate IDs must be rejected on load.
struct AssetElementReference {
    AssetId asset_id{};
    foundation::StringId element_id{};

    [[nodiscard]] constexpr bool IsValid() const noexcept {
        return asset_id.IsValid() && element_id.IsValid();
    }
    [[nodiscard]] constexpr bool operator==(const AssetElementReference &) const noexcept = default;
};
} // namespace epidemic::runtime
