#include "Epidemic/Runtime/Support/runtime_support.h"

#include <chrono>
#include <iostream>
#include <cstdio>

using namespace epidemic;
using namespace epidemic::runtime;

namespace {
bool Check(bool condition, const char *message) {
    if (!condition)
        std::cerr << "G5.3.B Runtime package " << message << '\n';
    return condition;
}

bool Run() {
    TimeOptions time_options{};

    time_options.game_ticks_per_real_second = 3;
    time_options.initial_time_scale = {1, 3};

    auto time = CreateTimeServices(time_options);
    if (!Check(bool(time), "time bootstrap"))
        return false;
    if (!Check(bool(time.Value().runtime->Advance(std::chrono::microseconds{333333})),
               "fractional time advance"))
        return false;
    const auto checkpoint = time.Value().runtime->CaptureCheckpoint();
    auto time_bytes = EncodeRuntimeTimeCheckpointPackage(checkpoint);
    if (!Check(bool(time_bytes) && !time_bytes.Value().empty(), "in-memory Time v1 encode"))
        return false;
    auto decoded_time = DecodeRuntimeTimeCheckpointPackage(time_bytes.Value());
    if (!Check(bool(decoded_time) && decoded_time.Value().now == checkpoint.now &&
                   decoded_time.Value().revision == checkpoint.revision &&
                   decoded_time.Value().tick_remainder_numerator ==
                       checkpoint.tick_remainder_numerator,
               "in-memory Time v1 roundtrip"))
        return false;
    auto time_bytes_again = EncodeRuntimeTimeCheckpointPackage(decoded_time.Value());
    if (!Check(bool(time_bytes_again) && time_bytes_again.Value() == time_bytes.Value(),
               "Time v1 bytes canonical"))
        return false;

    auto persistence = CreatePersistenceServices();
    if (!Check(bool(persistence), "Persistence bootstrap"))
        return false;
    PersistentObjectRecord object{};

    object.persistent_id = PersistentObjectId{11};

    object.asset_id = AssetId::FromString("runtime.generation.asset");
    object.kind = PersistentObjectKind::Object;
    object.tier = PersistenceTier::Protected;
    object.location.chunk_id = ChunkId{8};

    object.payload.schema_id = foundation::StringId::FromString("runtime.generation.payload");
    object.payload.schema_version = 1;
    object.payload.bytes = {std::byte{42}, std::byte{73}};

    auto tx = persistence.Value().store->OpenTransaction();
    if (!Check(bool(tx->UpsertObject(object)) && bool(tx->Commit()),
               "Persistence seed transaction"))
        return false;
    auto snapshot = CaptureRuntimePersistenceSnapshot(*persistence.Value().query);
    if (!Check(bool(snapshot) && snapshot.Value().objects.size() == 1 &&
                   snapshot.Value().current_revision != 0,
               "quiesced Persistence query snapshot"))
        return false;
    auto persistence_bytes = EncodeRuntimePersistencePackage(snapshot.Value());
    if (!Check(bool(persistence_bytes) && !persistence_bytes.Value().empty(),
               "in-memory Persistence v1 encode"))
        return false;
    auto decoded_persistence = DecodeRuntimePersistencePackage(persistence_bytes.Value());
    if (!Check(bool(decoded_persistence) &&
                   decoded_persistence.Value().current_revision ==
                       snapshot.Value().current_revision &&
                   decoded_persistence.Value().objects.size() == 1 &&
                   decoded_persistence.Value().objects[0].persistent_id == object.persistent_id,
               "in-memory Persistence v1 roundtrip"))
        return false;
    auto persistence_bytes_again = EncodeRuntimePersistencePackage(decoded_persistence.Value());
    if (!Check(bool(persistence_bytes_again) &&
                   persistence_bytes_again.Value() == persistence_bytes.Value(),
               "Persistence v1 bytes canonical"))
        return false;

    auto bad = snapshot.Value();
    bad.objects.push_back(bad.objects.front());
    if (!Check(!EncodeRuntimePersistencePackage(bad), "invalid Persistence snapshot rejected"))
        return false;
    auto corrupt = persistence_bytes.Value();
    corrupt[0] ^= std::byte{1};

    if (!Check(!DecodeRuntimePersistencePackage(corrupt), "corrupt Persistence package rejected"))
        return false;
    return true;
}

} // namespace

int main() {
    try {
        return Run() ? 0 : 1;
    } catch (...) {
        std::fputs("Runtime generation tests threw\n", stderr);
        return 1;
    }

}
