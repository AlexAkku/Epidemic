#include "Epidemic/Foundation/error.h"
#include "Epidemic/GameFramework/SaveGame/save_game.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace epidemic::gameplay;
using namespace epidemic::gameplay::savegame;

namespace epidemic::gameplay::savegame::detail
{
void SetSaveGameFaultPointForTesting(std::string_view point) noexcept;
void ClearSaveGameFaultPointForTesting() noexcept;
}

namespace
{
void Check(bool value, const char *message)
{
    if (!value)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

struct BarrierLease final : ISaveBarrierLease
{
    explicit BarrierLease(bool &active) : active_(&active)
    {
        active = true;
    }
    ~BarrierLease() override
    {
        *active_ = false;
    }
    bool *active_ = nullptr;
};

class Barrier final : public ISaveBarrier
{
  public:
    bool active = false;
    bool fail_capture = false;
    bool fail_restore = false;
    int capture_acquires = 0;
    int restore_acquires = 0;

    epidemic::foundation::Result<std::unique_ptr<ISaveBarrierLease>> AcquireCaptureLease() override
    {
        ++capture_acquires;
        if (fail_capture)
        {
            return epidemic::foundation::Result<std::unique_ptr<ISaveBarrierLease>>::Failure(
                epidemic::foundation::Error::Create("barrier", "capture failed"));
        }
        return epidemic::foundation::Result<std::unique_ptr<ISaveBarrierLease>>::Success(
            std::make_unique<BarrierLease>(active));
    }

    epidemic::foundation::Result<std::unique_ptr<ISaveBarrierLease>> AcquireRestoreLease() override
    {
        ++restore_acquires;
        if (fail_restore)
        {
            return epidemic::foundation::Result<std::unique_ptr<ISaveBarrierLease>>::Failure(
                epidemic::foundation::Error::Create("barrier", "restore failed"));
        }
        return epidemic::foundation::Result<std::unique_ptr<ISaveBarrierLease>>::Success(
            std::make_unique<BarrierLease>(active));
    }
};

struct Stage final : IRestoreStage
{
    int value = 0;
};

class Part final : public ISaveParticipant
{
  public:
    SaveParticipantId id{};
    SaveSchemaVersion version = 1;
    SaveParticipantRequirement requirement = SaveParticipantRequirement::Required;
    std::vector<SaveParticipantId> deps;
    int value = 0;
    bool fail_stage = false;
    bool throw_validate = false;
    bool throw_capture = false;
    bool *barrier_active = nullptr;
    std::vector<SaveParticipantId> *commit_order = nullptr;

    SaveParticipantId Id() const noexcept override
    {
        return id;
    }
    SaveSchemaVersion SchemaVersion() const noexcept override
    {
        return version;
    }
    SaveParticipantRequirement Requirement() const noexcept override
    {
        return requirement;
    }
    std::vector<SaveParticipantId> Dependencies() const override
    {
        return deps;
    }
    epidemic::foundation::Result<SaveSection> CaptureSnapshot(const SaveContext &) const override
    {
        if (throw_capture)
        {
            throw std::runtime_error("capture exception");
        }
        if (barrier_active != nullptr && !*barrier_active)
        {
            return epidemic::foundation::Result<SaveSection>::Failure(
                epidemic::foundation::Error::Create("barrier", "capture outside barrier"));
        }
        SaveSection section;
        section.participant = id;
        section.schema_version = version;
        section.payload.resize(sizeof(value));
        std::memcpy(section.payload.data(), &value, sizeof(value));
        return epidemic::foundation::Result<SaveSection>::Success(std::move(section));
    }
    epidemic::foundation::Result<void> ValidateSnapshot(const SaveSection &section,
                                                        const RestoreContext &) const override
    {
        if (throw_validate)
        {
            throw std::runtime_error("validation exception");
        }
        return section.payload.size() == sizeof(int)
                   ? epidemic::foundation::Result<void>::Success()
                   : epidemic::foundation::Result<void>::Failure(
                         epidemic::foundation::Error::Create("invalid", "invalid payload"));
    }
    epidemic::foundation::Result<std::unique_ptr<IRestoreStage>> StageRestore(const SaveSection &section,
                                                                              const RestoreContext &) override
    {
        if (fail_stage)
        {
            return epidemic::foundation::Result<std::unique_ptr<IRestoreStage>>::Failure(
                epidemic::foundation::Error::Create("stage", "failed"));
        }
        auto stage = std::make_unique<Stage>();
        std::memcpy(&stage->value, section.payload.data(), sizeof(int));
        return epidemic::foundation::Result<std::unique_ptr<IRestoreStage>>::Success(std::move(stage));
    }
    void CommitRestore(IRestoreStage &stage) noexcept override
    {
        if (barrier_active != nullptr && !*barrier_active)
        {
            std::abort();
        }
        value = static_cast<Stage &>(stage).value;
        if (commit_order != nullptr)
        {
            commit_order->push_back(id);
        }
    }
};

class Migration final : public ISaveMigration
{
  public:
    SaveParticipantId id{};
    SaveSchemaVersion from = 1;
    SaveSchemaVersion to = 2;
    bool corrupt_identity = false;

    SaveParticipantId Participant() const noexcept override
    {
        return id;
    }
    SaveSchemaVersion FromVersion() const noexcept override
    {
        return from;
    }
    SaveSchemaVersion ToVersion() const noexcept override
    {
        return to;
    }
    epidemic::foundation::Result<SaveSection> Migrate(const SaveSection &source) const override
    {
        auto result = source;
        result.schema_version = to;
        if (corrupt_identity)
        {
            result.participant = SaveParticipantId::FromString("wrong");
        }
        return epidemic::foundation::Result<SaveSection>::Success(std::move(result));
    }
};

SaveContext SaveCtx()
{
    SaveContext context;
    context.content_version = 4;
    context.content_hash = 99;
    context.engine_version = 7;
    context.framework_version = 8;
    return context;
}

RestoreContext ExactRestoreCtx()
{
    RestoreContext context;
    context.compatibility = ContentCompatibilityPolicy::ExactRequired;
    context.current_content_version = 4;
    context.current_content_hash = 99;
    context.current_engine_version = 7;
    context.current_framework_version = 8;
    return context;
}

void Rehash(SaveGameImage &image, std::size_t index)
{
    auto &section = image.sections[index];
    section.payload_hash = SaveGameOrchestrator::HashBytes(section.payload);
    auto manifest_it = std::find_if(image.manifest.participants.begin(), image.manifest.participants.end(),
                                    [&](const SaveParticipantManifest &entry) {
                                        return entry.id == section.participant;
                                    });
    Check(manifest_it != image.manifest.participants.end(), "manifest entry found for rehash");
    manifest_it->payload_hash = section.payload_hash;
    manifest_it->schema_version = section.schema_version;
}
} // namespace

void TestSnapshotParticipantMechanics();
void TestSaveWireCodec();
void RecordSaveWireFixtures(const std::filesystem::path& root);
void TestCompositionRegistryClosure()
{
    Part a; a.id = SaveParticipantId::FromString("composition.a"); a.version = 2;
    Part b; b.id = SaveParticipantId::FromString("composition.b"); b.deps = {a.id};
    Migration migration; migration.id = a.id;
    std::vector<ISaveParticipant*> participants{&b, &a};
    std::vector<const ISaveMigration*> migrations{&migration};
    std::vector<SaveParticipantManifest> expected{
        {a.id, 2, 0, SaveParticipantRequirement::Required, {}},
        {b.id, 1, 0, SaveParticipantRequirement::Required, {a.id}}};
    SaveGameOrchestrator orchestrator;
    auto missing = participants; missing.pop_back();
    Check(!orchestrator.RegisterComposition(missing, migrations, expected), "composition rejects missing participant");
    auto duplicate = participants; duplicate.push_back(&a);
    Check(!orchestrator.RegisterComposition(duplicate, migrations, expected), "composition rejects duplicate participant");
    auto wrong = expected; wrong[0].schema_version = 3;
    Check(!orchestrator.RegisterComposition(participants, migrations, wrong), "composition rejects schema drift");
    wrong = expected; wrong[1].dependencies.clear();
    Check(!orchestrator.RegisterComposition(participants, migrations, wrong), "composition rejects dependency drift");
    wrong = expected; wrong[0].requirement = SaveParticipantRequirement::Optional;
    Check(!orchestrator.RegisterComposition(participants, migrations, wrong), "composition rejects requirement drift");
    wrong = expected; wrong.push_back(expected[0]);
    Check(!orchestrator.RegisterComposition(participants, migrations, wrong), "composition rejects duplicate contract identity");
    auto nulls = participants; nulls[0] = nullptr;
    Check(!orchestrator.RegisterComposition(nulls, migrations, expected), "composition rejects null participant");
    migration.id = SaveParticipantId::FromString("orphan.migration");
    Check(!orchestrator.RegisterComposition(participants, migrations, expected), "composition rejects orphan migration");
    migration.id = a.id;
    auto duplicate_migrations = migrations; duplicate_migrations.push_back(&migration);
    Check(!orchestrator.RegisterComposition(participants, duplicate_migrations, expected), "composition rejects duplicate migration");
    a.deps = {b.id}; wrong = expected; wrong[0].dependencies = a.deps;
    Check(!orchestrator.RegisterComposition(participants, migrations, wrong), "composition rejects cyclic contract");
    a.deps.clear();
    Check(!orchestrator.RegistryFrozen() && orchestrator.ResolveOrder().Value().empty(),
          "failed composition leaves registry empty and unfrozen");
    Check(bool(orchestrator.RegisterComposition(participants, migrations, expected)), "exact composition registers and freezes");
    Check(orchestrator.RegistryFrozen() && orchestrator.ResolveOrder().Value().size() == 2, "composition closes exact registry");
    Check(!orchestrator.RegisterParticipant(a) && !orchestrator.RegisterMigration(migration), "late registration rejected");
    Check(!orchestrator.RegisterComposition(participants, migrations, expected), "late composition rejected");
    Barrier barrier;
    Check(bool(orchestrator.SetBarrier(barrier)), "composition barrier");
    a.value = 73;
    auto image = orchestrator.Capture(SaveCtx());
    Check(bool(image), "composition captures exact participants");
    auto old = std::find_if(image.Value().sections.begin(), image.Value().sections.end(), [&](const auto& s) { return s.participant == a.id; });
    old->schema_version = 1;
    Rehash(image.Value(), static_cast<std::size_t>(old - image.Value().sections.begin()));
    a.value = 99;
    auto context = ExactRestoreCtx(); context.compatibility = ContentCompatibilityPolicy::MigrationRequired;
    Check(bool(orchestrator.Restore(std::move(image.Value()), context)) && a.value == 73,
          "composition registers migration before freeze and uses it on restore");
}

void TestCompatibilityProfileMatrix()
{
    for (auto policy : {ContentCompatibilityPolicy::ExactRequired, ContentCompatibilityPolicy::CompatibleVersion,
         ContentCompatibilityPolicy::MigrationRequired, ContentCompatibilityPolicy::BestEffort, ContentCompatibilityPolicy::DeveloperMode})
    {
        Barrier barrier; Part required; required.id=SaveParticipantId::FromString("profile.required"); required.value=42;
        Part optional; optional.id=SaveParticipantId::FromString("profile.optional"); optional.requirement=SaveParticipantRequirement::Optional; optional.value=17;
        SaveGameOrchestrator orchestrator;
        Check(bool(orchestrator.SetBarrier(barrier)) && bool(orchestrator.RegisterParticipant(required)) && bool(orchestrator.RegisterParticipant(optional)), "profile setup");
        auto captured=orchestrator.Capture(SaveCtx()); Check(bool(captured), "profile captures current schema");
        auto context=ExactRestoreCtx(); context.compatibility=policy;
        required.value=99; optional.value=98;
        auto package=EncodeSaveGameImage(captured.Value()); Check(bool(package), "profile image encodes");
        auto decoded=DecodeSaveGameImage(package.Value()); Check(bool(decoded), "profile image decodes");
        Check(bool(orchestrator.Restore(std::move(decoded.Value()),context)) && required.value==42 && optional.value==17,
            "each compatibility profile restores supported v1");
        auto future=captured.Value();
        for (std::size_t i=0; i<future.sections.size(); ++i) if (future.sections[i].participant==required.id)
        { future.sections[i].schema_version=2; Rehash(future,i); }
        required.value=99; optional.value=98;
        Check(!orchestrator.Restore(std::move(future),context) && required.value==99 && optional.value==98,
            "all profiles reject future required schema without publication");
        future=captured.Value();
        for (std::size_t i=0; i<future.sections.size(); ++i) if (future.sections[i].participant==optional.id)
        { future.sections[i].schema_version=2; Rehash(future,i); }
        const bool permissive=policy==ContentCompatibilityPolicy::BestEffort || policy==ContentCompatibilityPolicy::DeveloperMode;
        const auto result=orchestrator.Restore(std::move(future),context);
        Check(bool(result)==permissive && optional.value==98 && required.value==(permissive?42:99),
            "strict profiles reject future optional schema; permissive profiles skip it explicitly");
        required.value=99;
        auto unknown=captured.Value(); auto extra=unknown.sections.front(); extra.participant=SaveParticipantId::FromString("profile.unknown");
        unknown.sections.push_back(extra); unknown.manifest.participants.push_back({extra.participant,1,extra.payload_hash,SaveParticipantRequirement::Optional,{}});
        auto unknownWire=EncodeSaveGameImage(unknown); Check(bool(unknownWire),"wire preserves unknown optional participant metadata");
        auto unknownDecoded=DecodeSaveGameImage(unknownWire.Value()); Check(bool(unknownDecoded),"unknown optional participant structurally decodes");
        Check(bool(orchestrator.Restore(std::move(unknownDecoded.Value()),context))==(policy!=ContentCompatibilityPolicy::ExactRequired), "unknown optional semantics by profile");
        unknown.manifest.participants.back().requirement=SaveParticipantRequirement::Required; required.value=99;
        unknownWire=EncodeSaveGameImage(unknown); Check(bool(unknownWire),"wire preserves unknown required participant metadata");
        unknownDecoded=DecodeSaveGameImage(unknownWire.Value()); Check(bool(unknownDecoded),"unknown required participant structurally decodes");
        Check(bool(orchestrator.Restore(std::move(unknownDecoded.Value()),context))==(policy==ContentCompatibilityPolicy::DeveloperMode), "unknown required semantics by profile");
        auto metadata=captured.Value(); metadata.manifest.content_version=100; metadata.manifest.engine_version=100;
        Check(bool(orchestrator.Restore(std::move(metadata),context))==permissive, "only permissive profiles ignore newer metadata");
    }
    // Historical chains are tested on a synthetic v2 participant, never advertised
    // as supported history of the 47 current-v1 production participants.
    for (auto policy : {ContentCompatibilityPolicy::CompatibleVersion,ContentCompatibilityPolicy::MigrationRequired})
    {
        Barrier barrier; Part owner; owner.id=SaveParticipantId::FromString("profile.migration"); owner.version=2; owner.value=73;
        Migration migration; migration.id=owner.id; SaveGameOrchestrator orchestrator;
        Check(bool(orchestrator.SetBarrier(barrier)) && bool(orchestrator.RegisterParticipant(owner)) && bool(orchestrator.RegisterMigration(migration)), "strict migration profile setup");
        auto image=orchestrator.Capture(SaveCtx()).Value(); image.sections[0].schema_version=1; Rehash(image,0); owner.value=99;
        auto context=ExactRestoreCtx(); context.compatibility=policy;
        Check(bool(orchestrator.Restore(std::move(image),context)) && owner.value==73, "explicit historical chain works in both migration-capable strict profiles");
    }
}

int main(int argc, char** argv)
{
    if (argc == 3 && std::string_view(argv[1]) == "--record-wire-fixtures")
    { RecordSaveWireFixtures(argv[2]); return 0; }
    TestCompositionRegistryClosure();
    TestSnapshotParticipantMechanics();
    TestSaveWireCodec();
    TestCompatibilityProfileMatrix();
    Barrier barrier;
    std::vector<SaveParticipantId> commit_order;
    Part a;
    a.id = SaveParticipantId::FromString("a");
    a.value = 11;
    a.barrier_active = &barrier.active;
    a.commit_order = &commit_order;
    Part b;
    b.id = SaveParticipantId::FromString("b");
    b.value = 22;
    b.deps = {a.id};
    b.barrier_active = &barrier.active;
    b.commit_order = &commit_order;

    SaveGameOrchestrator orchestrator;
    Check(static_cast<bool>(orchestrator.SetBarrier(barrier)), "set barrier");
    Check(static_cast<bool>(orchestrator.RegisterParticipant(a)), "register a");
    Check(static_cast<bool>(orchestrator.RegisterParticipant(b)), "register b");
    auto order = orchestrator.ResolveOrder();
    Check(static_cast<bool>(order) && order.Value().size() == 2 && order.Value()[0] == a.id && order.Value()[1] == b.id,
          "dependency order deterministic");

    auto image = orchestrator.Capture(SaveCtx());
    Check(static_cast<bool>(image), "capture");
    Check(!barrier.active && barrier.capture_acquires == 1, "capture barrier lease released");
    Check(orchestrator.RegistryFrozen(), "registry freezes on first operation");
    Part late;
    late.id = SaveParticipantId::FromString("late");
    Check(!orchestrator.RegisterParticipant(late), "registration rejected after freeze");

    a.value = 100;
    b.value = 200;
    b.fail_stage = true;
    Check(!orchestrator.Restore(image.Value(), ExactRestoreCtx()), "staging failure");
    Check(a.value == 100 && b.value == 200, "staging failure leaves every participant untouched");
    Check(!barrier.active, "restore barrier released after failed stage");

    b.fail_stage = false;
    commit_order.clear();
    Check(static_cast<bool>(orchestrator.Restore(image.Value(), ExactRestoreCtx())), "restore");
    Check(a.value == 11 && b.value == 22, "values restored");
    Check(commit_order.size() == 2 && commit_order[0] == a.id && commit_order[1] == b.id,
          "commit ordering deterministic");
    Check(!barrier.active, "restore barrier released after commit");

    auto bad_metadata = image.Value();
    bad_metadata.manifest.framework_version = 9;
    Check(!orchestrator.Restore(std::move(bad_metadata), ExactRestoreCtx()), "exact metadata mismatch rejected");

    auto corrupt_manifest = image.Value();
    corrupt_manifest.manifest.participants[0].payload_hash ^= 1;
    Check(!orchestrator.Restore(std::move(corrupt_manifest), ExactRestoreCtx()), "manifest section mismatch rejected");

    auto unsupported = image.Value();
    unsupported.manifest.save_version = 2;
    Check(!orchestrator.Restore(std::move(unsupported), ExactRestoreCtx()), "unsupported save format rejected");

    Part throwing;
    throwing.id = SaveParticipantId::FromString("throwing");
    throwing.value = 5;
    throwing.throw_validate = true;
    Barrier exception_barrier;
    SaveGameOrchestrator exception_orchestrator;
    Check(static_cast<bool>(exception_orchestrator.SetBarrier(exception_barrier)), "exception barrier");
    Check(static_cast<bool>(exception_orchestrator.RegisterParticipant(throwing)), "register throwing participant");
    throwing.throw_validate = false;
    auto throwing_image = exception_orchestrator.Capture(SaveCtx());
    Check(static_cast<bool>(throwing_image), "capture throwing participant image");
    throwing.throw_validate = true;
    Check(!exception_orchestrator.Restore(std::move(throwing_image.Value()), ExactRestoreCtx()),
          "participant exception converted to error");

    Part migrated_part;
    migrated_part.id = SaveParticipantId::FromString("migrated");
    migrated_part.version = 2;
    migrated_part.value = 31;
    Migration migration;
    migration.id = migrated_part.id;
    Barrier migration_barrier;
    SaveGameOrchestrator migration_orchestrator;
    Check(static_cast<bool>(migration_orchestrator.SetBarrier(migration_barrier)), "migration barrier");
    Check(static_cast<bool>(migration_orchestrator.RegisterParticipant(migrated_part)), "register migrated participant");
    Check(static_cast<bool>(migration_orchestrator.RegisterMigration(migration)), "register migration");
    auto migrated_image = migration_orchestrator.Capture(SaveCtx());
    Check(static_cast<bool>(migrated_image), "capture migrated participant");
    migrated_image.Value().sections[0].schema_version = 1;
    Rehash(migrated_image.Value(), 0);
    migrated_part.value = 44;
    auto migration_context = ExactRestoreCtx();
    migration_context.compatibility = ContentCompatibilityPolicy::MigrationRequired;
    Check(static_cast<bool>(migration_orchestrator.Restore(migrated_image.Value(), migration_context)),
          "explicit migration chain restores old schema");
    Check(migrated_part.value == 31, "migrated value committed");

    auto bad_migration_image = migrated_image.Value();
    migration.corrupt_identity = true;
    migrated_part.value = 52;
    Check(!migration_orchestrator.Restore(std::move(bad_migration_image), migration_context),
          "migration cannot change participant identity");
    Check(migrated_part.value == 52, "failed migration leaves live state untouched");
    migration.corrupt_identity = false;

    Part required;
    required.id = SaveParticipantId::FromString("required");
    required.value = 61;
    Part optional;
    optional.id = SaveParticipantId::FromString("optional");
    optional.requirement = SaveParticipantRequirement::Optional;
    optional.value = 62;
    Barrier optional_barrier;
    SaveGameOrchestrator optional_orchestrator;
    Check(static_cast<bool>(optional_orchestrator.SetBarrier(optional_barrier)), "optional barrier");
    Check(static_cast<bool>(optional_orchestrator.RegisterParticipant(required)), "register required");
    Check(static_cast<bool>(optional_orchestrator.RegisterParticipant(optional)), "register optional");
    auto optional_image = optional_orchestrator.Capture(SaveCtx());
    Check(static_cast<bool>(optional_image), "capture optional image");
    optional_image.Value().sections.erase(
        std::remove_if(optional_image.Value().sections.begin(), optional_image.Value().sections.end(),
                       [&](const SaveSection &section) { return section.participant == optional.id; }),
        optional_image.Value().sections.end());
    optional_image.Value().manifest.participants.erase(
        std::remove_if(optional_image.Value().manifest.participants.begin(),
                       optional_image.Value().manifest.participants.end(),
                       [&](const SaveParticipantManifest &entry) { return entry.id == optional.id; }),
        optional_image.Value().manifest.participants.end());
    required.value = 70;
    optional.value = 71;
    auto best_effort_context = ExactRestoreCtx();
    best_effort_context.compatibility = ContentCompatibilityPolicy::BestEffort;
    Check(static_cast<bool>(optional_orchestrator.Restore(std::move(optional_image.Value()), best_effort_context)),
          "best effort permits missing optional participant");
    Check(required.value == 61 && optional.value == 71, "missing optional participant leaves its live state unchanged");

    SaveResourceLimits small_limits;
    small_limits.max_section_bytes = 2;
    small_limits.max_total_payload_bytes = 2;
    Barrier limited_barrier;
    Part limited;
    limited.id = SaveParticipantId::FromString("limited");
    SaveGameOrchestrator limited_orchestrator({}, small_limits);
    Check(static_cast<bool>(limited_orchestrator.SetBarrier(limited_barrier)), "limited barrier");
    Check(static_cast<bool>(limited_orchestrator.RegisterParticipant(limited)), "register limited");
    Check(!limited_orchestrator.Capture(SaveCtx()), "capture resource limit enforced");

    Part no_barrier_part;
    no_barrier_part.id = SaveParticipantId::FromString("no_barrier");
    SaveGameOrchestrator no_barrier;
    Check(static_cast<bool>(no_barrier.RegisterParticipant(no_barrier_part)), "register no barrier participant");
    Check(!no_barrier.Capture(SaveCtx()), "capture without quiescence barrier rejected");

    Part c;
    c.id = SaveParticipantId::FromString("c");
    c.deps = {SaveParticipantId::FromString("d")};
    Part d;
    d.id = SaveParticipantId::FromString("d");
    d.deps = {c.id};
    Barrier cycle_barrier;
    SaveGameOrchestrator cycle;
    Check(static_cast<bool>(cycle.SetBarrier(cycle_barrier)), "cycle barrier");
    Check(static_cast<bool>(cycle.RegisterParticipant(c)) && static_cast<bool>(cycle.RegisterParticipant(d)),
          "cycle participants");
    Check(!cycle.ResolveOrder(), "dependency cycle rejected");

    // A deterministic module-local fail point immediately before commit proves
    // that all fallible restore work completes before live participant publication.
    commit_order.clear();
    commit_order.reserve(2);
    auto allocation_image = image.Value();
    a.value = 100;
    b.value = 200;
    epidemic::gameplay::savegame::detail::SetSaveGameFaultPointForTesting("restore.pre_commit");
    bool saw_restore_allocation_failure = false;
    try
    {
        (void)orchestrator.Restore(std::move(allocation_image), ExactRestoreCtx());
    }
    catch (const std::bad_alloc &)
    {
        saw_restore_allocation_failure = true;
    }
    epidemic::gameplay::savegame::detail::ClearSaveGameFaultPointForTesting();
    Check(saw_restore_allocation_failure, "save restore exercised local pre-commit allocation failure");
    Check(a.value == 100 && b.value == 200, "allocation failure leaves participants untouched");
    Check(commit_order.empty(), "allocation failure does not enter participant commit");
    Check(!barrier.active, "allocation failure releases restore barrier");
    return 0;
}
