#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace epidemic::game::editor::sheets {
// No mutable implicit slot numbers: frames are keyed by persistent, nonzero
// IDs. World-space pivot lives with PrefabSpriteBinding; region pixels are
// independent.
struct Region {
  std::uint64_t id = 0;
  std::string name;
  std::uint32_t x = 0, y = 0, width = 0, height = 0;
  float pivot_x = .5f, pivot_y = .5f;
  bool missing =
      false; // A deleted frame is a tombstone, never re-bound to another frame.
  [[nodiscard]] bool operator==(const Region &) const = default;
};
struct Grid {
  std::uint32_t image_width = 0, image_height = 0;
  std::uint32_t cell_width = 0, cell_height = 0;
  std::uint32_t origin_x = 0, origin_y = 0;
  std::uint32_t padding_x = 0, padding_y = 0;
};
struct Rect {
  std::uint32_t x = 0, y = 0, width = 0, height = 0;
};
struct PackedRegion {
  std::uint64_t id = 0;
  Rect rect;
};
struct AtlasLayout {
  std::uint32_t width = 0, height = 0;
  std::vector<PackedRegion> regions;
};
template <class T> struct Result {
  T value{};
  std::string error;
  [[nodiscard]] explicit operator bool() const noexcept {
    return error.empty();
  }
};
// StableID is explicitly independent of input order, grid row, and packed
// coordinates. Renaming requires a caller-supplied existing region id
// (Reconcile), not a silent rebinding.
[[nodiscard]] std::uint64_t StableId(std::uint64_t parent,
                                     std::string_view persistent_key) noexcept;
[[nodiscard]] Result<std::vector<Region>>
BuildGrid(std::uint64_t parent, const Grid &grid,
          std::string_view prefix = "frame");
[[nodiscard]] Result<std::vector<Region>>
Validate(std::uint32_t width, std::uint32_t height,
         std::span<const Region> regions);
// Preserve IDs by logical name, retain deleted regions as missing tombstones.
[[nodiscard]] Result<std::vector<Region>>
Reconcile(std::span<const Region> existing, std::span<const Region> edited);
// Deterministic shelf packing. Sorted by stable ID, not input order. Does not
// move logical IDs.
[[nodiscard]] Result<AtlasLayout> Pack(std::span<const Region> regions,
                                       std::uint32_t max_width,
                                       std::uint32_t max_height,
                                       std::uint32_t padding = 1);
// CPU normalized PBGRA8 crop of a slice. No codec or GPU dependency.
[[nodiscard]] Result<std::vector<std::byte>>
ExtractPbgra(std::span<const std::byte> image, std::uint32_t width,
             std::uint32_t height, const Region &region);
// Deterministic premultiplied BGRA atlas pixels. Unused padding stays
// transparent.
[[nodiscard]] Result<std::vector<std::byte>>
ComposeAtlasPbgra(std::span<const std::byte> image, std::uint32_t width,
                  std::uint32_t height, std::span<const Region> regions,
                  const AtlasLayout &layout);
// Portable textual metadata that can be stored as a user-authored Content
// sidecar. Strict size and character bounds; file I/O/atomic save stays with
// ProjectFolder.
[[nodiscard]] Result<std::string> Encode(std::uint64_t parent,
                                         std::span<const Region> regions);
[[nodiscard]] Result<std::vector<Region>> Decode(std::uint64_t expected_parent,
                                                 std::string_view text);
} // namespace epidemic::game::editor::sheets
