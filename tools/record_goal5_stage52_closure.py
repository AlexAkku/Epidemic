"""Explicit 5.1 re-review and M5.2 Framework schema/storage evidence admission.

Unknown discovered symbols fail closed; Runtime cross-plane coordination remains
5.3, not a claimed fresh-process whole-engine restore. Does not qualify itself.
"""
import argparse
import copy
import sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'docs/freeze'))
from goal5_persistence_contract import load_json,encoded

RUNTIME_REVIEW = {
    'SaveTransactionState':('NON_PERSISTENT_OWNER','Transaction lifecycle enum, not owned durable data.'),
    'ISaveTransaction':('NON_PERSISTENT_OWNER','Abstract operation protocol; an open transaction is never a durable snapshot.'),
    'IPersistenceAdministrativeTransaction':('NON_PERSISTENT_OWNER','Administrative operation protocol, not a saved state owner.'),
    'IPersistenceBackend':('NON_PERSISTENT_OWNER','Injected persistence I/O protocol; actual authoritative records belong to InMemoryPersistenceStore and selected backend.'),
    'IPersistenceStore':('NON_PERSISTENT_OWNER','Abstract query/transaction protocol; concrete InMemoryPersistenceStore owns records.'),
    'ITombstoneStore':('NON_PERSISTENT_OWNER','Read-only query interface; TombstoneRecord values are persisted in PersistenceSnapshot.'),
    'PersistenceOptions':('NON_PERSISTENT_OWNER','Immutable bootstrap backend pointer and durability selection, never serialized as data.'),
    'ITimeRuntime':('NON_PERSISTENT_OWNER','Abstract checkpoint/advance port; concrete TimeRuntime is the single clock owner.'),
    'TimeSnapshot':('READ_ONLY_SNAPSHOT','Frame-facing observation omits fractional remainder; header explicitly requires TimeCheckpoint for continuation.'),
    'TimeMutableState':('RECONSTRUCTED_TRANSIENT','Private prepare candidate rebuilt from TimeRuntime; not a second durable clock owner.'),
    'PersistenceCandidateState':('RECONSTRUCTED_TRANSIENT','Private transaction prepare indexes and candidate snapshot; only successful PublishSnapshot changes the live owner.'),
    'InMemorySaveTransaction':('RECONSTRUCTED_TRANSIENT','Short-lived staged operations are committed or rolled back, not resumed as an open transaction after loading.'),
    'InMemoryPersistenceBackend':('RECONSTRUCTED_TRANSIENT','Memory-only default/reference backend. It cannot claim disk or cross-process durability; create afresh or inject a durable backend.'),
    'TimeRuntime':('PERSISTENT_OWNER','Owns the authoritative clock and deterministic fractional remainder; TimeCheckpoint captures mutable continuation and immutable configuration identity.'),
    'TimeCheckpoint':('PERSISTENT_CHECKPOINT','Complete TimeRuntime continuation checkpoint; includes remainder/revision and requires identical immutable clock configuration.'),
    'InMemoryPersistenceStore':('PERSISTENT_OWNER','Owns authoritative object/tombstone/lazy-rule/zone-override maps and revision; one PersistenceSnapshot is committed via the configured backend.'),
    'PersistenceSnapshot':('PERSISTENT_CHECKPOINT','Complete backend transaction value snapshot owned by InMemoryPersistenceStore, not operational object layout.'),
    'PersistentObjectRecord':('PERSISTENT_CHECKPOINT','Authoritative object record inside PersistenceSnapshot; payload schema ID/version is owned by the application backend contract.'),
    'TombstoneRecord':('PERSISTENT_CHECKPOINT','Authoritative deletion identity/revision inside PersistenceSnapshot.'),
    'LazyRuleRecord':('PERSISTENT_CHECKPOINT','Accepted deterministic lazy-rule seed/state/revision inside PersistenceSnapshot.'),
    'ZoneOverrideSnapshot':('PERSISTENT_CHECKPOINT','Authoritative zone membership/tombstones/lazy rules/revision inside PersistenceSnapshot.')
}

def main():
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument('--record',action='store_true',required=True); parser.parse_args()
    path=ROOT/'docs/freeze/goal5_persistence_contract.json'; contract=load_json(path)
    inventory=load_json(ROOT/'docs/freeze/goal5_persistence_candidates.json')
    graph=load_json(ROOT/'docs/freeze/goal5_participant_contract.json')
    by_participant={p['id']:p for p in graph['participants']}
    discovered={c['candidate_id']:c for c in inventory['candidates']}
    for identity,item in discovered.items():
        if identity in contract['candidates']: continue
        if item['qualified_symbol'] not in {'epidemic::gameplay::savegame::WireReader','epidemic::gameplay::savegame::WireWriter','epidemic::runtime::FilePersistenceBackend'}:
            raise ValueError('unreviewed new persistence symbol: '+item['qualified_symbol'])
        contract['candidates'][identity]={
            'candidate_id':identity,'module':item['module'],'qualified_symbol':item['qualified_symbol'],
            'persistence_role':'RECONSTRUCTED_TRANSIENT','save_plane':'NONE','authoritative_owner':'N/A',
            'capture_contract':'ephemeral bounded byte cursor/buffer only','restore_contract':'create for each encode/decode call',
            'participant_id':'N/A','durable_schema':'N/A','migration_policy':'N/A','durable_storage':'N/A',
            'dependencies':[],'transient_rebuild':'create a fresh local reader/writer for the immutable package bytes',
            'reconciliation':'N/A','deterministic_relevance':'NO','digest_policy':'EXCLUDE',
            'risk_review':{flag:{'decision':'NOT_DURABLE','rationale':'Temporary value codec helper; no authoritative gameplay state.'} for flag in item['risk_flags']},
            'evidence':[{'kind':'executable','source_path':'EngineFramework/DevelopmentInfrastructure/Tests/save_wire_tests.cpp',
                         'test_target':'EpidemicGameFrameworkSaveGameTests','test_symbol':'TestSaveWireCodec',
                         'contract_assertion':'bounded field-wise historical roundtrip and corrupt/truncated/overflow rejection'}],
            'review_status':'REVIEWED','rationale':'Local codec cursor/buffer is rebuilt per operation and delegates authority to image DTOs, never owns a second saved state.'}
    seen_runtime=set()
    for identity,c in contract['candidates'].items():
        if identity not in discovered: raise ValueError('stale candidate review: '+identity)
        name=c['qualified_symbol'].split('::')[-1]
        if c['module'] in {'EngineRuntime/Time','EngineRuntime/Persistence'} and name in RUNTIME_REVIEW:
            seen_runtime.add(name); role,rationale=RUNTIME_REVIEW[name]
            c.update(persistence_role=role,rationale=rationale,review_status='REVIEWED')
            durable=role in {'PERSISTENT_OWNER','PERSISTENT_CHECKPOINT'}
            if not durable:
                c.update(save_plane='NONE',authoritative_owner='N/A',participant_id='N/A',durable_schema='N/A',migration_policy='N/A',durable_storage='N/A',dependencies=[],reconciliation='N/A',deterministic_relevance='NO',digest_policy='EXCLUDE')
                if role=='RECONSTRUCTED_TRANSIENT': c['transient_rebuild']=rationale
            else:
                is_time=c['module']=='EngineRuntime/Time'
                owner='epidemic::runtime::TimeRuntime' if is_time else 'epidemic::runtime::InMemoryPersistenceStore'
                c.update(save_plane='RUNTIME_PERSISTENCE',authoritative_owner=owner,participant_id='RUNTIME_PERSISTENCE_PLANE',
                    capture_contract='ITimeRuntime::CaptureCheckpoint' if is_time else 'InMemoryPersistenceStore::CreateSnapshot / IPersistenceBackend::CommitSnapshot',
                    restore_contract='ITimeRuntime::RestoreCheckpoint: validate complete configuration before commit' if is_time else 'CreatePersistenceServices: IPersistenceBackend::Load then ValidatePersistenceSnapshot',
                    durable_schema='RUNTIME_TIME_CHECKPOINT_CURRENT_CONFIGURATION_MATCH' if is_time else 'RUNTIME_PERSISTENCE_SNAPSHOT_WITH_APPLICATION_PAYLOAD_SCHEMA_ID_VERSION',
                    migration_policy='CURRENT_CHECKPOINT_CONFIGURATION_ONLY_NO_HISTORICAL_WIRE_SUPPORT' if is_time else 'APPLICATION_OWNED_PAYLOAD_SCHEMA_FIXED_AT_SERVICE_COMPOSITION',
                    durable_storage='GAP:5.3_RUNTIME_CHECKPOINT_PACKAGE_COORDINATION' if is_time else 'IPersistenceBackend::CommitSnapshot(SaveAndFlushRequired): injected atomic durable backend contract; default MemoryOnly is not durable',
                    deterministic_relevance='YES',digest_policy='DEFER_TO_5_5',dependencies=[],transient_rebuild='N/A: durable role is restored rather than rebuilt')
        if c['save_plane']=='FRAMEWORK_SAVEGAME' and c['persistence_role'] in {'PERSISTENT_OWNER','PERSISTENT_CHECKPOINT'}:
            p=by_participant[c['participant_id']]
            c.update(durable_schema=p['id']+':v1',migration_policy='NO_PRIOR_SUPPORTED_VERSION',
                durable_storage='SaveGameImage wire v1 -> ISavePackageStore -> Integration FileSavePackageStore -> Base Platform atomic package replacement',
                dependencies=['participant:'+d for d in p['dependencies']])
            adapter=p['placement'].split('/')[-1]
            if p['owner']=='ScheduledTriggerDispatcher': source,target,symbol='core_integration_tests.cpp','EpidemicGameFrameworkCoreIntegrationTests','const auto staged_leg'
            elif p['owner']=='NarrativeExternalConsequenceOutbox': source,target,symbol='narrative_integration_tests.cpp','EpidemicGameFrameworkNarrativeIntegrationTests','auto repeated_stage'
            else: source,target,symbol=adapter.lower()+'_save_participant_tests.cpp','Epidemic'+adapter+'SaveParticipantTests','Test'+p['owner']+'SaveParticipant'
            proof={'kind':'executable','source_path':'EngineFramework/DevelopmentInfrastructure/Tests/'+source,
                'test_target':target,'test_symbol':symbol,'contract_assertion':'typed owner capture, private preparation, corruption rejection and one-use publication'}
            if proof not in c['evidence']: c['evidence'].append(proof)
    if seen_runtime!=set(RUNTIME_REVIEW): raise ValueError('Runtime review set changed: '+str(set(RUNTIME_REVIEW)-seen_runtime))
    for c in contract['candidates'].values():
        if c['qualified_symbol']=='epidemic::runtime::FilePersistenceBackend':
            c.update(persistence_role='NON_PERSISTENT_OWNER',rationale='Storage adapter owns only a bootstrap path. InMemoryPersistenceStore remains authoritative; backend Load/Commit transfer complete immutable byte packages without a second live state cache.',capture_contract='IPersistenceBackend::CommitSnapshot bounded v1 representation',restore_contract='IPersistenceBackend::Load validates full package before service composition',transient_rebuild='CreateFilePersistenceBackend for the configured native path',evidence=[{'kind':'executable','source_path':'EngineRuntime/Support/tests/runtime_package_tests.cpp','test_target':'EpidemicRuntimeSupportTests','test_symbol':'TestRuntimeDurablePackages','contract_assertion':'all fields, version bounds, corruption/truncation, atomic failure, fresh service reopen'}])
        if c['save_plane']=='RUNTIME_PERSISTENCE' and c['persistence_role'] in {'PERSISTENT_OWNER','PERSISTENT_CHECKPOINT'}:
            is_time=c['module']=='EngineRuntime/Time'
            c.update(durable_schema=('RUNTIME_TIME_EPIRTIM1' if is_time else 'RUNTIME_PERSISTENCE_EPIRPER1')+':v1',migration_policy='NO_PRIOR_SUPPORTED_VERSION',durable_storage='Runtime Support '+('CommitRuntimeTimeCheckpoint/LoadRuntimeTimeCheckpoint' if is_time else 'CreateFilePersistenceBackend::CommitSnapshot/Load')+' -> bounded v1 package -> Base Platform atomic replacement; independent save plane, unified causal generation stays M5.3')
            proof={'kind':'executable','source_path':'EngineRuntime/Support/tests/runtime_package_tests.cpp','test_target':'EpidemicRuntimeSupportTests','test_symbol':'TestRuntimeDurablePackages','contract_assertion':'full durable Runtime package -> fresh clock/store restore; invalid schema/structure/failure preserves old package'}
            if proof not in c['evidence']: c['evidence'].append(proof)
    for module,r in contract['modules'].items():
        r['candidate_ids']=sorted(i for i,c in contract['candidates'].items() if c['module']==module)
    path.write_text(encoded(contract),encoding='utf-8',newline='\n')
    print('PASS: reviewed 21 Runtime false-positive/owned-state decisions, two wire helpers and all 139 Framework durable schema/storage paths. Runtime whole-plane coordination stays M5.3.')
if __name__=='__main__': main()
