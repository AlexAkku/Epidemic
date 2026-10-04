"""Owning schema generator and fail-closed read-only M5.2 schema/fixture validator."""
from __future__ import annotations
import argparse
import copy
import hashlib
import json
import sys
from pathlib import Path
sys.dont_write_bytecode = True
from module_dossiers import ROOT
from goal5_persistence_contract import load_json, encoded, ctest_targets

FREEZE = ROOT / 'docs/freeze'
REVIEW = FREEZE / 'goal5_schema_review.json'
CONTRACT = FREEZE / 'goal5_schema_contract.json'
FIXTURES = ROOT / 'Tests/Fixtures/SaveGame'
PROFILES = {'ExactRequired', 'CompatibleVersion', 'MigrationRequired', 'BestEffort', 'DeveloperMode'}
STRICT = {'ExactRequired', 'CompatibleVersion', 'MigrationRequired'}

def generate(review, graph):
    identities = review['participant_ids']
    if review.get('reviewed') is not True or len(identities) != len(set(identities)):
        raise ValueError('schema review must be explicit and duplicate-free')
    if set(identities) != {p['id'] for p in graph['participants']}:
        raise ValueError('schema review does not match the exact participant set')
    records = []
    for p in sorted(graph['participants'], key=lambda p:p['id']):
        policy = copy.deepcopy(review['initial_participant_policy'])
        if p['schema_version'] != policy['current_version']:
            raise ValueError('changed participant schema requires a separate explicit policy review')
        policy.update(participant_id=p['id'], fixture_ids=[p['id'] + ':v' + str(v) for v in policy['supported_versions']])
        records.append(policy)
    return {'schema_version':1, 'participants':records, 'wire_format':review['wire_format'],
            'storage':review['storage'], 'evidence':{
                'schema_profiles':{'target':'EpidemicGameFrameworkSaveGameTests', 'source':'EngineFramework/DevelopmentInfrastructure/Tests/save_game_tests.cpp', 'symbol':'TestCompatibilityProfileMatrix'},
                'wire':{'target':'EpidemicGameFrameworkSaveGameTests', 'source':'EngineFramework/DevelopmentInfrastructure/Tests/save_wire_tests.cpp', 'symbol':'TestSaveWireCodec'},
                'storage':{'target':'EpidemicPersistenceWireTruthTests', 'source':'Tests/Truth/persistence_wire_tests.cpp', 'symbol':'TestDurableStorage'},
                'reopen':{'target':'EpidemicPersistenceProcessTruthTests', 'source':'tools/ci/test_save_process.py', 'symbol':'fresh process'} }}

def validate(contract, graph, fixture_manifest, verify_files=True):
    errors = []
    if not isinstance(contract, dict) or contract.get('schema_version') != 1: return ['invalid schema contract']
    records = contract.get('participants')
    if not isinstance(records, list): return ['missing participant schemas']
    expected = {p['id']:p['schema_version'] for p in graph['participants']}
    ids = [r.get('participant_id') for r in records if isinstance(r,dict)]
    if len(ids) != len(records) or len(ids) != len(set(ids)) or set(ids) != set(expected): errors.append('participant schema coverage is not exact')
    fixtures = fixture_manifest.get('fixtures', [])
    fixture_ids = [f.get('fixture_id') for f in fixtures if isinstance(f,dict)]
    if len(fixture_ids) != len(fixtures) or len(fixture_ids) != len(set(fixture_ids)): errors.append('duplicate/invalid fixture identity')
    by_fixture = {f['fixture_id']:f for f in fixtures if isinstance(f,dict) and 'fixture_id' in f}
    required_fixtures = {'whole:1:empty','whole:1:multi'}
    for r in records:
        if not isinstance(r, dict): continue
        pid = r.get('participant_id'); current = r.get('current_version'); minimum = r.get('minimum_supported_version')
        versions = r.get('supported_versions', [])
        if type(current) is not int or current < 1 or expected.get(pid) != current: errors.append(f'{pid}: invalid current schema')
        if type(minimum) is not int or minimum < 1 or not isinstance(versions,list) or any(type(v) is not int or v<1 for v in versions):
            errors.append(f'{pid}: invalid supported range'); continue
        if type(current) is not int: continue
        if current < minimum or versions != list(range(minimum,current+1)): errors.append(f'{pid}: supported range incoherent')
        edges = r.get('migration_edges')
        routes = {}
        if not isinstance(edges,list): errors.append(f'{pid}: invalid migrations'); edges=[]
        for edge in edges:
            if not isinstance(edge,dict): errors.append(f'{pid}: malformed migration'); continue
            source = edge.get('from'); target = edge.get('to')
            if type(source) is not int or type(target) is not int or source not in versions or target not in versions or source >= target or source in routes:
                errors.append(f'{pid}: invalid/cyclic/duplicate migration edge'); continue
            routes[source] = target
        for version in versions:
            seen=set(); cursor=version
            while cursor != current:
                if cursor in seen or cursor not in routes: errors.append(f'{pid}: migration gap/cycle'); break
                seen.add(cursor); cursor=routes[cursor]
        if versions == [1]:
            if r.get('migration_policy') != 'NO_PRIOR_SUPPORTED_VERSION' or edges: errors.append(f'{pid}: version one policy must not invent history')
        elif r.get('migration_policy') != 'EXPLICIT_MIGRATION_CHAIN': errors.append(f'{pid}: historical support requires migration policy')
        needed = [str(pid)+':v'+str(v) for v in versions]
        if r.get('fixture_ids') != needed: errors.append(f'{pid}: historical fixture coverage missing')
        required_fixtures.update(needed)
        profiles = r.get('compatibility_profiles',{})
        if not isinstance(profiles,dict) or set(profiles) != PROFILES: errors.append(f'{pid}: profile policy not explicit'); continue
        for name, policy in profiles.items():
            strict = name in STRICT
            future = 'REJECT' if strict else 'REJECT_REQUIRED_SKIP_OPTIONAL'
            if not isinstance(policy,dict) or policy.get('strict') is not strict or policy.get('future_version') != future:
                errors.append(f'{pid}: future/permissive policy invalid for {name}')
    if set(by_fixture) != required_fixtures: errors.append('fixture manifest set is not exact')
    if fixture_manifest.get('schema_version') != 1: errors.append('invalid fixture manifest version')
    for fid, f in by_fixture.items():
        relative = f.get('path','')
        path = (FIXTURES / relative).resolve()
        if not relative or not path.is_relative_to(FIXTURES.resolve()) or Path(relative).is_absolute(): errors.append(f'{fid}: invalid fixture path'); continue
        if not isinstance(f.get('sha256'),str) or len(f['sha256']) != 64 or type(f.get('size')) is not int or f['size'] <= 0:
            errors.append(f'{fid}: invalid fixture fingerprint'); continue
        if verify_files and (not path.is_file() or path.stat().st_size != f['size'] or hashlib.sha256(path.read_bytes()).hexdigest() != f['sha256']):
            errors.append(f'{fid}: historical fixture missing or changed')
        if fid.startswith('whole:'):
            if f.get('wire_version') != 1: errors.append(f'{fid}: incorrect wire fixture metadata')
        else:
            pid, version = fid.rsplit(':v',1)
            if f.get('participant_id') != pid or f.get('version') != int(version): errors.append(f'{fid}: fixture metadata mismatched')
    wire = contract.get('wire_format',{})
    if wire.get('magic') != 'EPISAVE1' or wire.get('wire_version') != 1 or wire.get('save_format_version') != 1 or wire.get('trailing_bytes') != 'REJECT': errors.append('wire format policy invalid')
    limits = wire.get('limits',{})
    for key in ('max_participants','max_dependencies_per_participant','max_section_bytes','max_total_payload_bytes','max_wire_bytes'):
        if type(limits.get(key)) is not int or limits[key] <= 0: errors.append('wire resource limit invalid: '+key)
    if verify_files:
        targets = ctest_targets()
        for name,e in contract.get('evidence',{}).items():
            path = ROOT / e.get('source','')
            if e.get('target') not in targets or not path.is_file() or e.get('symbol','!missing!') not in path.read_text(encoding='utf-8'):
                errors.append('stale schema execution evidence: '+name)
    return errors

def self_test():
    profiles = {n:{'strict':n in STRICT,'future_version':'REJECT' if n in STRICT else 'REJECT_REQUIRED_SKIP_OPTIONAL'} for n in PROFILES}
    graph = {'participants':[{'id':'fixture','schema_version':2}]}
    base = {'schema_version':1, 'participants':[{'participant_id':'fixture','current_version':2,'minimum_supported_version':1,
        'supported_versions':[1,2],'migration_edges':[{'from':1,'to':2}], 'migration_policy':'EXPLICIT_MIGRATION_CHAIN',
        'fixture_ids':['fixture:v1','fixture:v2'],'compatibility_profiles':profiles}], 'wire_format':load_json(REVIEW)['wire_format']}
    fixtures = {'schema_version':1,'fixtures':[{'fixture_id':fid,'path':fid.replace(':','_')+'.bin','sha256':'a'*64,'size':1,
        **({'wire_version':1} if fid.startswith('whole:') else {'participant_id':'fixture','version':int(fid[-1])})}
        for fid in ('fixture:v1','fixture:v2','whole:1:empty','whole:1:multi')]}
    assert not validate(base,graph,fixtures,False)
    mutations = [lambda c:c['participants'].clear(), lambda c:c['participants'].append(copy.deepcopy(c['participants'][0])),
        lambda c:c['participants'][0].update(current_version=0), lambda c:c['participants'][0].update(supported_versions=[2]),
        lambda c:c['participants'][0].update(migration_edges=[]), lambda c:c['participants'][0].update(migration_edges=[{'from':2,'to':1}]),
        lambda c:c['participants'][0].update(fixture_ids=[]), lambda c:c['participants'][0].update(migration_policy='NO_PRIOR_SUPPORTED_VERSION'),
        lambda c:c['participants'][0]['compatibility_profiles'].pop('ExactRequired'),
        lambda c:c['participants'][0]['compatibility_profiles']['ExactRequired'].update(future_version='ALLOW'),
        lambda c:c['participants'][0]['compatibility_profiles']['BestEffort'].update(strict=True),
        lambda c:c['wire_format'].update(trailing_bytes='IGNORE'), lambda c:c['wire_format']['limits'].update(max_wire_bytes=0)]
    for mutate in mutations:
        bad=copy.deepcopy(base); mutate(bad); assert validate(bad,graph,fixtures,False)
    for key,value in [('fixtures',fixtures['fixtures'][:-1]),('schema_version',0)]:
        bad=copy.deepcopy(fixtures); bad[key]=value; assert validate(base,graph,bad,False)
    bad=copy.deepcopy(fixtures); bad['fixtures'][0]['path']='../escape.bin'; assert validate(base,graph,bad,False)
    one=copy.deepcopy(base); one['participants'][0].update(current_version=1,supported_versions=[1],migration_edges=[],migration_policy='NO_PRIOR_SUPPORTED_VERSION',fixture_ids=['fixture:v1'])
    one_graph={'participants':[{'id':'fixture','schema_version':1}]}; one_fixtures=copy.deepcopy(fixtures); one_fixtures['fixtures'].pop(1)
    assert not validate(one,one_graph,one_fixtures,False)
    print('PASS: v1-only and historical chain positives; 16 schema/profile/fixture negatives rejected.')

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for flag in ('record','check','self-test'): parser.add_argument('--'+flag,action='store_true')
    args=parser.parse_args()
    if args.self_test: self_test(); return 0
    if not args.record and not args.check: parser.error('choose --record, --check or --self-test')
    try:
        graph=load_json(FREEZE/'goal5_participant_contract.json'); expected=generate(load_json(REVIEW),graph)
        if args.record: CONTRACT.write_text(encoded(expected),encoding='utf-8',newline='\n')
        if args.check:
            if not CONTRACT.is_file() or CONTRACT.read_text(encoding='utf-8') != encoded(expected): raise ValueError('schema contract is missing/stale')
            errors=validate(expected,graph,load_json(FIXTURES/'manifest.json'))
            if errors: raise ValueError('\n'.join(errors))
        print('PASS: explicit schemas for all 47 participants; check is read-only; no prior version fabricated.')
    except (OSError,ValueError,KeyError,TypeError) as error:
        print('BLOCKED: '+str(error),file=sys.stderr); return 1
    return 0
if __name__ == '__main__': raise SystemExit(main())
