"""Record exact-tree M5.2 Matrix qualification without inventing semantic evidence."""
import argparse, sys
from pathlib import Path
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'docs/freeze'))
from goal5_participant_contract import REVIEW, QUALIFICATION, source_fingerprints, graph_fingerprint, validate
from goal5_persistence_contract import load_json, encoded

def main():
    ap=argparse.ArgumentParser(description=__doc__); ap.add_argument('--record',action='store_true'); ap.add_argument('--invalidate',action='store_true')
    args=ap.parse_args()
    if args.invalidate:
        QUALIFICATION.write_text(encoded({'schema_version':1,'status':'PENDING_REQUALIFICATION','reason':'Source/evidence merge requires a new exact-tree Windows Matrix.'}),encoding='utf-8',newline='\n')
        return 0
    if not args.record: ap.error('explicit --record required')
    review=load_json(REVIEW)
    errors=validate(review,load_json(ROOT/'docs/freeze/goal5_persistence_contract.json'),True)
    if errors: raise ValueError('semantic participant evidence is incomplete before qualification:\n'+'\n'.join(errors))
    report=load_json(ROOT/'.ci_cache/report.json')
    if report.get('status')!='ok' or report.get('mode')!='Matrix': raise ValueError('a successful Matrix run is required')
    if report.get('goal5_source_sha256') != source_fingerprints():
        raise ValueError('Matrix source identity is missing or changed since the run began')
    manifest=load_json(ROOT/'docs/freeze/ctest_manifest.json'); stages={s['name']:s for s in report['stages']}; profiles={}
    for name,profile in manifest['profiles'].items():
        test=stages.get('test:'+name,{}); build=stages.get('build:'+name,{}); count=len(profile['tests'])
        if test.get('status')!='ok' or build.get('status')!='ok' or test.get('details',{}).get('regex') is not None or test.get('note')!=f'{count}/{count} tests passed':
            raise ValueError(f'exact Matrix profile not qualified: {name}')
        profiles[name]={'status':'PASS','test_count':count}
    receipt={'schema_version':1,'status':'PASS','mode':'Matrix','finished_at_utc':report['finished_at_utc'],
             'profiles':profiles,'source_sha256':source_fingerprints(),'graph_sha256':graph_fingerprint(review)}
    QUALIFICATION.parent.mkdir(exist_ok=True)
    QUALIFICATION.write_text(encoded(receipt),encoding='utf-8',newline='\n')
    print(f"PASS: recorded Matrix execution receipt for {len(review['participants'])} already-reviewed participant implementations.")
if __name__=='__main__': raise SystemExit(main())
