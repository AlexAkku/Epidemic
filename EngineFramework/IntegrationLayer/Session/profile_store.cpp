#include <Epidemic/GameFramework/Session/profile_store.h>
#include <Epidemic/GameFramework/SaveGame/save_game.h>
#include "blob_store.h"
#include <thread>
#include <stdexcept>
namespace epidemic::gameplay::session {
namespace {
namespace sg = savegame;
template <class T> T Take(foundation::Result<T> result) {
    if (!result.HasValue())
        throw std::runtime_error(result.GetError().message);
    return std::move(result).Value();
}
void Check(foundation::Result<void> result) {
    if (!result.HasValue())
        throw std::runtime_error(result.GetError().message);
}
struct Barrier final : sg::ISaveBarrier {
    struct Lease final : sg::ISaveBarrierLease {
        bool &held;
        explicit Lease(bool &value) : held(value) {
            held = true;
        }
        ~Lease() override {
            held = false;
        }
    };
    bool held = false;
    std::thread::id owner = std::this_thread::get_id();
    foundation::Result<std::unique_ptr<sg::ISaveBarrierLease>> AcquireCaptureLease() override {
        if (held || owner != std::this_thread::get_id())
            return foundation::Result<std::unique_ptr<sg::ISaveBarrierLease>>::Failure(
                foundation::Error::Create("session.save_barrier", "Nested save or wrong thread"));
        return foundation::Result<std::unique_ptr<sg::ISaveBarrierLease>>::Success(std::make_unique<Lease>(held));
    }
    foundation::Result<std::unique_ptr<sg::ISaveBarrierLease>> AcquireRestoreLease() override {
        return AcquireCaptureLease();
    }
};
struct Payload final : sg::ISaveParticipant {
    struct Stage final : sg::IRestoreStage {
        std::vector<std::byte> bytes;
    };
    std::vector<std::byte> bytes;
    sg::SaveParticipantId Id() const noexcept override {
        return sg::SaveParticipantId::FromString("session.profile");
    }
    sg::SaveSchemaVersion SchemaVersion() const noexcept override {
        return 1;
    }
    std::vector<sg::SaveParticipantId> Dependencies() const override {
        return {};
    }
    foundation::Result<sg::SaveSection> CaptureSnapshot(const sg::SaveContext &) const override {
        return foundation::Result<sg::SaveSection>::Success(
            {Id(), 1, bytes, sg::SaveGameOrchestrator::HashBytes(bytes)});
    }
    foundation::Result<void> ValidateSnapshot(const sg::SaveSection &section,
                                              const sg::RestoreContext &) const override {
        if (section.participant != Id() || section.schema_version != 1 || section.payload.empty() ||
            section.payload.size() > 32768)
            return foundation::Result<void>::Failure(
                foundation::Error::Create("session.profile_schema", "Invalid profile section"));
        return foundation::Result<void>::Success();
    }
    foundation::Result<std::unique_ptr<sg::IRestoreStage>> StageRestore(const sg::SaveSection &section,
                                                                        const sg::RestoreContext &context) override {
        const auto valid = ValidateSnapshot(section, context);
        if (!valid.HasValue())
            return foundation::Result<std::unique_ptr<sg::IRestoreStage>>::Failure(valid.GetError());
        auto stage = std::make_unique<Stage>();
        stage->bytes = section.payload;
        return foundation::Result<std::unique_ptr<sg::IRestoreStage>>::Success(std::move(stage));
    }
    void CommitRestore(sg::IRestoreStage &stage) noexcept override {
        bytes.swap(static_cast<Stage &>(stage).bytes);
    }
};
struct Store final : sg::ISavePackageStore {
    std::filesystem::path path;
    explicit Store(std::filesystem::path file) : path(std::move(file)) {
    }
    foundation::Result<std::vector<std::byte>> Load() override {
        try {
            return foundation::Result<std::vector<std::byte>>::Success(runtime::presentation::LoadBlob(path));
        } catch (const std::exception &e) {
            return foundation::Result<std::vector<std::byte>>::Failure(
                foundation::Error::Create("session.save_load", e.what()));
        }
    }
    foundation::Result<void> Commit(std::span<const std::byte> package) override {
        try {
            runtime::presentation::SaveBlob(path, package);
            return foundation::Result<void>::Success();
        } catch (const std::exception &e) {
            return foundation::Result<void>::Failure(foundation::Error::Create("session.save_commit", e.what()));
        }
    }
};
void Register(sg::SaveGameOrchestrator &saves, Barrier &barrier, Payload &payload) {
    Check(saves.SetBarrier(barrier));
    Check(saves.RegisterParticipant(payload));
    Check(saves.FreezeRegistry());
}
}
std::string LoadProfilePayload(const std::filesystem::path &path) {
    Barrier barrier;
    Payload payload;
    sg::SaveGameOrchestrator saves;
    Store store(path);
    Register(saves, barrier, payload);
    auto image = Take(sg::LoadSaveGameImage(store));
    Check(saves.Restore(std::move(image),
                        {.current_content_version = 1, .current_engine_version = 1, .current_framework_version = 1}));
    return {reinterpret_cast<const char *>(payload.bytes.data()), payload.bytes.size()};
}
void SaveProfilePayload(const std::filesystem::path &path, std::string_view text) {
    if (text.empty() || text.size() > 32768)
        throw std::invalid_argument("Invalid profile payload size");
    Barrier barrier;
    Payload payload;
    sg::SaveGameOrchestrator saves;
    Store store(path);
    const auto bytes = std::as_bytes(std::span{text.data(), text.size()});
    payload.bytes.assign(bytes.begin(), bytes.end());
    Register(saves, barrier, payload);
    auto image = Take(saves.Capture({.content_version = 1, .engine_version = 1, .framework_version = 1}));
    Check(sg::CommitSaveGameImage(store, image));
}
}
