#!/usr/bin/env python3
"""Check Obsidian wikilinks and Canvas nodes in this offline Vault. Python 3.10+."""
from pathlib import Path
import re,json,sys
root=Path(__file__).resolve().parent.parent
mds=list(root.rglob('*.md'))
files={p.relative_to(root).as_posix():p for p in root.rglob('*') if p.is_file()}
name_index={}
for f in files:
 if f.endswith('.md'):
  name_index.setdefault(Path(f).name[:-3],[]).append(f)
errors=[]
for p in mds:
 text=p.read_text(encoding='utf-8-sig')
 text=re.sub(r'```[^\n]*\n[\s\S]*?\n```','',text)
 text=re.sub(r'`[^`\n]*`','',text)
 for target in re.findall(r'\[\[([^\]]+)\]\]',text):
  dest=target.split('|',1)[0].split('#',1)[0].strip()
  if not dest:continue
  if dest in files or dest+'.md' in files:continue
  if '/' not in dest and dest in name_index and len(name_index[dest])==1:continue
  errors.append((str(p.relative_to(root)),dest))
for p in root.rglob('*.canvas'):
 data=json.loads(p.read_text(encoding='utf-8'))
 for node in data.get('nodes',[]):
  if node.get('type')=='file' and node.get('file') not in files: errors.append((p.name,node['file']))
print('Markdown files:',len(mds),'Canvas files:',len(list(root.rglob('*.canvas'))),'broken wikilinks:',len(errors))
for a,b in errors[:70]:print(a,'->',b)
sys.exit(1 if errors else 0)
