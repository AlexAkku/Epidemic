#include "Epidemic/GameFramework/Integration/core_adapters.h"
#include "Epidemic/Platform/iplatform_runtime.h"

#include <algorithm>
#include <array>
#include <limits>
#include <mutex>
#include <new>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

namespace epidemic::gameplay::integration {
namespace detail {
thread_local std::string_view save_generation_fault_point;

void SetSaveGenerationFaultPointForTesting(std::string_view point) noexcept {
    save_generation_fault_point = point;
}

void ClearSaveGenerationFaultPointForTesting() noexcept {
    save_generation_fault_point = {};

}

} // namespace detail

namespace {
void GenerationFaultPoint(std::string_view point) {
    if (detail::save_generation_fault_point == point) {
        detail::save_generation_fault_point = {};

        throw std::bad_alloc{};

    }

}

template <typename F> struct ScopeExit {
    F action;

    ~ScopeExit() noexcept {
        action();
    }

};

constexpr std::array<std::byte, 8> kGenerationMagic{std::byte{'E'}, std::byte{'P'}, std::byte{'I'},
                                                    std::byte{'G'}, std::byte{'E'}, std::byte{'N'},
                                                    std::byte{'0'}, std::byte{'1'}};

constexpr std::uint32_t kGenerationWireVersion = 1;
constexpr std::uint32_t kGenerationPlaneCount = 3;

enum class GenerationPlane : std::uint32_t {
    FrameworkSaveGame = 1,
    RuntimeTime = 2,
    RuntimePersistence = 3,
};

foundation::Error GenerationError(std::string_view code, std::string_view message,
                                  std::string_view context = {}) {
    return foundation::Error::Create(code, message, context);
}

bool LimitsValid(const SaveGenerationLimits &limits) noexcept {
    return limits.max_framework_savegame_bytes != 0 && limits.max_runtime_time_bytes != 0 &&
           limits.max_runtime_persistence_bytes != 0 && limits.max_wire_bytes != 0;
}

class WireWriter {
  public:
    explicit WireWriter(std::size_t maximum) : maximum_(maximum) {
    }

    void Magic() {
        Append(kGenerationMagic);
    }

    void U32(std::uint32_t value) {
        for (std::size_t index = 0; index < 4; ++index)
            Byte(std::byte((value >> (index * 8)) & 0xffu));
    }

    void U64(std::uint64_t value) {
        for (std::size_t index = 0; index < 8; ++index)
            Byte(std::byte((value >> (index * 8)) & 0xffu));
    }

    void Payload(std::span<const std::byte> payload) {
        Append(payload);
    }

    [[nodiscard]] std::span<const std::byte> View() const noexcept {
        return bytes_;
    }

    [[nodiscard]] std::vector<std::byte> Finish() && {
        return std::move(bytes_);
    }

  private:
    void Byte(std::byte value) {
        if (bytes_.size() >= maximum_)
            throw std::length_error("save generation package size limit exceeded");
        bytes_.push_back(value);
    }

    template <typename Range> void Append(const Range &range) {
        const auto count = std::size(range);
        if (count > maximum_ - bytes_.size())
            throw std::length_error("save generation package size limit exceeded");
        bytes_.insert(bytes_.end(), std::begin(range), std::end(range));
    }

    std::size_t maximum_ = 0;
    std::vector<std::byte> bytes_;
};

class WireReader {
  public:
    explicit WireReader(std::span<const std::byte> bytes) : bytes_(bytes) {
    }

    bool Magic() {
        if (Remaining() < kGenerationMagic.size())
            return false;
        if (!std::equal(kGenerationMagic.begin(), kGenerationMagic.end(),
                        bytes_.begin() + static_cast<std::ptrdiff_t>(position_)))
            return false;
        position_ += kGenerationMagic.size();
        return true;
    }

    bool U32(std::uint32_t &value) {
        if (Remaining() < 4)
            return false;
        value = 0;
        for (std::size_t index = 0; index < 4; ++index)
            value |= static_cast<std::uint32_t>(std::to_integer<unsigned char>(bytes_[position_++]))
                     << (index * 8);
        return true;
    }

    bool U64(std::uint64_t &value) {
        if (Remaining() < 8)
            return false;
        value = 0;
        for (std::size_t index = 0; index < 8; ++index)
            value |= static_cast<std::uint64_t>(std::to_integer<unsigned char>(bytes_[position_++]))
                     << (index * 8);
        return true;
    }

    bool Payload(std::uint64_t size, std::vector<std::byte> &output) {
        if (size > std::numeric_limits<std::size_t>::max())
            return false;
        const auto count = static_cast<std::size_t>(size);
        if (count > Remaining())
            return false;
        const auto first = bytes_.begin() + static_cast<std::ptrdiff_t>(position_);
        output.assign(first, first + static_cast<std::ptrdiff_t>(count));
        position_ += count;
        return true;
    }

    [[nodiscard]] std::size_t Position() const noexcept {
        return position_;
    }

    [[nodiscard]] std::size_t Remaining() const noexcept {
        return bytes_.size() - position_;
    }

  private:
    std::span<const std::byte> bytes_;
    std::size_t position_ = 0;
};

foundation::Result<void> ValidatePackageFields(const SaveGenerationPackage &package,
                                               const SaveGenerationLimits &limits,
                                               savegame::SaveWireLimits framework_limits) {
    if (!LimitsValid(limits))
        return foundation::Result<void>::Failure(
            GenerationError("gameplay.save_generation.invalid_limits",
                            "invalid save generation resource limits"));
    if (!package.generation.IsValid())
        return foundation::Result<void>::Failure(
            GenerationError("gameplay.save_generation.invalid_id",
                            "save generation zero is invalid"));
    if (package.framework_savegame.empty() ||
        package.framework_savegame.size() > limits.max_framework_savegame_bytes)
        return foundation::Result<void>::Failure(
            GenerationError("gameplay.save_generation.framework_limit",
                            "Framework save payload is empty or exceeds its limit"));
    if (package.runtime_time.empty() || package.runtime_time.size() > limits.max_runtime_time_bytes)
        return foundation::Result<void>::Failure(
            GenerationError("gameplay.save_generation.runtime_time_limit",
                            "Runtime Time payload is empty or exceeds its limit"));
    if (package.runtime_persistence.empty() ||
        package.runtime_persistence.size() > limits.max_runtime_persistence_bytes)
        return foundation::Result<void>::Failure(
            GenerationError("gameplay.save_generation.runtime_persistence_limit",
                            "Runtime Persistence payload is empty or exceeds its limit"));
    auto framework = savegame::DecodeSaveGameImage(package.framework_savegame, framework_limits);
    if (!framework)
        return foundation::Result<void>::Failure(framework.GetError());
    return foundation::Result<void>::Success();
}

void WritePlane(WireWriter &writer, GenerationPlane plane, SaveGenerationId generation,
                std::span<const std::byte> payload) {
    writer.U32(static_cast<std::uint32_t>(plane));
    writer.U64(generation.Raw());
    writer.U64(static_cast<std::uint64_t>(payload.size()));
    writer.U64(savegame::SaveGameOrchestrator::HashBytes(payload));
    writer.Payload(payload);
}

foundation::Result<void> ReadPlane(WireReader &reader, GenerationPlane expected_plane,
                                   SaveGenerationId generation, std::size_t maximum,
                                   std::vector<std::byte> &output) {
    std::uint32_t raw_plane = 0;
    std::uint64_t raw_generation = 0;
    std::uint64_t raw_size = 0;
    std::uint64_t expected_hash = 0;
    if (!reader.U32(raw_plane) || !reader.U64(raw_generation) || !reader.U64(raw_size) ||
        !reader.U64(expected_hash))
        return foundation::Result<void>::Failure(
            GenerationError("gameplay.save_generation.truncated",
                            "truncated save generation plane descriptor"));
    if (raw_plane != static_cast<std::uint32_t>(expected_plane))
        return foundation::Result<void>::Failure(
            GenerationError("gameplay.save_generation.plane_order",
                            "save generation planes are missing, duplicated, or out of canonical "
                            "order"));
    if (raw_generation != generation.Raw())
        return foundation::Result<void>::Failure(
            GenerationError("gameplay.save_generation.generation_mismatch",
                            "save generation plane is bound to a different generation"));
    if (raw_size == 0 || raw_size > maximum)
        return foundation::Result<void>::Failure(
            GenerationError("gameplay.save_generation.plane_limit",
                            "save generation plane is empty or exceeds its resource limit"));
    if (!reader.Payload(raw_size, output))
        return foundation::Result<void>::Failure(
            GenerationError("gameplay.save_generation.truncated",
                            "truncated save generation plane payload"));
    if (savegame::SaveGameOrchestrator::HashBytes(output) != expected_hash)
        return foundation::Result<void>::Failure(
            GenerationError("gameplay.save_generation.plane_hash",
                            "save generation plane checksum mismatch"));
    return foundation::Result<void>::Success();
}

class CallbackBarrierLease final : public savegame::ISaveBarrierLease {
  public:
    explicit CallbackBarrierLease(std::function<void()> release) : release_(std::move(release)) {
    }

    ~CallbackBarrierLease() override {
        if (release_)
            release_();
    }

  private:
    std::function<void()> release_;
};

foundation::Result<std::unique_ptr<savegame::ISaveBarrierLease>>
BarrierFailure(std::string_view code, std::string_view message) {
    return foundation::Result<std::unique_ptr<savegame::ISaveBarrierLease>>::Failure(
        GenerationError(code, message));
}

} // namespace

foundation::Result<std::vector<std::byte>>
EncodeSaveGenerationPackage(const SaveGenerationPackage &package, SaveGenerationLimits limits) {
    try {
        savegame::SaveWireLimits framework_limits{};

        framework_limits.max_wire_bytes = limits.max_framework_savegame_bytes;
        auto valid = ValidatePackageFields(package, limits, framework_limits);
        if (!valid)
            return foundation::Result<std::vector<std::byte>>::Failure(valid.GetError());

        WireWriter writer(limits.max_wire_bytes);
        writer.Magic();
        writer.U32(kGenerationWireVersion);
        writer.U64(package.generation.Raw());
        writer.U32(kGenerationPlaneCount);
        WritePlane(writer, GenerationPlane::FrameworkSaveGame, package.generation,
                   package.framework_savegame);
        WritePlane(writer, GenerationPlane::RuntimeTime, package.generation, package.runtime_time);
        WritePlane(writer, GenerationPlane::RuntimePersistence, package.generation,
                   package.runtime_persistence);
        const auto checksum = savegame::SaveGameOrchestrator::HashBytes(writer.View());
        writer.U64(checksum);
        return foundation::Result<std::vector<std::byte>>::Success(std::move(writer).Finish());
    } catch (const std::exception &error) {
        return foundation::Result<std::vector<std::byte>>::Failure(
            GenerationError("gameplay.save_generation.encode",
                            "failed to encode save generation package", error.what()));
    } catch (...) {
        return foundation::Result<std::vector<std::byte>>::Failure(
            GenerationError("gameplay.save_generation.encode",
                            "failed to encode save generation package"));
    }

}

foundation::Result<SaveGenerationPackage>
DecodeSaveGenerationPackage(std::span<const std::byte> bytes, SaveGenerationLimits limits) {
    try {
        if (!LimitsValid(limits) || bytes.size() > limits.max_wire_bytes || bytes.size() < 64)
            return foundation::Result<SaveGenerationPackage>::Failure(
                GenerationError("gameplay.save_generation.invalid_wire",
                                "save generation package is truncated, oversized, or limits are "
                                "invalid"));

        const auto payload_bytes = bytes.first(bytes.size() - 8);
        WireReader checksum_reader(bytes.last(8));
        std::uint64_t expected_checksum = 0;
        if (!checksum_reader.U64(expected_checksum) ||
            savegame::SaveGameOrchestrator::HashBytes(payload_bytes) != expected_checksum)
            return foundation::Result<SaveGenerationPackage>::Failure(
                GenerationError("gameplay.save_generation.package_hash",
                                "save generation package checksum mismatch"));

        WireReader reader(payload_bytes);
        std::uint32_t version = 0;
        std::uint64_t raw_generation = 0;
        std::uint32_t plane_count = 0;
        if (!reader.Magic() || !reader.U32(version) || !reader.U64(raw_generation) ||
            !reader.U32(plane_count))
            return foundation::Result<SaveGenerationPackage>::Failure(
                GenerationError("gameplay.save_generation.invalid_wire",
                                "invalid save generation package header"));
        if (version != kGenerationWireVersion)
            return foundation::Result<SaveGenerationPackage>::Failure(
                GenerationError("gameplay.save_generation.version",
                                "unsupported save generation package version"));
        const auto generation = SaveGenerationId::FromRaw(raw_generation);
        if (!generation.IsValid())
            return foundation::Result<SaveGenerationPackage>::Failure(
                GenerationError("gameplay.save_generation.invalid_id",
                                "save generation zero is invalid"));
        if (plane_count != kGenerationPlaneCount)
            return foundation::Result<SaveGenerationPackage>::Failure(
                GenerationError("gameplay.save_generation.plane_count",
                                "save generation package must contain exactly three save planes"));

        SaveGenerationPackage package{};

        package.generation = generation;
        auto result = ReadPlane(reader, GenerationPlane::FrameworkSaveGame, generation,
                                limits.max_framework_savegame_bytes, package.framework_savegame);
        if (!result)
            return foundation::Result<SaveGenerationPackage>::Failure(result.GetError());
        result = ReadPlane(reader, GenerationPlane::RuntimeTime, generation,
                           limits.max_runtime_time_bytes, package.runtime_time);
        if (!result)
            return foundation::Result<SaveGenerationPackage>::Failure(result.GetError());
        result = ReadPlane(reader, GenerationPlane::RuntimePersistence, generation,
                           limits.max_runtime_persistence_bytes, package.runtime_persistence);
        if (!result)
            return foundation::Result<SaveGenerationPackage>::Failure(result.GetError());
        if (reader.Remaining() != 0)
            return foundation::Result<SaveGenerationPackage>::Failure(
                GenerationError("gameplay.save_generation.trailing_bytes",
                                "save generation package contains trailing bytes"));

        savegame::SaveWireLimits framework_limits{};

        framework_limits.max_wire_bytes = limits.max_framework_savegame_bytes;
        auto valid = ValidatePackageFields(package, limits, framework_limits);
        if (!valid)
            return foundation::Result<SaveGenerationPackage>::Failure(valid.GetError());
        return foundation::Result<SaveGenerationPackage>::Success(std::move(package));
    } catch (const std::bad_alloc &) {
        return foundation::Result<SaveGenerationPackage>::Failure(
            GenerationError("gameplay.save_generation.allocation_failed",
                            "failed to allocate while decoding save generation package"));
    } catch (const std::exception &error) {
        return foundation::Result<SaveGenerationPackage>::Failure(
            GenerationError("gameplay.save_generation.decode",
                            "failed to decode save generation package", error.what()));
    } catch (...) {
        return foundation::Result<SaveGenerationPackage>::Failure(
            GenerationError("gameplay.save_generation.decode",
                            "failed to decode save generation package"));
    }

}

foundation::Result<RuntimeSaveGenerationCapture>
CallbackRuntimeSaveGenerationPort::Capture(SaveGenerationId generation) {
    if (!generation.IsValid())
        return foundation::Result<RuntimeSaveGenerationCapture>::Failure(
            GenerationError("gameplay.save_generation.invalid_id",
                            "Runtime capture requires a valid save generation"));
    if (!time_capture_ || !persistence_capture_)
        return foundation::Result<RuntimeSaveGenerationCapture>::Failure(
            GenerationError("gameplay.save_generation.runtime_port_unconfigured",
                            "Runtime save generation port is not fully configured"));
    try {
        auto time = time_capture_(generation);
        if (!time)
            return foundation::Result<RuntimeSaveGenerationCapture>::Failure(time.GetError());
        auto persistence = persistence_capture_(generation);
        if (!persistence)
            return foundation::Result<RuntimeSaveGenerationCapture>::Failure(
                persistence.GetError());
        RuntimeSaveGenerationCapture capture{};

        capture.time_package = std::move(time.Value());
        capture.persistence_package = std::move(persistence.Value());
        return foundation::Result<RuntimeSaveGenerationCapture>::Success(std::move(capture));
    } catch (const std::exception &error) {
        return foundation::Result<RuntimeSaveGenerationCapture>::Failure(
            GenerationError("gameplay.save_generation.runtime_port_exception",
                            "Runtime save generation capture threw", error.what()));
    } catch (...) {
        return foundation::Result<RuntimeSaveGenerationCapture>::Failure(
            GenerationError("gameplay.save_generation.runtime_port_exception",
                            "Runtime save generation capture threw"));
    }

}

foundation::Result<std::optional<std::vector<std::byte>>> FileSaveGenerationStore::LoadCurrent() {
    try {
        if (path_.empty())
            return foundation::Result<std::optional<std::vector<std::byte>>>::Failure(
                GenerationError("gameplay.save_generation.empty_path",
                                "save generation path is empty"));
        std::error_code error;
        const bool exists = std::filesystem::exists(path_, error);
        if (error)
            return foundation::Result<std::optional<std::vector<std::byte>>>::Failure(
                GenerationError("gameplay.save_generation.stat_failed",
                                "failed to inspect current save generation", error.message()));
        if (!exists)
            return foundation::Result<std::optional<std::vector<std::byte>>>::Success(std::nullopt);
        auto bytes = platform::ReadFilePackage(foundation::Path(path_), limits_.max_wire_bytes);
        if (!bytes)
            return foundation::Result<std::optional<std::vector<std::byte>>>::Failure(
                bytes.GetError());
        auto validated = DecodeSaveGenerationPackage(bytes.Value(), limits_);
        if (!validated)
            return foundation::Result<std::optional<std::vector<std::byte>>>::Failure(
                validated.GetError());
        return foundation::Result<std::optional<std::vector<std::byte>>>::Success(
            std::move(bytes.Value()));
    } catch (const std::exception &error) {
        return foundation::Result<std::optional<std::vector<std::byte>>>::Failure(
            GenerationError("gameplay.save_generation.load_exception",
                            "failed to load current save generation", error.what()));
    }

}

foundation::Result<void>
FileSaveGenerationStore::CommitCurrent(std::span<const std::byte> package) {
    auto validated = DecodeSaveGenerationPackage(package, limits_);
    if (!validated)
        return foundation::Result<void>::Failure(validated.GetError());
    return platform::CommitFilePackage(foundation::Path(path_), package, limits_.max_wire_bytes);
}

struct SaveGenerationBarrier::State {
    explicit State(SaveGenerationBarrierCallbacks value) : callbacks(std::move(value)) {
    }

    mutable std::mutex mutex;
    SaveGenerationBarrierCallbacks callbacks;
    bool acquiring = false;
    bool capture_active = false;
    bool outer_alive = false;
    bool restore_active = false;
    std::thread::id owner_thread{};

    SaveGenerationId generation{};

    std::size_t nested_capture_leases = 0;
    std::size_t restore_leases = 0;
    std::unique_ptr<savegame::ISaveBarrierLease> capture_quiescence;
    std::unique_ptr<savegame::ISaveBarrierLease> restore_quiescence;
};

SaveGenerationBarrier::SaveGenerationBarrier(SaveGenerationBarrierCallbacks callbacks)
    : state_(std::make_shared<State>(std::move(callbacks))) {
}

SaveGenerationBarrier::~SaveGenerationBarrier() = default;

foundation::Result<std::unique_ptr<savegame::ISaveBarrierLease>>
SaveGenerationBarrier::AcquireGenerationLease(SaveGenerationId generation) try {
    if (!generation.IsValid())
        return BarrierFailure("gameplay.save_generation.invalid_id",
                              "outer capture requires a valid save generation");

    const auto state = state_;
    {
        std::lock_guard lock(state->mutex);
        if (state->acquiring || state->capture_active || state->restore_active)
            return BarrierFailure("gameplay.save_generation.barrier_busy",
                                  "save generation barrier is already active");
        if (!state->callbacks.acquire_capture_quiescence || !state->callbacks.capture_preflight)
            return BarrierFailure("gameplay.save_generation.barrier_unconfigured",
                                  "capture quiescence/preflight callbacks are required");
        state->acquiring = true;
    }

    ScopeExit acquisition_cleanup{[state]() noexcept {
        std::lock_guard lock(state->mutex);
        state->acquiring = false;
    }};

    foundation::Result<std::unique_ptr<savegame::ISaveBarrierLease>> quiescence =
        BarrierFailure("gameplay.save_generation.barrier_callback",
                       "capture quiescence callback did not run");
    try {
        quiescence = state->callbacks.acquire_capture_quiescence();
    } catch (const std::exception &error) {
        std::lock_guard lock(state->mutex);
        state->acquiring = false;
        return BarrierFailure("gameplay.save_generation.barrier_exception", error.what());
    } catch (...) {
        std::lock_guard lock(state->mutex);
        state->acquiring = false;
        return BarrierFailure("gameplay.save_generation.barrier_exception",
                              "capture quiescence callback threw");
    }

    if (!quiescence || !quiescence.Value()) {
        std::lock_guard lock(state->mutex);
        state->acquiring = false;
        return quiescence
                   ? BarrierFailure("gameplay.save_generation.barrier_invalid",
                                    "capture quiescence returned an empty lease")
                   : foundation::Result<std::unique_ptr<savegame::ISaveBarrierLease>>::Failure(
                         quiescence.GetError());
    }

    foundation::Result<void> preflight = foundation::Result<void>::Success();
    try {
        preflight = state->callbacks.capture_preflight();
    } catch (const std::exception &error) {
        preflight = foundation::Result<void>::Failure(
            GenerationError("gameplay.save_generation.preflight_exception",
                            "save generation preflight threw", error.what()));
    } catch (...) {
        preflight = foundation::Result<void>::Failure(
            GenerationError("gameplay.save_generation.preflight_exception",
                            "save generation preflight threw"));
    }

    if (!preflight) {
        std::lock_guard lock(state->mutex);
        state->acquiring = false;
        return foundation::Result<std::unique_ptr<savegame::ISaveBarrierLease>>::Failure(
            preflight.GetError());
    }

    auto release = [state]() noexcept {
        std::unique_ptr<savegame::ISaveBarrierLease> quiescence_to_release;
        {
            std::lock_guard lock(state->mutex);
            if (!state->capture_active || !state->outer_alive)
                return;
            state->outer_alive = false;
            if (state->nested_capture_leases == 0) {
                state->capture_active = false;
                state->generation = {};

                state->owner_thread = {};

                quiescence_to_release = std::move(state->capture_quiescence);
            }

        }

        quiescence_to_release.reset();
    };

    GenerationFaultPoint("lease.outer");
    auto lease = std::make_unique<CallbackBarrierLease>(std::move(release));
    {
        std::lock_guard lock(state->mutex);
        state->capture_quiescence = std::move(quiescence.Value());
        state->generation = generation;
        state->owner_thread = std::this_thread::get_id();
        state->outer_alive = true;
        state->capture_active = true;
        state->acquiring = false;
    }

    return foundation::Result<std::unique_ptr<savegame::ISaveBarrierLease>>::Success(
        std::move(lease));
} catch (const std::exception &error) {
    return BarrierFailure("gameplay.save_generation.lease_preparation", error.what());
} catch (...) {
    return BarrierFailure("gameplay.save_generation.lease_preparation",
                          "generation lease preparation failed");
}

foundation::Result<std::unique_ptr<savegame::ISaveBarrierLease>>
SaveGenerationBarrier::AcquireCaptureLease() try {
    const auto state = state_;
    auto release = [state]() noexcept {
        std::unique_ptr<savegame::ISaveBarrierLease> quiescence_to_release;
        {
            std::lock_guard lock(state->mutex);
            if (state->nested_capture_leases != 0)
                --state->nested_capture_leases;
            if (state->capture_active && !state->outer_alive && state->nested_capture_leases == 0) {
                state->capture_active = false;
                state->generation = {};

                state->owner_thread = {};

                quiescence_to_release = std::move(state->capture_quiescence);
            }

        }

        quiescence_to_release.reset();
    };

    std::lock_guard lock(state->mutex);
    if (!state->capture_active || !state->outer_alive)
        return BarrierFailure("gameplay.save_generation.outer_lease_required",
                              "Framework capture requires an active outer save generation lease");
    if (state->owner_thread != std::this_thread::get_id())
        return BarrierFailure("gameplay.save_generation.wrong_thread",
                              "nested Framework capture must run on the outer generation thread");
    GenerationFaultPoint("lease.nested");
    auto lease = std::make_unique<CallbackBarrierLease>(std::move(release));
    ++state->nested_capture_leases;
    return foundation::Result<std::unique_ptr<savegame::ISaveBarrierLease>>::Success(
        std::move(lease));
} catch (const std::exception &error) {
    return BarrierFailure("gameplay.save_generation.lease_preparation", error.what());
} catch (...) {
    return BarrierFailure("gameplay.save_generation.lease_preparation",
                          "nested lease preparation failed");
}

foundation::Result<std::unique_ptr<savegame::ISaveBarrierLease>>
SaveGenerationBarrier::AcquireRestoreLease() try {
    const auto state = state_;
    auto release = [state]() noexcept {
        std::unique_ptr<savegame::ISaveBarrierLease> quiescence_to_release;
        {
            std::lock_guard lock(state->mutex);
            if (state->restore_leases == 0)
                return;
            --state->restore_leases;
            if (state->restore_leases == 0) {
                state->restore_active = false;
                state->owner_thread = {};

                quiescence_to_release = std::move(state->restore_quiescence);
            }

        }

        quiescence_to_release.reset();
    };

    {
        std::lock_guard lock(state->mutex);
        if (state->restore_active) {
            if (state->owner_thread != std::this_thread::get_id())
                return BarrierFailure("gameplay.save_generation.wrong_thread",
                                      "nested restore requires the outer restore thread");
            GenerationFaultPoint("lease.restore_nested");
            auto lease = std::make_unique<CallbackBarrierLease>(release);
            ++state->restore_leases;
            return foundation::Result<std::unique_ptr<savegame::ISaveBarrierLease>>::Success(
                std::move(lease));
        }

        if (state->acquiring || state->capture_active)
            return BarrierFailure("gameplay.save_generation.barrier_busy",
                                  "save generation barrier is already active");
        if (!state->callbacks.acquire_restore_quiescence)
            return BarrierFailure("gameplay.save_generation.restore_barrier_unconfigured",
                                  "restore quiescence callback is required");
        state->acquiring = true;
    }

    ScopeExit acquisition_cleanup{[state]() noexcept {
        std::lock_guard lock(state->mutex);
        state->acquiring = false;
    }};

    foundation::Result<std::unique_ptr<savegame::ISaveBarrierLease>> quiescence =
        BarrierFailure("gameplay.save_generation.barrier_callback",
                       "restore quiescence callback did not run");
    try {
        quiescence = state->callbacks.acquire_restore_quiescence();
    } catch (const std::exception &error) {
        std::lock_guard lock(state->mutex);
        state->acquiring = false;
        return BarrierFailure("gameplay.save_generation.barrier_exception", error.what());
    } catch (...) {
        std::lock_guard lock(state->mutex);
        state->acquiring = false;
        return BarrierFailure("gameplay.save_generation.barrier_exception",
                              "restore quiescence callback threw");
    }

    if (!quiescence || !quiescence.Value()) {
        std::lock_guard lock(state->mutex);
        state->acquiring = false;
        return quiescence
                   ? BarrierFailure("gameplay.save_generation.barrier_invalid",
                                    "restore quiescence returned an empty lease")
                   : foundation::Result<std::unique_ptr<savegame::ISaveBarrierLease>>::Failure(
                         quiescence.GetError());
    }

    GenerationFaultPoint("lease.restore");
    auto lease = std::make_unique<CallbackBarrierLease>(std::move(release));
    {
        std::lock_guard lock(state->mutex);
        state->restore_quiescence = std::move(quiescence.Value());
        state->restore_active = true;
        state->owner_thread = std::this_thread::get_id();
        state->restore_leases = 1;
        state->acquiring = false;
    }

    return foundation::Result<std::unique_ptr<savegame::ISaveBarrierLease>>::Success(
        std::move(lease));
} catch (const std::exception &error) {
    return BarrierFailure("gameplay.save_generation.lease_preparation", error.what());
} catch (...) {
    return BarrierFailure("gameplay.save_generation.lease_preparation",
                          "restore lease preparation failed");
}

SaveGenerationId SaveGenerationBarrier::ActiveGeneration() const noexcept {
    const auto state = state_;
    std::lock_guard lock(state->mutex);
    return state->capture_active ? state->generation : SaveGenerationId{};

}

bool SaveGenerationBarrier::IsCaptureActive() const noexcept {
    const auto state = state_;
    std::lock_guard lock(state->mutex);
    return state->capture_active;
}

SaveGenerationCoordinator::SaveGenerationCoordinator(
    savegame::SaveGameOrchestrator &framework, IRuntimeSaveGenerationPort &runtime,
    SaveGenerationBarrier &barrier, ISaveGenerationStore &store, SaveGenerationLimits limits,
    savegame::SaveWireLimits framework_limits) noexcept
    : framework_(framework), runtime_(runtime), barrier_(barrier), store_(store), limits_(limits),
      framework_limits_(framework_limits) {
    (void)framework_.SetBarrier(barrier_);
}

foundation::Result<SaveGenerationId>
SaveGenerationCoordinator::CaptureAndCommit(const savegame::SaveContext &context) try {
    if (!LimitsValid(limits_))
        return foundation::Result<SaveGenerationId>::Failure(
            GenerationError("gameplay.save_generation.invalid_limits",
                            "invalid save generation resource limits"));

    auto current = store_.LoadCurrent();
    if (!current)
        return foundation::Result<SaveGenerationId>::Failure(current.GetError());

    std::uint64_t current_generation = 0;
    if (current.Value()) {
        auto decoded = DecodeSaveGenerationPackage(*current.Value(), limits_);
        if (!decoded)
            return foundation::Result<SaveGenerationId>::Failure(decoded.GetError());
        current_generation = decoded.Value().generation.Raw();
    }

    if (current_generation == std::numeric_limits<std::uint64_t>::max())
        return foundation::Result<SaveGenerationId>::Failure(
            GenerationError("gameplay.save_generation.overflow", "save generation id overflow"));
    const auto generation = SaveGenerationId::FromRaw(current_generation + 1);

    auto outer_lease = barrier_.AcquireGenerationLease(generation);
    if (!outer_lease)
        return foundation::Result<SaveGenerationId>::Failure(outer_lease.GetError());

    // Another coordinator may have published while this caller was acquiring
    // its safe point. Never reuse a generation computed from stale lineage.
    auto stable_current = store_.LoadCurrent();
    if (!stable_current)
        return foundation::Result<SaveGenerationId>::Failure(stable_current.GetError());
    if (stable_current.Value() != current.Value())
        return foundation::Result<SaveGenerationId>::Failure(
            GenerationError("gameplay.save_generation.stale_lineage",
                            "save lineage changed before safe-point acquisition; retry required"));

    auto runtime_capture = runtime_.Capture(generation);
    if (!runtime_capture)
        return foundation::Result<SaveGenerationId>::Failure(runtime_capture.GetError());

    (void)framework_.SetBarrier(barrier_);
    auto framework_image = framework_.Capture(context);
    if (!framework_image)
        return foundation::Result<SaveGenerationId>::Failure(framework_image.GetError());
    auto framework_package =
        savegame::EncodeSaveGameImage(framework_image.Value(), framework_limits_);
    if (!framework_package)
        return foundation::Result<SaveGenerationId>::Failure(framework_package.GetError());

    SaveGenerationPackage package{};

    package.generation = generation;
    package.framework_savegame = std::move(framework_package.Value());
    package.runtime_time = std::move(runtime_capture.Value().time_package);
    package.runtime_persistence = std::move(runtime_capture.Value().persistence_package);
    GenerationFaultPoint("package.encode");
    auto encoded = EncodeSaveGenerationPackage(package, limits_);
    if (!encoded)
        return foundation::Result<SaveGenerationId>::Failure(encoded.GetError());
    auto committed = store_.CommitCurrent(encoded.Value());
    if (!committed)
        return foundation::Result<SaveGenerationId>::Failure(committed.GetError());
    return foundation::Result<SaveGenerationId>::Success(generation);
} catch (const std::exception &error) {
    return foundation::Result<SaveGenerationId>::Failure(
        GenerationError("gameplay.save_generation.capture_exception",
                        "save generation preparation failed", error.what()));
} catch (...) {
    return foundation::Result<SaveGenerationId>::Failure(
        GenerationError("gameplay.save_generation.capture_exception",
                        "save generation preparation failed"));
}

} // namespace epidemic::gameplay::integration
