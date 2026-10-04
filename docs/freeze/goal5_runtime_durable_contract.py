"""Runtime durable path/schema/golden gate. Explicit first admission only; normal checks never rewrite fixtures."""
import argparse
import copy
import hashlib
import json
import subprocess
import sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[2]
FIXTURES=ROOT/'Tests/Fixtures/RuntimePersistence'
MANIFEST=FIXTURES/'manifest.json'
POLICY={'current_version':1,'minimum_supported_version':1,'supported_versions':[1],
        'migration_edges':[],'migration_policy':'NO_PRIOR_SUPPORTED_VERSION','future_version':'REJECT',
        'integer_encoding':'FIXED_WIDTH_LITTLE_ENDIAN','checksum':'FNV1A64_CORRUPTION_NOT_AUTHENTICATION',
        'max_package_bytes':64*1024*1024,'max_records':10000,'max_payload_bytes':1024*1024,
        'max_aggregate_zone_records':10000,'atomic_store':'Base Platform CommitFilePackage',
        'single_writer':'EXTERNALLY_SYNCHRONIZED','cross_plane_consistent_cut':'NOT_QUALIFIED_UNTIL_5_3',
        'missing_file':'ERROR_INITIALIZE_EXPLICITLY','retry':'NEW_CALL_AFTER_FAILURE_OLD_FILE_UNCHANGED',
        'hardware_power_loss_guarantee':False}
FORMATS={'time.bin':'EPIRTIM1','persistence.bin':'EPIRPER1'}

def validate(manifest,blobs):
    errors=[]
    if manifest.get('schema_version')!=1 or manifest.get('reviewed') is not True or manifest.get('policy')!=POLICY: errors.append('unreviewed/incoherent Runtime durable policy')
    fixtures=manifest.get('fixtures',{})
    if set(fixtures)!=set(FORMATS) or set(blobs)!=set(FORMATS): errors.append('incomplete Runtime golden inventory')
    for name,magic in FORMATS.items():
        data=blobs.get(name,b''); row=fixtures.get(name,{})
        if row.get('magic')!=magic or row.get('schema_version')!=1 or row.get('sha256')!=hashlib.sha256(data).hexdigest() or row.get('size_bytes')!=len(data): errors.append(name+': changed/missing historical fixture')
        if len(data)<20 or len(data)>POLICY['max_package_bytes'] or data[:8]!=magic.encode() or int.from_bytes(data[8:12],'little')!=1: errors.append(name+': invalid fixture format/version')
    return errors

def self_test():
    blobs={name:magic.encode()+b'\x01\x00\x00\x00'+b'\x00'*8 for name,magic in FORMATS.items()}
    manifest={'schema_version':1,'reviewed':True,'policy':copy.deepcopy(POLICY),'fixtures':{name:{'magic':FORMATS[name],'schema_version':1,'size_bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()} for name,data in blobs.items()}}
    assert not validate(manifest,blobs)
    for field,value in [('supported_versions',[0,1]),('migration_policy','FAKE_V0'),('future_version','ACCEPT'),('max_package_bytes',0),('cross_plane_consistent_cut','QUALIFIED')]:
        bad=copy.deepcopy(manifest); bad['policy'][field]=value; assert validate(bad,blobs)
    bad=copy.deepcopy(manifest); del bad['fixtures']['time.bin']; assert validate(bad,blobs)
    bad=dict(blobs); bad['time.bin']+=b'x'; assert validate(manifest,bad)
    bad=copy.deepcopy(manifest); bad['reviewed']=False; assert validate(bad,blobs)
    print('PASS: Runtime durable policy positive and eight version/policy/fixture negatives.')

def main():
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument('--check',action='store_true'); parser.add_argument('--self-test',action='store_true'); parser.add_argument('--record',action='store_true'); parser.add_argument('--executable'); args=parser.parse_args()
    if args.self_test: self_test(); return 0
    if args.record:
        if not args.executable: parser.error('--record requires the tests-first Runtime executable')
        if MANIFEST.exists() or any((FIXTURES/name).exists() for name in FORMATS): raise ValueError('immutable Runtime fixtures already admitted; overwrite forbidden')
        FIXTURES.mkdir(parents=True,exist_ok=True)
        subprocess.run([args.executable,'runtime-record-fixtures',str(FIXTURES)],check=True,timeout=90)
        blobs={name:(FIXTURES/name).read_bytes() for name in FORMATS}
        manifest={'schema_version':1,'reviewed':True,'policy':POLICY,'fixtures':{name:{'magic':FORMATS[name],'schema_version':1,'size_bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()} for name,data in blobs.items()}}
        MANIFEST.write_text(json.dumps(manifest,indent=2,sort_keys=True)+'\n',encoding='utf-8',newline='\n')
    elif not args.check: parser.error('choose --check/--self-test or explicit --record')
    manifest=json.loads(MANIFEST.read_text(encoding='utf-8')); blobs={name:(FIXTURES/name).read_bytes() for name in FORMATS}
    errors=validate(manifest,blobs)
    if {p.name for p in FIXTURES.iterdir()} != {'manifest.json',*FORMATS}: errors.append('unexpected Runtime fixture file')
    if errors: raise ValueError('\n'.join(errors))
    print('PASS: Runtime Time/Persistence explicit v1 durable paths and two immutable goldens; no cross-plane qualification claimed.')
    return 0
if __name__=='__main__': raise SystemExit(main())
