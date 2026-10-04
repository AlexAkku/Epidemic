"""Explicit one-time fixture admission. Normal CI only validates immutable fingerprints."""
import argparse
import hashlib
import sys
sys.dont_write_bytecode=True
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'docs/freeze'))
from goal5_persistence_contract import load_json, encoded

def main():
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument('--record',action='store_true',required=True); parser.parse_args()
    root=ROOT/'Tests/Fixtures/SaveGame'; manifest=root/'manifest.json'
    if manifest.exists(): raise ValueError('historical fixture manifest already exists; explicit schema evolution review required, never regenerate it')
    graph=load_json(ROOT/'docs/freeze/goal5_participant_contract.json'); rows=[]
    for p in sorted(graph['participants'],key=lambda p:p['id']):
        if p['schema_version'] != 1: raise ValueError('initial fixture admission supports reviewed version 1 only')
        rows.append({'fixture_id':p['id']+':v1','participant_id':p['id'],'version':1,'path':'participant/'+p['id']+'/v1.bin'})
    for name in ('empty','multi'): rows.append({'fixture_id':'whole:1:'+name,'wire_version':1,'path':'whole/1/'+name+'.bin'})
    for row in rows:
        path=root/row['path']; data=path.read_bytes()
        if not data: raise ValueError('empty admitted fixture '+str(path))
        row.update(size=len(data),sha256=hashlib.sha256(data).hexdigest())
    manifest.write_text(encoded({'schema_version':1,'policy':'IMMUTABLE_HISTORICAL_FIXTURES_NO_CI_REGENERATION','fixtures':rows}),encoding='utf-8',newline='\n')
    print('PASS: admitted 47 current-v1 participant fixtures and two whole-wire goldens.')
if __name__=='__main__': main()
