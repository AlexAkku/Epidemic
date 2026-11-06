"""Pin resolved runtime artifacts and retain their upstream license/NOTICE/POM files."""
import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
from zipfile import ZipFile

parser=argparse.ArgumentParser()
parser.add_argument('--update',action='store_true')
args=parser.parse_args()
root=Path(__file__).resolve().parents[3]
product=root/'Temp/U/standalone'
repository=root/'Temp/U/maven'
index=defaultdict(list)
for file in repository.rglob('*.jar'):
    parts=file.relative_to(repository).parts
    index['.'.join(parts[:-3])+'.'+file.name].append(file)
records=[]
for jar in sorted((product/'lib').glob('*.jar')):
    if jar.name=='epidemic.jar':continue
    digest=hashlib.file_digest(jar.open('rb'),'sha256').hexdigest()
    matches=[p for p in index[jar.name] if hashlib.file_digest(p.open('rb'),'sha256').hexdigest()==digest]
    if len(matches)!=1:raise RuntimeError(f'Artifact origin is ambiguous or missing: {jar.name}')
    source=matches[0];parts=source.relative_to(repository).parts
    group='.'.join(parts[:-3]);artifact,version=parts[-3:-1]
    coordinate=f'{group}:{artifact}:{version}'
    records.append({'coordinate':coordinate,'file':jar.name,'sha256':digest})
    notices=product/'licenses'/f'{group}.{artifact}-{version}';notices.mkdir(parents=True,exist_ok=True)
    pom=source.parent/f'{artifact}-{version}.pom'
    if not pom.is_file():raise RuntimeError(f'Missing upstream POM: {coordinate}')
    (notices/'upstream.pom').write_bytes(pom.read_bytes())
    with ZipFile(jar) as archive:
        for name in archive.namelist():
            base=Path(name).name.upper()
            if name.startswith('META-INF/') and base.startswith(('LICENSE','NOTICE','COPYING')) and not name.endswith('/'):
                (notices/name.replace('/','_')).write_bytes(archive.read(name))
lock=Path(__file__).with_name('dependencies.lock.json')
if args.update:lock.write_text(json.dumps(records,indent=2)+'\n',encoding='utf-8')
elif json.loads(lock.read_text(encoding='utf-8'))!=records:raise RuntimeError('Resolved artifacts differ from the committed dependency lock')
print(f'PASS {len(records)} runtime artifact coordinates/hashes and retained upstream POM/LICENSE/NOTICE files')
