"""Read-only stage 5.1/5.2 exit gate; later-stage debt is explicit, never erased."""
import argparse
import copy
import sys
sys.dont_write_bytecode=True
from module_dossiers import ROOT
from goal5_persistence_contract import load_json,gap_report

def validate(persistence,graph,schemas,gaps,manifest):
    errors=[]
    candidates=persistence.get('candidates',{})
    participants={p['id']:p for p in graph.get('participants',[])}
    versions={p['participant_id']:p['current_version'] for p in schemas.get('participants',[])}
    for identity,c in candidates.items():
        if c.get('review_status')!='REVIEWED': errors.append(identity+': unreviewed persistence candidate')
        if c.get('persistence_role') in {'PERSISTENT_OWNER','PERSISTENT_CHECKPOINT'}:
            if not c.get('durable_storage') or str(c['durable_storage']).startswith('GAP:'): errors.append(identity+': durable storage not closed (all save planes)')
            if not c.get('durable_schema') or str(c['durable_schema']).startswith('GAP:'): errors.append(identity+': durable schema not closed (all save planes)')
            if c.get('save_plane')=='RUNTIME_PERSISTENCE' and (c.get('migration_policy')!='NO_PRIOR_SUPPORTED_VERSION' or not str(c.get('durable_schema','')).endswith(':v1')): errors.append(identity+': Runtime initial schema policy not closed')
        if c.get('save_plane')!='FRAMEWORK_SAVEGAME' or c.get('persistence_role') not in {'PERSISTENT_OWNER','PERSISTENT_CHECKPOINT'}: continue
        pid=c.get('participant_id')
        if pid not in participants or pid not in versions: errors.append(identity+': schema/participant mapping missing'); continue
        if c.get('durable_schema')!=pid+':v'+str(versions[pid]) or c.get('migration_policy')!='NO_PRIOR_SUPPORTED_VERSION': errors.append(identity+': initial schema policy not closed')
        if not c.get('durable_storage') or str(c['durable_storage']).startswith('GAP:'): errors.append(identity+': Framework durable storage not closed')
    if gaps!=gap_report(persistence): errors.append('stale gap report')
    if any(g.get('target_stage')=='5.2' for g in gaps.get('gaps',[])): errors.append('unresolved stage 5.2 debt')
    for name in ('full-debug','full-release'):
        tests={t['name']:set(t.get('labels',[])) for t in manifest.get('profiles',{}).get(name,{}).get('tests',[])}
        for target in ('EpidemicPersistenceWireTruthTests','EpidemicPersistenceProcessTruthTests'):
            if not {'truth','persistence-wire','persistence-storage'} <= tests.get(target,set()): errors.append(name+': storage/process suite not registered with exact labels')
        if 'persistence-wire' not in tests.get('EpidemicGameFrameworkSaveGameTests',set()): errors.append(name+': codec unit suite not labelled')
        if 'persistence-schema' not in tests.get('EpidemicFrameworkSaveCompositionTests',set()): errors.append(name+': actual participant fixtures not labelled')
        if not {'persistence-schema','persistence-wire','persistence-storage'} <= tests.get('EpidemicRuntimeSupportTests',set()): errors.append(name+': Runtime durable suite not labelled')
    return errors

def self_test():
    persistence={'candidates':{'c':{'module':'EngineFramework/Fake','review_status':'REVIEWED','save_plane':'FRAMEWORK_SAVEGAME','persistence_role':'PERSISTENT_OWNER',
        'participant_id':'p','durable_schema':'p:v1','migration_policy':'NO_PRIOR_SUPPORTED_VERSION','durable_storage':'atomic byte store',
        'dependencies':[],'reconciliation':'qualified','digest_policy':'INCLUDE','evidence':[{'kind':'executable'}]}}}
    graph={'participants':[{'id':'p'}]}; schemas={'participants':[{'participant_id':'p','current_version':1}]}
    tests=[{'name':n,'labels':['truth','persistence-wire','persistence-storage']} for n in ('EpidemicPersistenceWireTruthTests','EpidemicPersistenceProcessTruthTests')]
    tests += [{'name':'EpidemicGameFrameworkSaveGameTests','labels':['persistence-wire']},{'name':'EpidemicFrameworkSaveCompositionTests','labels':['persistence-schema']}]
    tests += [{'name':'EpidemicRuntimeSupportTests','labels':['persistence-schema','persistence-wire','persistence-storage']}]
    manifest={'profiles':{n:{'tests':tests} for n in ('full-debug','full-release')}}
    gaps=gap_report(persistence)
    assert not validate(persistence,graph,schemas,gaps,manifest)
    for field,value in [('review_status','DISCOVERED'),('participant_id','missing'),('durable_schema','GAP:SCHEMA'),('migration_policy',''),('durable_storage','GAP:STORE')]:
        bad=copy.deepcopy(persistence); bad['candidates']['c'][field]=value
        assert validate(bad,graph,schemas,gap_report(bad),manifest)
    bad=copy.deepcopy(manifest); bad['profiles']['full-debug']['tests']=[]; assert validate(persistence,graph,schemas,gaps,bad)
    bad=copy.deepcopy(gaps); bad['gaps'].append({'target_stage':'5.2'}); assert validate(persistence,graph,schemas,bad,manifest)
    bad=copy.deepcopy(persistence); bad['candidates']['c'].update(save_plane='RUNTIME_PERSISTENCE',durable_storage='GAP:5.3_NOT_A_DURABLE_PATH'); assert validate(bad,graph,schemas,gap_report(bad),manifest)
    print('PASS: stage 5.1/5.2 positive and eight missing-review/schema/store/evidence/debt negatives, including Runtime durable-path gaps disguised as coordination.')

def main():
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument('--check',action='store_true'); parser.add_argument('--self-test',action='store_true'); args=parser.parse_args()
    if args.self_test: self_test(); return 0
    if not args.check: parser.error('choose --check or --self-test')
    try:
        freeze=ROOT/'docs/freeze'
        errors=validate(*(load_json(freeze/name) for name in ('goal5_persistence_contract.json','goal5_participant_contract.json','goal5_schema_contract.json','goal5_persistence_gaps.json','ctest_manifest.json')))
        from goal5_participant_contract import validate as validate_participants
        errors.extend(validate_participants(load_json(freeze/'goal5_participant_contract.json'), load_json(freeze/'goal5_persistence_contract.json'), implementation=True))
        if errors: raise ValueError('\n'.join(errors))
    except (OSError,ValueError,KeyError,TypeError) as error: print('BLOCKED: '+str(error),file=sys.stderr); return 1
    print('PASS: stage 5.1 reviewed; stage 5.2 schema/storage coverage exact, no stage-5.2 debt, wire/storage/process suites registered in Debug/Release. Later-stage debt retained.')
    return 0
if __name__=='__main__': raise SystemExit(main())
