#include "Epidemic/GameFramework/SaveGame/save_game.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <chrono>
#include <memory>
#include <string>
#if defined(EP_AUDITOR_SHADOW_PROBES) && EP_AUDITOR_SHADOW_PROBES
#include <shadow_probe.h>
#endif

using namespace epidemic::gameplay;
using namespace epidemic::gameplay::savegame;
namespace {
void WireCheck(bool value, const char *message) {
    if (!value) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}
SaveGameImage WireImage() {
    SaveGameImage image;
    image.manifest.content_version = 4;
    image.manifest.content_hash = 99;
    image.manifest.engine_version = 7;
    image.manifest.framework_version = 8;
    image.manifest.saved_at.ticks = -17;
    const auto a = SaveParticipantId::FromString("fixture.a");
    const auto b = SaveParticipantId::FromString("fixture.b");
    for (auto id : {b, a}) {
        SaveSection section{id, 1, {std::byte{42}, std::byte{17}}, 0};
        section.payload_hash = SaveGameOrchestrator::HashBytes(section.payload);
        image.sections.push_back(section);
        image.manifest.participants.push_back(
            {id, 1, section.payload_hash,
             id == b ? SaveParticipantRequirement::Optional : SaveParticipantRequirement::Required,
             id == b ? std::vector<SaveParticipantId>{a} : std::vector<SaveParticipantId>{}});
    }
    return image;
}
void PatchInteger(std::vector<std::byte> &bytes, std::size_t offset, std::uint64_t value, std::size_t width) {
    for (std::size_t i = 0; i < width; ++i)
        bytes.at(offset + i) = std::byte((value >> (8 * i)) & 255);
}
void RepairChecksum(std::vector<std::byte> &bytes) {
    PatchInteger(bytes, bytes.size() - 8, SaveGameOrchestrator::HashBytes(std::span(bytes).first(bytes.size() - 8)), 8);
}
std::vector<std::byte> ReadFixture(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    WireCheck(bool(file), "wire historical fixture exists");
    std::vector<char> chars((std::istreambuf_iterator<char>(file)), {});
    std::vector<std::byte> bytes;
    for (auto c : chars)
        bytes.push_back(std::byte(static_cast<unsigned char>(c)));
    return bytes;
}
void WriteFixture(const std::filesystem::path &path, const std::vector<std::byte> &bytes) {
    WireCheck(!std::filesystem::exists(path), "historical fixture must never be overwritten");
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    WireCheck(bool(file), "record wire fixture");
}
}
void RecordSaveWireFixtures(const std::filesystem::path &root) {
    auto empty = EncodeSaveGameImage({});
    auto multi = EncodeSaveGameImage(WireImage());
    WireCheck(bool(empty) && bool(multi), "golden fixtures encode");
    WriteFixture(root / "whole/1/empty.bin", empty.Value());
    WriteFixture(root / "whole/1/multi.bin", multi.Value());
}
#if defined(EP_AUDITOR_SHADOW_PROBES) && EP_AUDITOR_SHADOW_PROBES
int RunAuditorSaveLoadScenario() {
    using namespace epidemic::auditor::shadow;
    std::string directory;
#ifdef _WIN32
    char *value = nullptr;
    size_t length = 0;
    if (_dupenv_s(&value, &length, "EP_AUDITOR_CAUSAL_DIRECTORY"))
        return 2;
    const std::unique_ptr<char, decltype(&std::free)> owned(value, &std::free);
    if (value)
        directory = value;
#else
    if (const auto *value = std::getenv("EP_AUDITOR_CAUSAL_DIRECTORY"))
        directory = value;
#endif
    if (directory.empty() || !std::filesystem::is_directory(directory) || std::filesystem::is_symlink(directory))
        return 2;
    const auto path = std::filesystem::path(directory) / "scenario-save.bin";
    if (std::filesystem::exists(path))
        return 3;
    action("action.start", "save-load-action", "Save and load wire package");
    scenario("scenario.start", "save-load-scenario");
    const auto image = WireImage();
    const auto encoded = EncodeSaveGameImage(image);
    if (!encoded)
        return 4;
    const auto write_start = std::chrono::steady_clock::now();
    WriteFixture(path, encoded.Value());
    io("io.write", "scenario-save", encoded.Value().size(),
       static_cast<std::uint64_t>(
           std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - write_start)
               .count()));
    const auto read_start = std::chrono::steady_clock::now();
    const auto bytes = ReadFixture(path);
    io("io.read", "scenario-load", bytes.size(),
       static_cast<std::uint64_t>(
           std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - read_start)
               .count()));
    const auto decoded = DecodeSaveGameImage(bytes);
    if (!decoded || decoded.Value().sections.size() != image.sections.size() || bytes != encoded.Value())
        return 5;
    scenario("scenario.end", "save-load-scenario");
    action("action.end", "save-load-action");
    return 0;
}
#endif
void TestSaveWireCodec() {
    auto empty = EncodeSaveGameImage({});
    WireCheck(bool(empty) && bool(DecodeSaveGameImage(empty.Value())), "empty image wire roundtrip");
    auto image = WireImage();
    auto encoded = EncodeSaveGameImage(image);
    WireCheck(bool(encoded), "multi-participant wire encode");
    auto decoded = DecodeSaveGameImage(encoded.Value());
    WireCheck(bool(decoded) && decoded.Value().sections.size() == 2 && decoded.Value().manifest.saved_at.ticks == -17,
              "multi-participant manifest and signed time roundtrip");
    auto canonical = EncodeSaveGameImage(decoded.Value());
    WireCheck(bool(canonical) && canonical.Value() == encoded.Value(), "canonical re-encoding is byte-stable");
    std::reverse(image.sections.begin(), image.sections.end());
    std::reverse(image.manifest.participants.begin(), image.manifest.participants.end());
    WireCheck(EncodeSaveGameImage(image).Value() == encoded.Value(), "participant and section ordering canonicalized");
    for (std::size_t size = 0; size < encoded.Value().size(); ++size)
        WireCheck(!DecodeSaveGameImage(std::span(encoded.Value()).first(size)),
                  "every truncated header/table/payload rejected");
    auto bad = encoded.Value();
    bad[0] ^= std::byte{1};
    RepairChecksum(bad);
    WireCheck(!DecodeSaveGameImage(bad), "corrupted magic rejected");
    bad = encoded.Value();
    PatchInteger(bad, 8, 2, 4);
    RepairChecksum(bad);
    WireCheck(!DecodeSaveGameImage(bad), "unsupported wire version rejected");
    bad = encoded.Value();
    PatchInteger(bad, 12, 2, 4);
    RepairChecksum(bad);
    WireCheck(!DecodeSaveGameImage(bad), "unsupported save format rejected");
    bad = encoded.Value();
    PatchInteger(bad, 16, UINT64_MAX, 8);
    RepairChecksum(bad);
    WireCheck(!DecodeSaveGameImage(bad), "invalid total byte size rejected");
    bad = encoded.Value();
    PatchInteger(bad, 64, UINT32_MAX, 4);
    RepairChecksum(bad);
    WireCheck(!DecodeSaveGameImage(bad), "resource count overflow rejected before allocation");
    bad = encoded.Value();
    bad[84] = std::byte{255};
    RepairChecksum(bad);
    WireCheck(!DecodeSaveGameImage(bad), "invalid requirement enum rejected");
    bad = encoded.Value();
    PatchInteger(bad, 85, UINT32_MAX, 4);
    RepairChecksum(bad);
    WireCheck(!DecodeSaveGameImage(bad), "malformed dependency count rejected");
    bad = encoded.Value();
    bad.back() ^= std::byte{1};
    WireCheck(!DecodeSaveGameImage(bad), "package checksum mismatch rejected");
    std::size_t sectionOffset = 72;
    for (const auto &entry : decoded.Value().manifest.participants)
        sectionOffset += 25 + 8 * entry.dependencies.size();
    bad = encoded.Value();
    bad.at(sectionOffset + 28) ^= std::byte{1};
    RepairChecksum(bad);
    WireCheck(!DecodeSaveGameImage(bad), "section payload corruption rejected even with repaired whole checksum");
    bad = encoded.Value();
    PatchInteger(bad, sectionOffset + 12, UINT64_MAX, 8);
    RepairChecksum(bad);
    WireCheck(!DecodeSaveGameImage(bad), "section length overflow rejected before allocation");
    bad = encoded.Value();
    std::copy_n(bad.begin() + static_cast<std::ptrdiff_t>(sectionOffset), 8,
                bad.begin() + static_cast<std::ptrdiff_t>(sectionOffset + 30));
    RepairChecksum(bad);
    WireCheck(!DecodeSaveGameImage(bad), "duplicate section identity on wire rejected");
    bad = encoded.Value();
    bad.push_back(std::byte{0});
    WireCheck(!DecodeSaveGameImage(bad), "trailing bytes rejected");
    image = WireImage();
    image.sections.push_back(image.sections.front());
    image.manifest.participants.push_back(image.manifest.participants.front());
    WireCheck(!EncodeSaveGameImage(image), "duplicate participant rejected");
    image = WireImage();
    image.sections[0].payload_hash ^= 1;
    WireCheck(!EncodeSaveGameImage(image), "section hash mismatch rejected");
    image = WireImage();
    image.manifest.participants[0].dependencies.push_back(SaveParticipantId::FromString("unknown"));
    WireCheck(!EncodeSaveGameImage(image), "unknown dependency rejected");
    image = WireImage();
    image.manifest.participants[1].dependencies = {image.manifest.participants[0].id};
    WireCheck(!EncodeSaveGameImage(image), "wire dependency cycle rejected");
    SaveWireLimits limits;
    limits.resources.max_participants = 1;
    WireCheck(!EncodeSaveGameImage(WireImage(), limits) && !DecodeSaveGameImage(encoded.Value(), limits),
              "participant limit enforced both ways");
    limits = {};
    limits.resources.max_section_bytes = 1;
    WireCheck(!EncodeSaveGameImage(WireImage(), limits) && !DecodeSaveGameImage(encoded.Value(), limits),
              "payload size limit enforced both ways");
    limits = {};
    limits.resources.max_section_bytes = 2;
    limits.resources.max_total_payload_bytes = 3;
    WireCheck(!EncodeSaveGameImage(WireImage(), limits) && !DecodeSaveGameImage(encoded.Value(), limits),
              "total payload size limit enforced both ways");
    limits = {};
    limits.max_wire_bytes = 80;
    WireCheck(!EncodeSaveGameImage(WireImage(), limits) && !DecodeSaveGameImage(encoded.Value(), limits),
              "whole-package byte limit enforced");
    limits = {};
    limits.resources.max_participants = 100000;
    WireCheck(!DecodeSaveGameImage(encoded.Value(), limits), "caller cannot bypass format hard caps");
    image = WireImage();
    auto extra = image.sections.front();
    extra.participant = SaveParticipantId::FromString("fixture.c");
    image.sections.push_back(extra);
    image.manifest.participants.push_back({extra.participant,
                                           1,
                                           extra.payload_hash,
                                           SaveParticipantRequirement::Optional,
                                           {image.sections[0].participant, image.sections[1].participant}});
    auto ordered = EncodeSaveGameImage(image);
    WireCheck(bool(ordered), "multiple semantic dependencies encode");
    std::reverse(image.manifest.participants.back().dependencies.begin(),
                 image.manifest.participants.back().dependencies.end());
    WireCheck(EncodeSaveGameImage(image).Value() == ordered.Value(), "dependency list ordering canonicalized");
    for (auto name : {"empty", "multi"}) {
        const auto bytes =
            ReadFixture(std::filesystem::path(EPIDEMIC_SAVE_FIXTURES) / "whole/1" / (std::string(name) + ".bin"));
        auto historical = DecodeSaveGameImage(bytes);
        WireCheck(bool(historical) && EncodeSaveGameImage(historical.Value()).Value() == bytes,
                  "historical wire golden decodes and canonically re-encodes");
    }
}
