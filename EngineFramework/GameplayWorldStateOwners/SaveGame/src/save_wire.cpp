#include "Epidemic/GameFramework/SaveGame/save_game.h"
#include "Epidemic/Foundation/error.h"
#include <algorithm>
#include <array>
#include <bit>
#include <functional>
#include <limits>
#include <stdexcept>
#include <string>

namespace epidemic::gameplay::savegame {
namespace {
constexpr std::string_view kMagic = "EPISAVE1";
constexpr std::size_t kHeaderBytes = 72;

void Need(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}

foundation::Error WireError(std::string_view phase, const std::exception &error) {
    return foundation::Error::Create("gameplay.save.wire", "save package operation failed",
                                     std::string(phase) + ": " + error.what());
}

void AddBounded(std::size_t &total, std::size_t bytes, std::size_t limit) {
    Need(bytes <= limit && total <= limit - bytes, "resource limit exceeded");
    total += bytes;
}

void CheckLimits(const SaveWireLimits &limits) {
    const auto &r = limits.resources;
    Need(limits.max_wire_bytes >= kHeaderBytes + 8 && r.max_participants > 0 &&
             r.max_dependencies_per_participant > 0 && r.max_section_bytes > 0 &&
             r.max_total_payload_bytes >= r.max_section_bytes,
         "invalid wire resource configuration");
    Need(limits.max_wire_bytes <= 600ull * 1024 * 1024 && r.max_participants <= 1024 &&
             r.max_dependencies_per_participant <= 256 &&
             r.max_section_bytes <= 64ull * 1024 * 1024 &&
             r.max_total_payload_bytes <= 512ull * 1024 * 1024,
         "wire resource configuration exceeds format hard caps");
}

class WireWriter {
  public:
    std::vector<std::byte> bytes;

    void Integer(std::uint64_t value, std::size_t width) {
        for (std::size_t i = 0; i < width; ++i)
            bytes.push_back(std::byte((value >> (8 * i)) & 255));
    }

    void Identity(SaveParticipantId id) {
        Integer(id.value.Raw(), 8);
    }

};

class WireReader {
  public:
    explicit WireReader(std::span<const std::byte> bytes) : bytes_(bytes) {
    }

    std::uint64_t Integer(std::size_t width) {
        Need(width <= Remaining(), "truncated wire field");
        std::uint64_t result = 0;
        for (std::size_t i = 0; i < width; ++i)
            result |= std::uint64_t(std::to_integer<unsigned char>(bytes_[position_++])) << (8 * i);
        return result;
    }

    SaveParticipantId Identity() {
        return {TypeId::FromRaw(Integer(8))};

    }

    std::span<const std::byte> Payload(std::size_t count) {
        Need(count <= Remaining(), "truncated payload");
        auto data = bytes_.subspan(position_, count);
        position_ += count;
        return data;
    }

    std::size_t Remaining() const noexcept {
        return bytes_.size() - position_;
    }

  private:
    std::span<const std::byte> bytes_;
    std::size_t position_ = 0;
};

void CheckStructure(const SaveGameImage &image, const SaveWireLimits &limits) {
    CheckLimits(limits);
    const auto &entries = image.manifest.participants;
    const auto &sections = image.sections;
    const auto &r = limits.resources;
    Need(image.manifest.save_version == 1, "unsupported save format version");
    Need(entries.size() <= r.max_participants && entries.size() <= UINT32_MAX &&
             entries.size() == sections.size(),
         "invalid participant count");
    std::vector<const SaveParticipantManifest *> ordered;
    for (const auto &entry : entries)
        ordered.push_back(&entry);
    std::sort(ordered.begin(), ordered.end(), [](auto a, auto b) { return a->id < b->id; });
    const auto indexOf = [&](SaveParticipantId id) {
        auto found = std::lower_bound(ordered.begin(), ordered.end(), id,
                                      [](auto e, auto value) { return e->id < value; });
        Need(found != ordered.end() && (*found)->id == id, "unknown dependency/section identity");
        return static_cast<std::size_t>(found - ordered.begin());
    };

    for (std::size_t i = 0; i < ordered.size(); ++i) {
        const auto &e = *ordered[i];
        Need(e.id.IsValid() && e.schema_version > 0 && (i == 0 || ordered[i - 1]->id != e.id),
             "invalid or duplicate participant");
        Need(e.requirement == SaveParticipantRequirement::Required ||
                 e.requirement == SaveParticipantRequirement::Optional,
             "invalid requirement enum");
        Need(e.dependencies.size() <= r.max_dependencies_per_participant &&
                 e.dependencies.size() <= UINT32_MAX,
             "dependency limit exceeded");
        auto deps = e.dependencies;
        std::sort(deps.begin(), deps.end());
        for (std::size_t d = 0; d < deps.size(); ++d) {
            Need(deps[d].IsValid() && deps[d] != e.id && (d == 0 || deps[d - 1] != deps[d]),
                 "malformed dependency list");
            (void)indexOf(deps[d]);
        }

    }

    std::vector<unsigned char> marks(ordered.size(), 0);
    std::function<void(std::size_t)> visit = [&](std::size_t i) {
        Need(marks[i] != 1, "dependency cycle");
        if (marks[i] == 2)
            return;
        marks[i] = 1;
        for (auto d : ordered[i]->dependencies)
            visit(indexOf(d));
        marks[i] = 2;
    };

    for (std::size_t i = 0; i < ordered.size(); ++i)
        visit(i);
    std::vector<bool> seen(ordered.size(), false);
    std::size_t payloadBytes = 0;
    for (const auto &s : sections) {
        const auto index = indexOf(s.participant);
        const auto &e = *ordered[index];
        Need(!seen[index], "duplicate section");
        seen[index] = true;
        Need(s.schema_version == e.schema_version && s.payload_hash == e.payload_hash,
             "manifest/section mismatch");
        Need(s.payload.size() <= r.max_section_bytes &&
                 SaveGameOrchestrator::HashBytes(s.payload) == s.payload_hash,
             "section hash/size mismatch");
        AddBounded(payloadBytes, s.payload.size(), r.max_total_payload_bytes);
    }

}

} // namespace

foundation::Result<std::vector<std::byte>> EncodeSaveGameImage(const SaveGameImage &image,
                                                               SaveWireLimits limits) {
    try {
        CheckStructure(image, limits);
        std::size_t wireBytes = kHeaderBytes + 8;
        for (const auto &e : image.manifest.participants) {
            AddBounded(wireBytes, 25, limits.max_wire_bytes);
            for (auto id : e.dependencies) {
                (void)id;
                AddBounded(wireBytes, 8, limits.max_wire_bytes);
            }

        }

        for (const auto &s : image.sections) {
            AddBounded(wireBytes, 28, limits.max_wire_bytes);
            AddBounded(wireBytes, s.payload.size(), limits.max_wire_bytes);
        }

        auto entries = image.manifest.participants;
        std::sort(entries.begin(), entries.end(),
                  [](const auto &a, const auto &b) { return a.id < b.id; });
        std::vector<const SaveSection *> sections;
        for (const auto &s : image.sections)
            sections.push_back(&s);
        std::sort(sections.begin(), sections.end(),
                  [](auto a, auto b) { return a->participant < b->participant; });
        WireWriter writer;
        writer.bytes.reserve(wireBytes);
        for (char c : kMagic)
            writer.Integer(static_cast<unsigned char>(c), 1);
        writer.Integer(1, 4);
        writer.Integer(image.manifest.save_version, 4);
        writer.Integer(wireBytes, 8);
        writer.Integer(image.manifest.content_version, 8);
        writer.Integer(image.manifest.content_hash, 8);
        writer.Integer(image.manifest.engine_version, 8);
        writer.Integer(image.manifest.framework_version, 8);
        writer.Integer(std::bit_cast<std::uint64_t>(image.manifest.saved_at.ticks), 8);
        writer.Integer(entries.size(), 4);
        writer.Integer(sections.size(), 4);
        for (auto &e : entries) {
            std::sort(e.dependencies.begin(), e.dependencies.end());
            writer.Identity(e.id);
            writer.Integer(e.schema_version, 4);
            writer.Integer(e.requirement == SaveParticipantRequirement::Required ? 1 : 2, 1);
            writer.Integer(e.dependencies.size(), 4);
            writer.Integer(e.payload_hash, 8);
            for (auto d : e.dependencies)
                writer.Identity(d);
        }

        for (auto s : sections) {
            writer.Identity(s->participant);
            writer.Integer(s->schema_version, 4);
            writer.Integer(s->payload.size(), 8);
            writer.Integer(s->payload_hash, 8);
            writer.bytes.insert(writer.bytes.end(), s->payload.begin(), s->payload.end());
        }

        writer.Integer(SaveGameOrchestrator::HashBytes(writer.bytes), 8);
        Need(writer.bytes.size() == wireBytes, "internal wire size mismatch");
        return foundation::Result<std::vector<std::byte>>::Success(std::move(writer.bytes));
    } catch (const std::exception &error) {
        return foundation::Result<std::vector<std::byte>>::Failure(WireError("encode", error));
    }

}

foundation::Result<SaveGameImage> DecodeSaveGameImage(std::span<const std::byte> bytes,
                                                      SaveWireLimits limits) {
    try {
        CheckLimits(limits);
        Need(bytes.size() >= kHeaderBytes + 8 && bytes.size() <= limits.max_wire_bytes,
             "truncated/oversized package");
        WireReader checksum(bytes.last(8));
        Need(checksum.Integer(8) == SaveGameOrchestrator::HashBytes(bytes.first(bytes.size() - 8)),
             "package checksum mismatch");
        WireReader reader(bytes.first(bytes.size() - 8));
        for (char c : kMagic)
            Need(reader.Integer(1) == static_cast<unsigned char>(c), "invalid wire magic");
        Need(reader.Integer(4) == 1, "unsupported wire version");
        SaveGameImage image;
        image.manifest.save_version = static_cast<std::uint32_t>(reader.Integer(4));
        Need(image.manifest.save_version == 1, "unsupported save format version");
        Need(reader.Integer(8) == bytes.size(), "wire size mismatch/trailing bytes");
        image.manifest.content_version = reader.Integer(8);
        image.manifest.content_hash = reader.Integer(8);
        image.manifest.engine_version = reader.Integer(8);
        image.manifest.framework_version = reader.Integer(8);
        image.manifest.saved_at.ticks = std::bit_cast<std::int64_t>(reader.Integer(8));
        const auto count = reader.Integer(4);
        const auto sectionCount = reader.Integer(4);
        Need(count == sectionCount && count <= limits.resources.max_participants &&
                 count <= reader.Remaining() / 25,
             "invalid participant table size");
        image.manifest.participants.reserve(static_cast<std::size_t>(count));
        image.sections.reserve(static_cast<std::size_t>(count));
        SaveParticipantId previous{};

        for (std::uint64_t i = 0; i < count; ++i) {
            SaveParticipantManifest e;
            e.id = reader.Identity();
            Need(e.id.IsValid() && (i == 0 || previous < e.id),
                 "noncanonical/duplicate participant identity");
            previous = e.id;
            e.schema_version = static_cast<std::uint32_t>(reader.Integer(4));
            const auto requirement = reader.Integer(1);
            Need(requirement == 1 || requirement == 2, "invalid requirement enum");
            e.requirement = requirement == 1 ? SaveParticipantRequirement::Required
                                             : SaveParticipantRequirement::Optional;
            const auto deps = reader.Integer(4);
            e.payload_hash = reader.Integer(8);
            Need(deps <= limits.resources.max_dependencies_per_participant &&
                     deps <= reader.Remaining() / 8,
                 "invalid dependency table size");
            e.dependencies.reserve(static_cast<std::size_t>(deps));
            SaveParticipantId last{};

            for (std::uint64_t d = 0; d < deps; ++d) {
                auto id = reader.Identity();
                Need(id.IsValid() && (d == 0 || last < id), "noncanonical/duplicate dependency");
                last = id;
                e.dependencies.push_back(id);
            }

            image.manifest.participants.push_back(std::move(e));
        }

        std::size_t totalPayload = 0;
        previous = {};

        for (std::uint64_t i = 0; i < count; ++i) {
            SaveSection s;
            s.participant = reader.Identity();
            Need(s.participant.IsValid() && (i == 0 || previous < s.participant),
                 "noncanonical/duplicate section identity");
            previous = s.participant;
            s.schema_version = static_cast<std::uint32_t>(reader.Integer(4));
            const auto size = reader.Integer(8);
            s.payload_hash = reader.Integer(8);
            Need(size <= limits.resources.max_section_bytes && size <= reader.Remaining(),
                 "truncated/oversized section");
            AddBounded(totalPayload, static_cast<std::size_t>(size),
                       limits.resources.max_total_payload_bytes);
            auto payload = reader.Payload(static_cast<std::size_t>(size));
            s.payload.assign(payload.begin(), payload.end());
            image.sections.push_back(std::move(s));
        }

        Need(reader.Remaining() == 0, "trailing bytes");
        CheckStructure(image, limits);
        return foundation::Result<SaveGameImage>::Success(std::move(image));
    } catch (const std::exception &error) {
        return foundation::Result<SaveGameImage>::Failure(WireError("decode", error));
    }

}

foundation::Result<void> CommitSaveGameImage(ISavePackageStore &store, const SaveGameImage &image,
                                             SaveWireLimits limits) {
    auto encoded = EncodeSaveGameImage(image, limits);
    if (!encoded)
        return foundation::Result<void>::Failure(encoded.GetError());
    try {
        return store.Commit(encoded.Value());
    } catch (const std::exception &error) {
        return foundation::Result<void>::Failure(WireError("store.commit", error));
    } catch (...) {
        return foundation::Result<void>::Failure(
            foundation::Error::Create("gameplay.save.store_exception",
                                      "unknown store commit exception"));
    }

}

foundation::Result<SaveGameImage> LoadSaveGameImage(ISavePackageStore &store,
                                                    SaveWireLimits limits) {
    try {
        auto bytes = store.Load();
        if (!bytes)
            return foundation::Result<SaveGameImage>::Failure(bytes.GetError());
        return DecodeSaveGameImage(bytes.Value(), limits);
    } catch (const std::exception &error) {
        return foundation::Result<SaveGameImage>::Failure(WireError("store.load", error));
    } catch (...) {
        return foundation::Result<SaveGameImage>::Failure(
            foundation::Error::Create("gameplay.save.store_exception",
                                      "unknown store load exception"));
    }

}

} // namespace epidemic::gameplay::savegame
