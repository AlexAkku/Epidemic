#pragma once
#include "Epidemic/Game/Editor/project_folder.h"
#include <variant>

namespace epidemic::game::editor {
struct AssetSetSprite {
  PrefabSpriteBinding binding;
};
struct AssetSetSheet {
  SpriteSheetDefinition sheet;
  std::uint32_t width = 0, height = 0;
};
using AssetCommand = std::variant<AssetSetSprite, AssetSetSheet>;
struct AssetEditRequest {
  std::uint64_t expected_revision = 0;
  std::vector<AssetCommand> commands;
};
} // namespace epidemic::game::editor
