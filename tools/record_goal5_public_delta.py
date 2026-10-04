"""Record the explicitly reviewed M5.2 callable delta into raw Goal 4 handoff sources.

Never writes canonical freeze JSON. Run the existing merge/generators afterwards.
Unknown callables stop recording; passing CTest evidence is required separately.
"""
from __future__ import annotations
import argparse
import json
import re
import tempfile
import os
import sys
from pathlib import Path
sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'docs/freeze'))
from module_dossiers import discover_modules
from public_api_inventory import scan_modules
from coverage_manifests import surface_id
from goal5_participant_bindings import bindings

MECHANICS = {
    'SaveRestoreWorkspace': ('CONSTRUCTOR', 'save_participant_mechanics_tests.cpp', 'Check(missing_rejected && &savegame::detail::WorkspaceTestAccess::Resolve(missing, live)'),
    'RegisterComposition': ('MUTATOR', 'save_game_tests.cpp', 'Check(bool(orchestrator.RegisterComposition(participants, migrations, expected))'),
    'Bind': ('MUTATOR', 'save_participant_mechanics_tests.cpp', 'Check(duplicate_rejected,'),
    'Require': ('QUERY', 'save_participant_mechanics_tests.cpp', 'Check(&workspace.Require<ValueOwner>() == &live &&'),
    'Resolve': ('QUERY', 'save_participant_mechanics_tests.cpp', 'Check(workspace.Require<ValueOwner>().state.revision'),
    'Capture': ('QUERY', 'save_participant_mechanics_tests.cpp', 'Check(bool(captured),'),
    'Clone': ('FACTORY', 'save_participant_mechanics_tests.cpp', 'Check(bool(stage) && live.state.revision'),
    'Restore': ('MUTATOR', 'save_participant_mechanics_tests.cpp', 'Check(!participant.ValidateSnapshot(semantic,'),
    'View': ('QUERY', 'save_participant_mechanics_tests.cpp', 'Check(workspace.Require<ValueOwner>().state.revision'),
    'Commit': ('MUTATOR', 'save_participant_mechanics_tests.cpp', 'Check(live.state.revision == 27'),
    'SwapPreparedMember': ('MUTATOR', 'save_participant_mechanics_tests.cpp', 'Check(published.size() == 2'),
    'operatorT': ('QUERY', 'save_participant_mechanics_tests.cpp', 'static_assert(savegame::detail::AggregateAccepts<ValueSnapshot>'),
    'AggregateAccepts': ('QUERY', 'save_participant_mechanics_tests.cpp', 'static_assert(savegame::detail::AggregateAccepts<ValueSnapshot>'),
    'VisitSnapshotFields': ('MUTATOR', 'save_participant_mechanics_tests.cpp', 'Check(decoded.revision == 27'),
    'Byte': ('MUTATOR', 'save_participant_mechanics_tests.cpp', 'Check(byte == 42'),
    'Remaining': ('QUERY', 'save_participant_mechanics_tests.cpp', 'Check(byte == 42'),
    'SnapshotReader': ('CONSTRUCTOR', 'save_participant_mechanics_tests.cpp', 'Check(byte == 42'),
    'TransferSnapshot': ('MUTATOR', 'save_participant_mechanics_tests.cpp', 'Check(bounded && values.empty()'),
    'DecodeParticipantSnapshot': ('QUERY', 'save_participant_mechanics_tests.cpp', 'Check(decoded.revision == 27'),
    'SnapshotParticipant': ('CONSTRUCTOR', 'save_participant_mechanics_tests.cpp', 'participant_tests::CheckParticipantContract(participant,'),
    'Id': ('QUERY', 'participant_test_contract.h', 'Check(participant.Id()'),
    'SchemaVersion': ('QUERY', 'participant_test_contract.h', 'Check(participant.SchemaVersion()'),
    'Dependencies': ('QUERY', 'participant_test_contract.h', 'Check(participant.Dependencies()'),
    'CaptureSnapshot': ('QUERY', 'save_participant_mechanics_tests.cpp', 'Check(bool(captured),'),
    'ValidateSnapshot': ('QUERY', 'save_participant_mechanics_tests.cpp', 'Check(!participant.ValidateSnapshot(semantic,'),
    'StageRestore': ('MUTATOR', 'save_participant_mechanics_tests.cpp', 'Check(bool(stage) && live.state.revision'),
    'CommitRestore': ('MUTATOR', 'save_participant_mechanics_tests.cpp', 'Check(live.state.revision == 27'),
}

# Finite M5.2.C-E review: no generic acceptance for unknown added callables.
WIRE_STORAGE = {
    ('EngineRuntime/Persistence','epidemic::runtime','ValidatePersistenceSnapshot'):
        ('QUERY','EngineRuntime/Support/tests/runtime_package_tests.cpp','EpidemicRuntimeSupportTests','"Runtime duplicate snapshot rejected"','Expose the existing semantic validation used by service composition to durable backend adapters; rejects invalid identity/revision/enum/zone relationships before atomic I/O.'),
    ('EngineRuntime/Support','epidemic::runtime','CommitRuntimeTimeCheckpoint'):
        ('MUTATOR','EngineRuntime/Support/tests/runtime_package_tests.cpp','EpidemicRuntimeSupportTests','"Runtime invalid clock commit preserves old generation"','Validate the complete Runtime continuation including fractional remainder/configuration, encode explicit bounded v1 fields, then Base atomic byte replacement. Independent plane only; causal generation is M5.3.'),
    ('EngineRuntime/Support','epidemic::runtime','LoadRuntimeTimeCheckpoint'):
        ('QUERY','EngineRuntime/Support/tests/runtime_package_tests.cpp','EpidemicRuntimeSupportTests','"Runtime Time persisted fractional remainder"','Bounded v1 decode/checksum/configuration validation returns a value checkpoint, without mutating any live clock; immutable golden and fresh-clock restore regressions.'),
    ('EngineRuntime/Support','epidemic::runtime','CreateFilePersistenceBackend'):
        ('FACTORY','EngineRuntime/Support/tests/runtime_package_tests.cpp','EpidemicRuntimeSupportTests','"Runtime missing backend file requires explicit initialization"','Create a path-only IPersistenceBackend adapter with no second authoritative state cache. Explicit initialization, semantic and wire validation, canonical all-record encoding, strongest atomic file durability and fresh store reopen.'),
    ('EngineBase/Platform','epidemic::platform','ReadFilePackage'):
        ('QUERY','Tests/Truth/persistence_wire_tests.cpp','EpidemicPersistenceWireTruthTests','"Base bounded byte read returns exact package"','Bounded neutral byte read; reject missing/oversized/changed file before exposing incomplete bytes; no gameplay dependency.'),
    ('EngineBase/Platform','epidemic::platform','CommitFilePackage'):
        ('MUTATOR','Tests/Truth/persistence_wire_tests.cpp','EpidemicPersistenceWireTruthTests','"Base identical byte package commit preserves exact content"','Externally synchronized byte commit, same-directory CREATE_NEW temp, full write/flush/close then atomic replacement; handled failure preserves old file and cleans only owned temp.'),
    ('EngineFramework/GameplayWorldStateOwners/SaveGame','epidemic::gameplay::savegame','EncodeSaveGameImage'):
        ('QUERY','EngineFramework/DevelopmentInfrastructure/Tests/save_wire_tests.cpp','EpidemicGameFrameworkSaveGameTests','"canonical re-encoding is byte-stable"','Canonical field-wise LE v1 encoding, bounded validated manifest/sections and whole checksum; never native object layout.'),
    ('EngineFramework/GameplayWorldStateOwners/SaveGame','epidemic::gameplay::savegame','DecodeSaveGameImage'):
        ('QUERY','EngineFramework/DevelopmentInfrastructure/Tests/save_wire_tests.cpp','EpidemicGameFrameworkSaveGameTests','"historical wire golden decodes and canonically re-encodes"','Fail-closed field-wise decode validates magic/version/counts/lengths/IDs/schema/dependencies/checksums/order/trailing bytes before exposing a complete image.'),
    ('EngineFramework/GameplayWorldStateOwners/SaveGame','epidemic::gameplay::savegame','CommitSaveGameImage'):
        ('MUTATOR','Tests/Truth/persistence_wire_tests.cpp','EpidemicPersistenceWireTruthTests','"invalid image never reaches store commit"','Encode and validate before atomic store Commit; preserve old package on encode/backend failure, convert backend exceptions.'),
    ('EngineFramework/GameplayWorldStateOwners/SaveGame','epidemic::gameplay::savegame','LoadSaveGameImage'):
        ('QUERY','Tests/Truth/persistence_wire_tests.cpp','EpidemicPersistenceWireTruthTests','"stored corruption rejected"','Load bounded complete bytes and decode before any caller restore; storage/structural failure never publishes gameplay state.'),
    ('EngineFramework/GameplayWorldStateOwners/SaveGame','epidemic::gameplay::savegame::ISavePackageStore','Load'):
        ('QUERY','Tests/Truth/persistence_wire_tests.cpp','EpidemicPersistenceWireTruthTests','"stored truncation rejected"','Semantic package read protocol, implemented by deterministic fault backend and production filesystem adapter.'),
    ('EngineFramework/GameplayWorldStateOwners/SaveGame','epidemic::gameplay::savegame::ISavePackageStore','Commit'):
        ('MUTATOR','Tests/Truth/persistence_wire_tests.cpp','EpidemicPersistenceWireTruthTests','"controlled memory failure preserves old generation"','Complete atomic package replacement protocol: failure leaves old package authoritative; no OS types in SaveGame.'),
    ('EngineFramework/GameplayWorldStateOwners/SaveGame','epidemic::gameplay::savegame::ISavePackageStore','~ISavePackageStore'):
        ('DESTRUCTOR','Tests/Truth/persistence_wire_tests.cpp','EpidemicPersistenceWireTruthTests','"virtual store destruction never removes committed save"','Virtual noexcept destruction of owned backend; durable committed bytes are not deleted by object destruction.'),
    ('EngineFramework/IntegrationLayer/Integration','epidemic::gameplay::integration::FileSavePackageStore','FileSavePackageStore'):
        ('CONSTRUCTOR','Tests/Truth/persistence_wire_tests.cpp','EpidemicPersistenceWireTruthTests','"embedded NUL path rejected before filesystem access"','Store validated native path/resource policy; no I/O until Load/Commit; embedded NUL is rejected.'),
    ('EngineFramework/IntegrationLayer/Integration','epidemic::gameplay::integration::FileSavePackageStore','Load'):
        ('QUERY','Tests/Truth/persistence_wire_tests.cpp','EpidemicPersistenceWireTruthTests','"filesystem encode and commit"','Read via bounded Base Platform byte port, no Windows include in Integration and no live semantic state mutation.'),
    ('EngineFramework/IntegrationLayer/Integration','epidemic::gameplay::integration::FileSavePackageStore','Commit'):
        ('MUTATOR','Tests/Truth/persistence_wire_tests.cpp','EpidemicPersistenceWireTruthTests','"filesystem next generation replaces old"','Validate complete wire bytes before Base Platform atomic byte commit. Single writer per path, file flush/write-through, no hardware power-loss guarantee.')
}

def read(path): return json.loads(path.read_text(encoding='utf-8'))
def write(path, data):
    content = json.dumps(data, indent=2, sort_keys=True) + '\n'
    if path.is_file() and path.read_text(encoding='utf-8') == content: return
    with tempfile.NamedTemporaryFile(mode='w', encoding='utf-8', newline='\n', dir=path.parent, delete=False) as output:
        output.write(content)
        temporary = output.name
    os.replace(temporary, path)
def anchor(path, token):
    lines = (ROOT / path).read_text(encoding='utf-8').splitlines()
    hits = [i + 1 for i, line in enumerate(lines) if token in line]
    if len(hits) != 1: raise ValueError(f'non-unique evidence: {path}: {token}: {hits}')
    return f'{path}:{hits[0]}::{token}'

def rebase_source(value, field=''):
    if isinstance(value, dict): return {k:rebase_source(v,field if field == 'regressions' else k) for k,v in value.items()}
    if isinstance(value, list): return [rebase_source(v,field) for v in value]
    if field not in {'anchor','assertions','regression','regressions','review_evidence'}: return value
    if not isinstance(value, str): return value
    match = re.fullmatch(r'([^\s:]+):(\d+)::(.+)', value)
    if not match: return value
    path, old_line, symbol = match.groups()
    if not (ROOT / path).is_file(): return value
    lines = (ROOT / path).read_text(encoding='utf-8').splitlines()
    old_line = int(old_line)
    if 0 < old_line <= len(lines) and symbol in lines[old_line-1]: return value
    hits = [i+1 for i,line in enumerate(lines) if symbol in line]
    if not hits: raise ValueError(f'reviewed source anchor lost its symbol: {value}')
    closest = min(abs(line-old_line) for line in hits)
    hits = [line for line in hits if abs(line-old_line) == closest]
    if len(hits) != 1: raise ValueError(f'ambiguous source anchor: {value}')
    return f'{path}:{hits[0]}::{symbol}'

def main():
    parser = argparse.ArgumentParser(description=__doc__); parser.add_argument('--record', action='store_true')
    if not parser.parse_args().record: parser.error('explicit --record required')
    rows = scan_modules(discover_modules())
    previous = read(ROOT / 'docs/freeze/coverage_manifests.json')['api']
    delta_path = ROOT / 'docs/freeze/goal5_admission_delta.json'
    old_delta = read(delta_path) if delta_path.is_file() else None
    baseline = set(old_delta['baseline_api_ids']) if old_delta else set(previous)
    added = [r for r in rows if r.id not in baseline]
    if baseline - {r.id for r in rows}: raise ValueError('unreviewed deletion from admission API surface')
    factories = {'Create' + b['owner'] + 'SaveParticipant': b for b in bindings()}
    overrides_path = ROOT / 'docs/freeze/public_api_classification_overrides.json'
    overrides = read(overrides_path)
    removed = set(previous) - {r.id for r in rows}
    private_scopes = {
        'epidemic::gameplay::SnapshotRestoreAccess', 'epidemic::gameplay::savegame::SaveRestoreWorkspace',
        'epidemic::gameplay::savegame::detail', 'epidemic::gameplay::savegame::detail::AggregateArgument',
        'epidemic::gameplay::savegame::detail::SnapshotParticipant', 'epidemic::gameplay::savegame::detail::SnapshotWriter',
        'epidemic::gameplay::savegame::detail::SnapshotReader'}
    allowed_removed = {key for key, row in previous.items() if key not in baseline and row['scope'] in private_scopes}
    if removed - allowed_removed: raise ValueError('removal outside reviewed M5.2 technical API delta')
    if any(previous[key]['module'] != 'EngineFramework/GameplayWorldStateOwners/SaveGame' for key in removed):
        raise ValueError('only SaveGame technical mechanisms may be privatized by this review')
    for key in removed: overrides['overrides'].pop(key, None)
    blocks = {}
    supplemental = {kind:{} for kind in ('api','anchors','lifecycle','stale_identity','external_boundary')}
    for folder in sorted((ROOT / '_goal4_handoff').glob('B[0-9][0-9]')):
        coverage = read(folder / 'coverage_reviews.json'); anchors = read(folder / 'public_api_anchors.json')
        blocks[folder] = (coverage, anchors, {v['module'] for v in coverage['api'].values()})
        for key in removed:
            coverage['api'].pop(key, None); anchors['anchors'].pop(key, None)
        for kind in ('stale_identity', 'external_boundary'):
            coverage[kind] = {k:v for k,v in coverage.get(kind,{}).items() if v.get('callable_id') not in removed}
    docs = ['# M5.2 reviewed persistence APIs', '',
            'Concrete owner bindings stay in the existing integration module. SaveGame depends on no gameplay owner.',
            'Payload v1 transfers snapshot value fields, not object layout. Definition tables present in a DTO remain durable; callbacks and registrations absent from that DTO remain bootstrap configuration.',
            'Stage validates privately; failures and abandoned stages leave live state intact. Commit is a one-use noexcept publication.',
            'RegisterComposition requires an empty registry, an exact reviewed set and valid migrations/graph; successful bootstrap freezes registration.',
            'M5.2.C-E add explicit v1-only schema policies, immutable fixtures, canonical whole-image wire and a bounded package-store contract. Native file operations stay in Base Platform; Integration adapts bytes to SaveGame. Consistent-cut barriers and whole-engine reconciliation remain M5.3/M5.4.', '',
            '| API | Contract and test |', '| --- | --- |']
    for row in added:
        wire_spec = WIRE_STORAGE.get((row.module,row.scope,row.name))
        if wire_spec:
            classification, source, target, token, rationale = wire_spec
            test = anchor(source,token); assertions=[test]
            if classification == 'MUTATOR':
                if row.module.startswith('EngineRuntime/'):
                    assertions += [anchor('EngineRuntime/Support/tests/runtime_package_tests.cpp',t) for t in ('"Runtime native commit failure reported"','"Runtime storage failure preserves both old independent packages"')]
                else: assertions += [anchor('Tests/Truth/persistence_wire_tests.cpp',t) for t in (
                    '"filesystem failure preserves old save and informative error"',
                    '"handled failure cleans only its own temporary file"',
                    '"Base oversized commit preserves old file"')]
        elif row.name in factories:
            b = factories[row.name]
            test_file = b['placement'].split('/')[-1].lower() + '_save_participant_tests.cpp'
            test = anchor('EngineFramework/DevelopmentInfrastructure/Tests/' + test_file, f'auto participant = savegame::{row.name}(')
            target = 'Epidemic' + b['placement'].split('/')[-1] + 'SaveParticipantTests'
            classification = 'FACTORY'
            rationale = f"Construct {b['id']} with schema 1, Required semantics and exact semantic dependencies {b['dependencies']}; authoritative state remains {b['owner']}."
            assertions = [anchor('EngineFramework/DevelopmentInfrastructure/Tests/participant_test_contract.h', token)
                          for token in ['Check(participant.Id()', 'Check(participant.Dependencies()', 'Check(!participant.ValidateSnapshot(corrupt, {}), "wrong participant',
                                        'Check(bool(live) && live.Value().payload == section.payload, "allocation failure',
                                        'Check(bool(repeated) && repeated.Value().payload == published.Value().payload']]
        elif row.module.endswith('/SaveGame') and row.name in MECHANICS:
            classification, file, token = MECHANICS[row.name]
            test = anchor('EngineFramework/DevelopmentInfrastructure/Tests/' + file, token)
            target = 'EpidemicGameFrameworkSaveGameTests'
            rationale = f'{row.scope}::{row.name}: neutral typed participant mechanics; tested value encoding, metadata closure, private staging and one-use publication, with no gameplay dependency.'
            assertions = [test]
            if classification == 'MUTATOR':
                proof_file = 'save_game_tests.cpp' if row.name == 'RegisterComposition' else 'save_participant_mechanics_tests.cpp'
                tokens = ['"failed composition leaves registry empty and unfrozen"', '"late registration rejected"'] if row.name == 'RegisterComposition' else ['Check(!participant.StageRestore(captured.Value(), {})', 'Check(!participant.ValidateSnapshot(semantic,', 'Check(live.state.revision == 99,']
                assertions += [anchor('EngineFramework/DevelopmentInfrastructure/Tests/' + proof_file, t) for t in tokens]
        else: raise ValueError(f'unreviewed added callable: {row.id} {row.scope}::{row.name}')
        if classification != row.classification:
            overrides['overrides'][row.id] = {'classification':classification, 'rationale':rationale, 'reviewed':True}
        docs.append(f'| `{row.id}` `{row.name}` | {rationale} |')
        contract = f'docs/EngineFramework/goal5_participants.md:{len(docs)}::`{row.id}`'
        matches = [(folder, coverage, anchors) for folder, (coverage, anchors, modules) in blocks.items() if row.module in modules]
        if row.module in {'EngineBase/Platform','EngineRuntime/Support','EngineRuntime/Persistence'} and wire_spec:
            coverage, anchors = supplemental, {'anchors':supplemental['anchors']}
        else:
            if len(matches) != 1: raise ValueError(f'owner handoff ambiguous: {row.module}')
            _, coverage, anchors = matches[0]
        anchors['anchors'][row.id] = {'contract':{'anchor':contract,'reviewed':True}, 'test':{'anchor':test,'assertions':assertions,'target':target,'reviewed':True}}
        coverage['api'][row.id] = {'module':row.module, 'header':row.header, 'scope':row.scope, 'signature':row.signature,
                                 'classification':classification, 'classification_status':'REVIEWED',
                                 'obligations':{k:'REVIEWED' for k in ('preconditions','success','failure','noop')} if classification in {'MUTATOR','LIFECYCLE'} else {},
                                 'review_evidence':test, 'review_rationale':rationale}
        if classification in {'MUTATOR','LIFECYCLE'} and any(t in row.signature.lower() for t in ('id','handle','ref')):
            identity = surface_id('stale_identity', row.module, row.id)
            coverage['stale_identity'][identity] = {'module':row.module,'callable_id':row.id,'status':'REVIEWED','review_evidence':test,'review_rationale':rationale}
        if wire_spec and any(t in row.signature.lower() for t in ('callback','backend','port','sink')):
            identity = surface_id('external_boundary',row.module,row.id)
            coverage['external_boundary'][identity] = {'module':row.module,'callable_id':row.id,'status':'REVIEWED','review_evidence':test,'review_rationale':rationale+' Failure returns Result; adapters invoke only bounded Base byte operations after validation.'}
    (ROOT / 'docs/EngineFramework/goal5_participants.md').write_text('\n'.join(docs) + '\n', encoding='utf-8', newline='\n')
    write(overrides_path, overrides)
    write(ROOT / '_goal5_handoff/base_public_surface_delta.json',
          {'schema_version':1,'reviewed':True,**supplemental})
    for folder, (coverage, anchors, _) in blocks.items():
        write(folder / 'coverage_reviews.json', rebase_source(coverage)); write(folder / 'public_api_anchors.json', rebase_source(anchors))
        for path in folder.glob('*.json'):
            source = read(path); rebased = rebase_source(source)
            if source != rebased: write(path, rebased)
    admission = read(ROOT / 'docs/freeze/goal5_admission.json')
    write(delta_path, {'schema_version':1,'reviewed':True,'stage':'M5.2.A-E','baseline_source_sha':admission['source_sha'],
                       'baseline_api_ids':sorted(baseline),'added_api_ids':sorted(r.id for r in added),
                       'privatized_api_ids':sorted(removed | set(old_delta.get('privatized_api_ids', [])))})
    print(f'PASS: recorded {len(added)} explicitly mapped API reviews into raw handoff sources; canonical merge remains separate.')
if __name__ == '__main__': raise SystemExit(main())
