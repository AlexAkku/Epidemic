"""Owning generator for typed participant bindings and their per-boundary tests.

Templates are source artifacts, never acceptance evidence. --check is read-only.
"""
from __future__ import annotations
import argparse
import json
import re
import sys
from pathlib import Path
sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "docs/freeze"))
from goal5_participant_bindings import bindings, DURABLE_MEMBERS, ADAPTER_ACCESS
from goal5_persistence_inventory import matching_brace


def qualification_codec_fields(source, symbol, reviewed_fields):
    start = source.index('struct ' + symbol + '\n')
    opening = source.index('{', start)
    closing = matching_brace(source, opening)
    if closing is None:
        raise ValueError('unbalanced ' + symbol)
    fields = []
    for declaration in source[opening + 1:closing].split(';'):
        if not declaration.strip():
            continue
        declaration = declaration.split('=')[0].split('{')[0].strip()
        match = re.search(r'\b([A-Za-z_]\w*)$', declaration)
        if not match:
            raise ValueError('unsupported ' + symbol + ' field declaration')
        fields.append(match.group(1))
    if fields != reviewed_fields.split():
        raise ValueError(symbol + ' schema changed; explicit qualification codec review required')
    return fields


def qualification_codec_self_test():
    source = 'struct Record\n{ int first{}; std::vector<int> second; };'
    assert qualification_codec_fields(source, 'Record', 'first second') == ['first', 'second']
    for fields in ('second first', 'first', 'first second third'):
        try:
            qualification_codec_fields(source, 'Record', fields)
        except ValueError:
            pass
        else:
            raise AssertionError('qualification codec accepted schema drift: ' + fields)
    print('PASS: qualification codec field order, additions and omissions fail closed.')


INTEGRATION_PUBLICATION_MUTATIONS = {
    "TradeCoordinator": """auto checkpoint = owner.CaptureSnapshot();
        checkpoint.execution_ids.next += 17;
        return owner.RestoreSnapshot(std::move(checkpoint));""",
    "AbilityEffectsDispatcher": """auto checkpoint = owner.CaptureCheckpoint();
        integration::AbilityEffectDeliveryRecord record;
        record.key.execution = {GameplayObjectId::FromString("participant.ability.execution")};
        record.key.output_index = 1;
        record.result.execution = {GameplayObjectId::FromString("participant.effect.execution")};
        record.result.disposition = effects::EffectBatchDisposition::Succeeded;
        record.result.waves = 1;
        checkpoint.delivered.push_back(std::move(record));
        return owner.RestoreCheckpoint(std::move(checkpoint));""",
    "AbilityTimeAdapter": """auto checkpoint = owner.CaptureCheckpoint();
        integration::AbilityTimePendingTrigger pending;
        pending.trigger = ScheduleId::FromString("participant.schedule");
        pending.execution = {GameplayObjectId::FromString("participant.ability.execution")};
        pending.observed_at = GameplayTimePoint{17};
        checkpoint.pending.push_back(std::move(pending));
        return owner.RestoreCheckpoint(std::move(checkpoint));""",
    "ConditionProgressionAdapter": """auto checkpoint = owner.CaptureCheckpoint();
        checkpoint.cursor = ChangeCursor{77, 9};
        return owner.RestoreCheckpoint(checkpoint);""",
    "DeathRewardAdapter": """auto checkpoint = owner.CaptureCheckpoint();
        checkpoint.cursor = combat.LatestChangeCursor();
        return owner.RestoreCheckpoint(std::move(checkpoint));""",
    "InteractionEffectExecutor": """auto snapshot = owner.CaptureSnapshot();
        interaction_effects_integration::InteractionEffectDeliveryRecord record;
        record.execution = {GameplayObjectId::FromString("participant.interaction.execution")};
        record.interaction_type = type;
        record.effect_index = 0;
        record.definition = effect.Value();
        record.state = interaction_effects_integration::InteractionEffectDeliveryState::Pending;
        record.reconciliation = interaction_effects_integration::InteractionEffectReconciliationOutcome::ConfirmedNotApplied;
        snapshot.deliveries.push_back(std::move(record));
        return owner.RestoreSnapshot(std::move(snapshot));""",
    "PopulationEncounterAdapter": """encounters::SpawnRequest request;
        request.encounter = encounter_definition.id;
        request.area = encounter_area;
        request.seed = random::RandomSeed{55};
        auto spawned = owner.SpawnFromPopulation(population, encounters, population_group.Value(), request, 1);
        if (!spawned) return ::epidemic::foundation::Result<void>::Failure(spawned.GetError());
        return ::epidemic::foundation::Result<void>::Success();""",
    "PopulationLifecycleAdapter": """auto snapshot = owner.CaptureSnapshot();
        snapshot.reconciliations.push_back({population::PopulationUnitId::FromString("participant.dead.unit"), Revision{1}, false, true});
        return owner.RestoreSnapshot(std::move(snapshot));""",
    "CrimeSocietyAdapter": """auto checkpoint = owner.CaptureCheckpoint();
        checkpoint.crime_cursor.sequence = 0;
        return owner.RestoreCheckpoint(std::move(checkpoint));""",
    "StateIntegrationPersistence": """auto checkpoint = owner.CaptureCheckpoint();
        checkpoint.facts.entity_cursor.sequence = 0;
        checkpoint.lifecycle.cursor.sequence = 0;
        return owner.RestoreCheckpoint(std::move(checkpoint));""",
    "WorldFactsAdapter": """auto checkpoint = owner.CaptureCheckpoint();
        checkpoint.world_sequence = 0;
        return owner.RestoreCheckpoint(std::move(checkpoint));""",
}

def header(module):
    headers = sorted((ROOT / module / "include").rglob("*.h"))
    if len(headers) != 1:
        raise ValueError(f"explicit header review required: {module}: {headers}")
    return headers[0]


def load_owner_record_fixtures(specs):
    path = ROOT / "tools/goal5_participant_templates/owner_record_fixtures.json"
    fixtures = json.loads(path.read_text(encoding="utf-8"))
    required = {spec["owner"] for spec in specs if spec["id"].startswith("framework.")}
    missing = sorted(required - set(fixtures))
    extra = sorted(set(fixtures) - required)
    if missing or extra:
        raise ValueError(f"owner non-empty fixture coverage mismatch: missing={missing}, extra={extra}")
    for owner in sorted(required):
        fixture = fixtures[owner]
        if not isinstance(fixture, dict) or set(fixture) != {"bootstrap", "seed", "count"}:
            raise ValueError(f"{owner}: owner fixture must contain exactly bootstrap/seed/count")
        if not isinstance(fixture["bootstrap"], str) or not isinstance(fixture["seed"], str) or not fixture["seed"].strip():
            raise ValueError(f"{owner}: owner fixture requires executable non-empty seed")
        if not isinstance(fixture["count"], str) or not fixture["count"].strip():
            raise ValueError(f"{owner}: owner fixture requires durable domain count expression")
    return fixtures


def generate_tests():
    specs = bindings()
    owner_fixtures = load_owner_record_fixtures(specs)
    groups = {}
    for spec in specs:
        if spec["owner"] not in {"ScheduledTriggerDispatcher", "NarrativeExternalConsequenceOutbox"}:
            groups.setdefault(spec["placement"], []).append(spec)
    result = {}
    for placement, group in groups.items():
        include = header(placement).relative_to(ROOT / placement / "include").as_posix()
        lines = ["// Generated by tools/generate_goal5_participants.py --tests-only.",
                 f'#include "{include}"', '#include "participant_test_contract.h"',
                 "using namespace epidemic::gameplay;", "using participant_tests::CheckParticipantContract;"]
        for b in group:
            fixtures = json.loads((ROOT / "tools/goal5_participant_templates/integration_fixtures.json").read_text(encoding="utf-8"))
            lines += [f"void Test{b['owner']}SaveParticipant()", "{"]
            lines += [f'    std::cerr << "[participant] {b["id"]}: bootstrap\\n";']
            if b["id"].startswith("framework."):
                record_fixture = owner_fixtures[b['owner']]
                lines += [f"    {b['namespace']}::{b['owner']} owner;"]
                lines += ['    ' + line for line in record_fixture['bootstrap'].splitlines()]
                lines += ["    participant_tests::FreezeOwner(owner);"]
            else:
                fixture = re.sub(r"(?:\(void\))?\b(\w+)\.Freeze\(\)", r"participant_tests::FreezeOwner(\1)", fixtures[b["owner"]])
                lines += ["    " + fixture]
            lines += ["    savegame::SaveRestoreWorkspace workspace;"]
            if b["owner"] == "PopulationEncounterAdapter":
                lines += ["    auto population_binding = savegame::CreatePopulationServiceSaveParticipant(population, workspace);",
                          "    auto encounter_binding = savegame::CreateEncountersServiceSaveParticipant(encounters, workspace);"]
            dependencies = ", ".join(f'savegame::SaveParticipantId::FromString("{d}")' for d in b["dependencies"])
            lines += [f'    std::cerr << "[participant] {b["id"]}: contract\\n";']
            lines += [f"    auto participant = savegame::Create{b['owner']}SaveParticipant(owner, workspace);",
                f"    CheckParticipantContract(*participant, \"{b['id']}\", {{{dependencies}}});"]
            if b["id"].startswith("framework."):
                expression = "owner." + b["capture"] + "()"
                if b["id"] == "framework.facts": expression += ".Value()"
                lines += [f'    std::cerr << "[participant] {b["id"]}: counters\\n";']
                lines += [f"    participant_tests::CheckOwnerCounterContinuity(owner, *participant, [&] {{ return {expression}; }},",
                          f"        [&](auto snapshot) {{ return owner.{b['restore']}(std::move(snapshot)); }});"]
                lines += [f'    const auto empty_snapshot = {expression};']
                lines += [f'    std::cerr << "[participant] {b["id"]}: non-empty seed\\n";']
                lines += ['    ' + line for line in record_fixture['seed'].splitlines()]
                lines += [f'    std::cerr << "[participant] {b["id"]}: non-empty publication\\n";']
                lines += [f"    participant_tests::CheckNonEmptyPublication(*participant, [&] {{ return {record_fixture['count']}; }},",
                          f"        [&] {{ return owner.{b['restore']}(empty_snapshot); }});"]
            if not b["id"].startswith("framework.") and b["owner"] in INTEGRATION_PUBLICATION_MUTATIONS:
                mutation = INTEGRATION_PUBLICATION_MUTATIONS[b["owner"]]
                lines += ["    participant_tests::CheckChangedStatePublication(*participant, [&]() -> ::epidemic::foundation::Result<void> {"]
                lines += ["        " + line for line in mutation.splitlines()]
                lines += ["    });"]
            lines += ["}"]
        lines += ["int main()", "{"] + [f"    Test{b['owner']}SaveParticipant();" for b in group] + ["    return 0;", "}", ""]
        result[ROOT / "EngineFramework/DevelopmentInfrastructure/Tests" / (placement.split("/")[-1].lower() + "_save_participant_tests.cpp")] = "\n".join(lines)
    return result


def replace_generated(text, name, block):
    begin = f"// BEGIN GENERATED {name}\n"
    end = f"// END GENERATED {name}\n"
    if begin in text:
        first = text.index(begin); last = text.index(end, first) + len(end)
        return text[:first] + begin + block + end + text[last:]
    return text.rstrip() + "\n\n" + begin + block + end


def generate_composition_test():
    specs = bindings()
    lines = ['// Generated from the reviewed participant mapping and explicit composition fixture.']
    for placement in sorted({b["placement"] for b in specs}):
        lines.append(f'#include "{header(placement).relative_to(ROOT / placement / "include").as_posix()}"')
    lines += ['#include "participant_test_contract.h"', '#include <filesystem>', '#include <fstream>', '#include <iterator>',
              'using namespace epidemic::gameplay;',
              'void TestFrameworkSaveComposition(const std::filesystem::path& recording = {})', '{']
    for b in specs:
        if b["id"].startswith("framework."):
            lines.append(f"    {b['namespace']}::{b['owner']} {b['id'].split('.')[1]};")
    fixture = (ROOT / "tools/goal5_participant_templates/composition_fixture.inc").read_text(encoding="utf-8")
    freeze = "\n".join(f"participant_tests::FreezeOwner({b['id'].split('.')[1]});" for b in specs if b["id"].startswith("framework."))
    fixture = fixture.replace("// BEGIN OWNER FREEZE\n// END OWNER FREEZE", freeze)
    lines += ["    " + line for line in fixture.splitlines()]
    lines += ['    savegame::SaveRestoreWorkspace workspace;', '    std::vector<std::unique_ptr<savegame::ISaveParticipant>> owned;']
    aliases = {b['id']: b['id'].split('.')[1] for b in specs}
    aliases.update({'integration.trade':'trade', 'integration.scheduled_trigger_dispatcher':'scheduled',
                    'integration.narrative.external_consequence_delivery':'narrative_outbox'})
    for b in specs:
        alias = aliases[b['id']]
        if b['owner'] == 'ScheduledTriggerDispatcher':
            expression = 'std::make_unique<integration::ScheduledTriggerDispatcherSaveParticipant>(scheduled)'
        elif b['owner'] == 'NarrativeExternalConsequenceOutbox':
            expression = 'std::make_unique<narrative_integration::NarrativeExternalConsequenceSaveParticipant>(narrative_outbox)'
        else:
            expression = f"savegame::Create{b['owner']}SaveParticipant({alias}, workspace)"
        lines.append(f"    owned.push_back({expression});")
    lines += ['    std::vector<savegame::ISaveParticipant*> participants;',
              '    for (auto& participant : owned) participants.push_back(participant.get());',
              '    const std::vector<savegame::SaveParticipantManifest> expected{']
    for b in specs:
        deps = ', '.join(f'savegame::SaveParticipantId::FromString("{d}")' for d in b['dependencies'])
        lines.append(f'        {{savegame::SaveParticipantId::FromString("{b["id"]}"), 1, 0, savegame::SaveParticipantRequirement::Required, {{{deps}}}}},')
    lines += ['    };', '    const std::vector<std::string> fixture_names{']
    lines += [f'        "{b["id"]}",' for b in specs]
    lines += ['    };', '    savegame::SaveGameOrchestrator orchestrator;',
              '    auto missing = participants; missing.pop_back();',
              '    participant_tests::Check(!orchestrator.RegisterComposition(missing, {}, expected), "real composition detects missing participant");',
              '    participant_tests::Check(bool(orchestrator.RegisterComposition(participants, {}, expected)), "all reviewed real participants register and freeze");',
              '    const auto order = orchestrator.ResolveOrder();',
              '    participant_tests::Check(orchestrator.RegistryFrozen() && bool(order) && order.Value().size() == expected.size(), "exact participant set closes registry");',
              '    for (std::size_t i = 0; i < participants.size(); ++i) {',
              '        auto& participant = *participants[i];',
              '        participant_tests::Check(!orchestrator.RegisterParticipant(participant), "late registration rejected for every real participant");',
              '        auto snapshot = participant.CaptureSnapshot({});',
              '        participant_tests::Check(bool(snapshot) && bool(participant.ValidateSnapshot(snapshot.Value(), {})), "every registered real participant captures a valid bootstrap checkpoint");',
              '        const auto path = (recording.empty() ? std::filesystem::path(EPIDEMIC_SAVE_FIXTURES) : recording) / "participant" / fixture_names[i] / "v1.bin";',
              '        if (!recording.empty()) {',
              '            participant_tests::Check(!std::filesystem::exists(path), "never overwrite historical participant fixture");',
              '            std::filesystem::create_directories(path.parent_path());',
              '            std::ofstream file(path, std::ios::binary); const auto& bytes = snapshot.Value().payload;',
              '            file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));',
              '            participant_tests::Check(bool(file), "record explicit participant fixture");',
              '        } else {',
              '            std::ifstream file(path, std::ios::binary); participant_tests::Check(bool(file), "historical participant fixture exists");',
              '            std::vector<char> chars((std::istreambuf_iterator<char>(file)), {}); auto historical = snapshot.Value(); historical.payload.clear();',
              '            for (char c : chars) historical.payload.push_back(std::byte(static_cast<unsigned char>(c)));',
              '            historical.payload_hash = savegame::SaveGameOrchestrator::HashBytes(historical.payload);',
              '            participant_tests::Check(bool(participant.ValidateSnapshot(historical, {})), "all real participants validate historical schema v1 fixture");',
              '            participant_tests::Check(historical.payload == snapshot.Value().payload, "schema-v1 bootstrap bytes remain stable");',
              '        }',
              '    }', '}', 'int main(int argc, char** argv) {',
              '    if (argc == 3 && std::string_view(argv[1]) == "--record-participant-fixtures") TestFrameworkSaveComposition(argv[2]);',
              '    else TestFrameworkSaveComposition();', '    return 0;', '}', '']
    # Reuse the exact production owners, bootstrap and registry construction in
    # whole-engine scenarios. The hook adds immutable scenario contracts before
    # the existing owner freeze; no independent restore ordering is introduced.
    signature = lines.index('void TestFrameworkSaveComposition(const std::filesystem::path& recording = {})')
    body_start = signature + 2
    body_end = lines.index('    };', lines.index('    const std::vector<savegame::SaveParticipantManifest> expected{')) + 1
    body = lines[body_start:body_end]
    body.insert(0, '    savegame::SaveGameOrchestrator orchestrator;')
    view_fields = [f"    {b['namespace']}::{b['owner']}& {aliases[b['id']]};" for b in specs]
    view_fields += ['    state_integration::StateFactsAdapter& state_facts;',
                    '    state_integration::StateLifecycleAdapter& state_lifecycle;',
                    '    state_integration::ConditionEffectsAdapter& state_delivery;',
                    '    state_integration::StateTimeAdapter& state_time;']
    view_fields += ['    savegame::SaveGameOrchestrator& orchestrator;',
                    '    epidemic::gameplay::ClockId clock;', '    epidemic::gameplay::ActionTypeId action;']
    view_args = ', '.join(aliases[b['id']] for b in specs) + ', state_facts, state_lifecycle, state_delivery, state_time, orchestrator, clock.Value(), action.Value()'
    freeze_start = next(i for i, line in enumerate(body) if 'participant_tests::FreezeOwner(' in line)
    mapping_freezes = [line for line in body if any(name + '.FreezeMappings()' in line for name in
                      ('condition_progression', 'death_reward', 'ability_effects'))]
    body = [line for line in body if line not in mapping_freezes]
    freeze_start = next(i for i, line in enumerate(body) if 'participant_tests::FreezeOwner(' in line)
    body[freeze_start:freeze_start] = [f'    FrameworkCompositionView view{{{view_args}}};', '    bootstrap(view);', *mapping_freezes]
    digest_lines = [f'inline constexpr std::size_t kFrameworkSemanticOwnerCount = {len(specs)};',
                    'template <typename Digest>', 'void AddFrameworkSemanticContributions(FrameworkCompositionView& view, Digest& digest)', '{']
    for b in specs:
        expression = f"view.{aliases[b['id']]}.{b['capture']}()"
        if b['id'] == 'framework.facts':
            digest_lines += ['    { auto snapshot = ' + expression + ';',
                '      participant_tests::Check(bool(snapshot), "Facts semantic digest capture");',
                f'      digest.AddSnapshot("{b["id"]}", snapshot.Value()); }}']
        else:
            digest_lines.append(f'    digest.AddSnapshot("{b["id"]}", {expression});')
    digest_lines += ['}']
    utility = ['// Generated from the same reviewed production composition as framework_save_composition_tests.cpp.',
               '#pragma once'] + lines[1:signature-1] + ['namespace goal5_test_utility {',
               'using namespace epidemic::gameplay;', 'struct FrameworkCompositionView {'] + view_fields + [
               '};'] + digest_lines + ['template <typename Bootstrap, typename Use>',
               'decltype(auto) WithFrameworkComposition(Bootstrap&& bootstrap, Use&& use)', '{'] + body + [
               '    participant_tests::Check(bool(orchestrator.RegisterComposition(participants, {}, expected)), "whole-engine exact production participant registry");',
               '    return use(view);', '}', '} // namespace goal5_test_utility', '']
    codecs = ['// Generated exact private value codecs; source remains the production participant implementation.',
              '#pragma once', '#include "Epidemic/GameFramework/SaveGameInternal/save_participant_internal.h"',
              'namespace epidemic::gameplay::savegame::detail {']
    for relative, symbol in [
        ('EngineFramework/IntegrationLayer/NarrativeIntegration/src/save_participants.cpp', 'narrative::NarrativeValue'),
        ('EngineFramework/IntegrationLayer/StateIntegration/src/save_participants.cpp', 'materials::SubstanceAmount')]:
        source = (ROOT / relative).read_text(encoding='utf-8')
        start = source.index('template <> struct SnapshotValueCodec<' + symbol + '>')
        opening = source.index('{', start)
        closing = matching_brace(source, opening)
        if closing is None: raise ValueError('unbalanced reviewed value codec: ' + relative)
        codecs.append(source[start:closing + 2])
    # MSVC C1202 is reproduced for these DTOs in the combined qualification TU.
    # Preserve declaration order and v1 production serialization; specialize
    # only the qualification codec, guarded by an exact reviewed field list.
    for relative, namespace, symbol, reviewed_fields in [
        ('EngineFramework/GameplayWorldStateOwners/Knowledge/include/Epidemic/GameFramework/Knowledge/knowledge.h',
         'knowledge', 'KnowledgeRecord',
         'id owner type topic assertion epistemic_state confidence subject source_kind source derived_from original_source_kind original_source transmission_depth learned_at last_confirmed_at last_decay_at decay_rule persistence payload revision'),
        ('EngineFramework/GameplayWorldStateOwners/Processes/include/Epidemic/GameFramework/Processes/processes.h',
         'processes', 'ProcessInstance',
         'id recipe actor station target simulation_area state started_at due_at last_updated_at reserved_inputs prepared_outputs total_duration paused_remaining quality runtime_payload revision')]:
        source = (ROOT / relative).read_text(encoding='utf-8')
        fields = qualification_codec_fields(source, symbol, reviewed_fields)
        qualified = namespace + '::' + symbol
        codecs += ['template <> struct SnapshotValueCodec<' + qualified + '>', '{',
                   '    template <typename IO> static void Transfer(IO& io, ' + qualified + '& value)', '    {']
        codecs += ['        TransferSnapshot(io, value.' + field + ');' for field in fields]
        codecs += ['    }', '};']
    codecs += ['template <> struct SnapshotValueCodec<::epidemic::gameplay::ChangeCursor>', '{',
               '    template <typename IO> static void Transfer(IO& io, ::epidemic::gameplay::ChangeCursor& value)',
               '    {',
               '        if constexpr (!IO::reading && requires { IO::semantic_observation_identity; })',
               '        {',
               '            auto epoch = static_cast<std::uint64_t>(value.IsValid());',
               '            TransferSnapshot(io, epoch);',
               '        }',
               '        else TransferSnapshot(io, value.epoch);',
               '        TransferSnapshot(io, value.sequence);',
               '    }', '};', '}', '']
    return {
        ROOT / 'EngineFramework/DevelopmentInfrastructure/Tests/framework_save_composition_tests.cpp': '\n'.join(lines),
        ROOT / 'EngineFramework/DevelopmentInfrastructure/Tests/goal5_framework_composition.h': '\n'.join(utility),
        ROOT / 'Tests/Truth/goal5_digest_value_codecs.h': '\n'.join(codecs)}


def generate_bindings():
    groups = {}
    for b in bindings():
        if b["owner"] not in {"ScheduledTriggerDispatcher", "NarrativeExternalConsequenceOutbox"}:
            groups.setdefault(b["placement"], []).append(b)
    result = {}
    for placement, group in groups.items():
        h = header(placement)
        declarations = ['#include "Epidemic/GameFramework/SaveGame/save_game.h"']
        includes = set()
        for b in group:
            owner_header = header(b["owner_module"])
            if owner_header != h:
                includes.add(owner_header.relative_to(ROOT / b["owner_module"] / "include").as_posix())
        declarations += [f'#include "{include}"' for include in sorted(includes)]
        declarations += ["namespace epidemic::gameplay::savegame", "{"]
        access_specs = [b for b in group if b["owner"] != "StateIntegrationPersistence"]
        if placement.endswith("/StateIntegration"):
            access_specs += [{"owner": owner, "namespace": "epidemic::gameplay::state_integration",
                             "snapshot": snapshot, "capture": "CaptureCheckpoint", "restore": "RestoreCheckpoint",
                             "owner_module": placement}
                            for owner, snapshot in [("StateFactsAdapter", "StateFactsCheckpoint"),
                            ("StateLifecycleAdapter", "StateLifecycleCheckpoint"),
                            ("ConditionEffectsAdapter", "ConditionEffectsCheckpoint"),
                            ("StateTimeAdapter", "StateTimeCheckpoint")]]
        for b in group:
            declarations.append(f"[[nodiscard]] std::unique_ptr<ISaveParticipant> Create{b['owner']}SaveParticipant({b['namespace']}::{b['owner']}& owner, SaveRestoreWorkspace& workspace);")
        declarations += ["}", ""]
        result[h] = replace_generated(h.read_text(encoding="utf-8"), "GOAL5 PARTICIPANTS", "\n".join(declarations))
        content = ["// Generated owner bindings; gameplay state remains authoritative in its original owner.",
                   f'#include "{h.relative_to(ROOT / placement / "include").as_posix()}"',
                   '#include "Epidemic/GameFramework/SaveGameInternal/save_participant_internal.h"']
        if placement.endswith("/Integration"):
            content.append((ROOT / "tools/goal5_participant_templates/facts_access.inc").read_text(encoding="utf-8"))
        if placement.endswith("/NarrativeIntegration"):
            content.append((ROOT / "tools/goal5_participant_templates/narrative_values.inc").read_text(encoding="utf-8"))
        content += ["namespace epidemic::gameplay", "{"]
        for b in access_specs:
            if b["owner"] == "GameplayFactsService": continue
            qualified = b["namespace"] + "::" + b["owner"]
            content += [f"template <> struct SnapshotRestoreAccess<{qualified}>", "{",
                f"    using Owner = {qualified};", f"    using Snapshot = {b['namespace']}::{b['snapshot']};",
                f"    static Snapshot Capture(const Owner& o) {{ return o.{b['capture']}(); }}",
                "    static std::unique_ptr<Owner> Clone(const Owner& o, savegame::SaveRestoreWorkspace& w)", "    {"]
            if b["owner"] in ADAPTER_ACCESS:
                constructor, config, members = ADAPTER_ACCESS[b["owner"]]
                content.append("        (void)w;")
                content.append(f"        auto copy = std::make_unique<Owner>({constructor});")
                for field in config.split(): content.append(f"        copy->{field} = o.{field};")
                content += ["        return copy;", "    }"]
            else:
                content += ["        (void)w; return std::make_unique<Owner>(o);", "    }"]
                members = DURABLE_MEMBERS[b["owner"]]
            restore = f"o.{b['restore']}(std::move(s))"
            if b["owner"] == "PopulationEncounterAdapter":
                restore = "o.RestoreSnapshot(std::move(s), w.Require<population::PopulationService>(), w.Require<encounters::EncountersService>())"
            content += [f"    static ::epidemic::foundation::Result<void> Restore(Owner& o, Snapshot s, savegame::SaveRestoreWorkspace& w) {{ (void)w; return {restore}; }}",
                "    static Owner& View(Owner& o) noexcept { return o; }",
                "    static void Commit(Owner& live, Owner& candidate) noexcept", "    {"]
            for member in members.split():
                content.append(f"        savegame::detail::SwapPreparedMember(live.{member}, candidate.{member});")
            content += ["    }", "};"]
            owner_header = header(b["owner_module"])
            owner_text = result.get(owner_header, owner_header.read_text(encoding="utf-8"))
            friend = f"    friend struct epidemic::gameplay::SnapshotRestoreAccess<{b['owner']}>;\n"
            if friend not in owner_text:
                owner_text, count = re.subn(r"(class\s+" + b["owner"] + r"\b[^;{]*\{)", r"\1\n" + friend, owner_text, count=1)
                if count != 1: raise ValueError(f"cannot insert ownership-local friend for {qualified}")
            result[owner_header] = owner_text
        content += ["}"]
        if placement.endswith("/StateIntegration"):
            content.append((ROOT / "tools/goal5_participant_templates/state_access.inc").read_text(encoding="utf-8"))
            state_text = result.get(h, h.read_text(encoding="utf-8"))
            friend = "    friend struct epidemic::gameplay::SnapshotRestoreAccess<StateIntegrationPersistence>;\n"
            if friend not in state_text:
                state_text = state_text.replace("class StateIntegrationPersistence\n{", "class StateIntegrationPersistence\n{\n" + friend)
            result[h] = state_text
        content += ["namespace epidemic::gameplay::savegame", "{"]
        for b in group:
            dependencies = ", ".join(f'SaveParticipantId::FromString("{d}")' for d in b["dependencies"])
            content += [f"std::unique_ptr<ISaveParticipant> Create{b['owner']}SaveParticipant({b['namespace']}::{b['owner']}& owner, SaveRestoreWorkspace& workspace)", "{",
                        f"    return std::make_unique<detail::SnapshotParticipant<{b['namespace']}::{b['owner']}>>(owner, workspace, \"{b['id']}\", std::vector<SaveParticipantId>{{{dependencies}}});", "}"]
        content += ["}", ""]
        result[ROOT / placement / "src/save_participants.cpp"] = "\n".join(content)
        cmake = ROOT / placement / "CMakeLists.txt"
        cmake_text = cmake.read_text(encoding="utf-8")
        target = "EpidemicGameFramework" + ("CoreIntegration" if placement.endswith("/Integration") else placement.split("/")[-1])
        extra = f"target_sources({target} PRIVATE src/save_participants.cpp)"
        private_include = 'target_include_directories(' + target + ' PRIVATE "${CMAKE_SOURCE_DIR}/EngineFramework/GameplayWorldStateOwners/SaveGame/internal")'
        if extra not in cmake_text:
            cmake_text = cmake_text.rstrip() + f"\n\n# M5.2 ownership-local save bindings; SaveGame never links back to owners.\n{extra}\ntarget_link_libraries({target} PUBLIC EpidemicGameFrameworkSaveGame)\n"
        if private_include not in cmake_text:
            cmake_text = cmake_text.rstrip() + "\n" + private_include + "\n"
        result[cmake] = cmake_text
    # Generate only value-field visitor boilerplate in the PRIVATE mechanics header.
    internal_header = ROOT / "EngineFramework/GameplayWorldStateOwners/SaveGame/internal/Epidemic/GameFramework/SaveGameInternal/save_participant_internal.h"
    visitor = ["namespace epidemic::gameplay::savegame::detail", "{", "template <typename T, typename F> void VisitSnapshotFields(T& value, F&& visitor)", "{",
               "    constexpr auto n = AggregateArity<T>();", "    if constexpr (n == 0) visitor();"]
    for n in range(1, 33):
        fields = ", ".join(f"f{i}" for i in range(n))
        visitor.append(f"    else if constexpr (n == {n}) {{ auto& [{fields}] = value; visitor({fields}); }}")
    visitor += ["}", "}", ""]
    internal_text = internal_header.read_text(encoding="utf-8")
    result[internal_header] = replace_generated(internal_text, "GOAL5 VALUE VISITOR", "\n".join(visitor))
    facts_header = header("EngineFramework/BaseInfrastructure/Facts")
    facts = facts_header.read_text(encoding="utf-8")
    friend = "    friend struct epidemic::gameplay::SnapshotRestoreAccess<GameplayFactsService>;\n"
    if friend not in facts:
        facts = facts.replace("class GameplayFactsService\n{", "class GameplayFactsService\n{\n" + friend)
    result[facts_header] = facts
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tests-only", action="store_true")
    parser.add_argument("--record", action="store_true")
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        qualification_codec_self_test()
        if not args.record and not args.check:
            return 0
    if not args.record and not args.check:
        parser.error("choose --record or --check")
    artifacts = generate_tests()
    artifacts.update(generate_composition_test())
    if not args.tests_only:
        artifacts.update(generate_bindings())
    errors = []
    for path, text in artifacts.items():
        if args.record and (not path.is_file() or path.read_text(encoding="utf-8") != text):
            path.write_text(text, encoding="utf-8", newline="\n")
        if args.check and (not path.is_file() or path.read_text(encoding="utf-8") != text):
            errors.append(f"stale generated source: {path.relative_to(ROOT)}")
    if errors:
        print("\n".join(errors), file=sys.stderr); return 1
    print(f"PASS: {len(artifacts)} generated participant test artifacts.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
