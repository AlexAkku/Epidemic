#include "Epidemic/Game/Editor/sprite_sheet.h"
#include <cstdio>
#include <stdexcept>
#include <source_location>
#include <algorithm>
#include <array>
#include <iostream>
using namespace epidemic::game::editor::sheets;

template <class T>
void Check(const T &value, const std::source_location at = std::source_location::current()) {
    if (!static_cast<bool>(value))
        throw std::runtime_error(std::string(at.file_name()) + ":" + std::to_string(at.line()));
}

int main() {
    try {
        const Grid grid{64, 64, 16, 16, 0, 0, 0, 0};

        auto a = BuildGrid(9988, grid, "walk");
        Check(a && a.value.size() == 16);
        auto b = BuildGrid(9988, grid, "walk");
        Check(b && b.value == a.value);
        auto valid = Validate(64, 64, a.value);
        Check(valid);
        auto renamed = a.value;
        renamed[0].name = "renamed-frame";
        const auto rename_result = Reconcile(a.value, renamed);
        if (!rename_result || rename_result.value.size() != a.value.size() ||
            std::none_of(rename_result.value.begin(), rename_result.value.end(),
                         [&](const Region &r) {
                             return r.id == a.value[0].id && r.name == "renamed-frame" &&
                                    !r.missing;
                         })) {
            std::cerr << "Explicit region rename must preserve its stable ID without adding a "
                         "tombstone\n";
            return 1;
        }

        auto packed = Pack(a.value, 128, 128, 2);
        Check(packed && packed.value.regions.size() == 16);
        std::reverse(a.value.begin(), a.value.end());
        auto packed2 = Pack(a.value, 128, 128, 2);
        Check(packed2 && packed.value.regions.size() == packed2.value.regions.size());
        for (size_t i = 0; i < packed.value.regions.size(); ++i) {
            Check(packed.value.regions[i].id == packed2.value.regions[i].id);
        }

        auto serialized = Encode(9988, a.value);
        Check(serialized);
        auto decoded = Decode(9988, serialized.value);
        Check(decoded && decoded.value.size() == 16);
        Check(!Decode(1234, serialized.value));
        auto modified = decoded.value;
        modified.pop_back();
        auto reconciled = Reconcile(decoded.value, modified);
        Check(reconciled && reconciled.value.size() == 16 &&
              std::count_if(reconciled.value.begin(), reconciled.value.end(),
                            [](const auto &r) { return r.missing; }) == 1);
        const auto retained = Reconcile(reconciled.value, reconciled.value);
        if (!retained || retained.value != reconciled.value) {
            std::cerr << "Reapplying unchanged metadata must retain tombstones\n";
            return 1;
        }

        // A recreated frame may not adopt a retired logical name or its old ID.
        const auto retired = std::find_if(reconciled.value.begin(), reconciled.value.end(),
                                          [](const auto &r) { return r.missing; });
        Check(retired != reconciled.value.end());
        auto reused = *retired;
        reused.id = StableId(9988, "new-frame");
        reused.missing = false;
        const auto forbidden = Reconcile(reconciled.value, std::span<const Region>(&reused, 1));
        Check(!forbidden && forbidden.error == "sheet.retired_name");
        std::vector<std::byte> pixels(std::size_t{64} * 64 * 4);
        pixels[0] = std::byte{12};

        auto cropped = ExtractPbgra(pixels, 64, 64, decoded.value[0]);
        Check(cropped && cropped.value.size() == std::size_t{16} * 16 * 4);
        auto wrong = decoded.value[0];
        wrong.x = 63;
        Check(!ExtractPbgra(pixels, 64, 64, wrong));
        auto oversized = decoded.value[0];
        oversized.missing = true;
        oversized.x = 4097;
        if (Encode(9988, std::span<const Region>(&oversized, 1)) ||
            Validate(64, 64, std::span<const Region>(&oversized, 1))) {
            std::cerr << "Tombstone metadata must be rejected before publishing values that Decode "
                         "refuses\n";
            return 1;
        }

        auto atlas_pixels = ComposeAtlasPbgra(pixels, 64, 64, decoded.value, packed.value);
        Check(atlas_pixels &&
              atlas_pixels.value.size() ==
                  static_cast<std::size_t>(packed.value.width) * packed.value.height * 4);
        Check(!BuildGrid(0, grid));
        std::cout << "sheet layout/identity/metadata/crop PASS\n";
    } catch (const std::exception &error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    } catch (...) {
        std::fputs("Unexpected test failure\n", stderr);
        return 1;
    }

}
