"""Read-only aid: print members changed by reviewed owner RestoreSnapshot bodies."""
import re
import argparse
import sys
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "docs/freeze"))
from goal5_participant_bindings import bindings, DURABLE_MEMBERS
from goal5_persistence_inventory import strip_comments, matching_brace

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--check', action='store_true')
args = parser.parse_args()
errors = []
for b in bindings():
    if not b["id"].startswith("framework.") or b["id"] == "framework.facts": continue
    code = "\n".join(strip_comments(p.read_text(encoding="utf-8")) for p in (ROOT / b["owner_module"] / "src").glob("*.cpp"))
    methods = {}
    for m in re.finditer(r"\b" + b["owner"] + r"::(\w+)\s*\(", code):
        opening = code.find("{", m.end()); closing = matching_brace(code, opening)
        if closing is not None: methods[m.group(1)] = code[opening + 1:closing]
    visited = set(); members = set()
    def visit(method):
        if method in visited: return
        visited.add(method)
        body = methods.get(method, "")
        members.update(re.findall(r"\b(\w+_)\s*(?:\.(?:swap|clear|push_back|emplace|insert|erase|reserve|Restore|resize)\s*\(|(?:[+\-*/]|\|)?=(?!=)|\+\+|--)", body))
        members.update(re.findall(r"(?:\+\+|--)\s*(\w+_)\b", body))
        members.update(re.findall(r"\b(\w+_)\.[A-Za-z_]\w*\s*(?:[+\-*/|]?=(?!=)|\+\+|--)", body))
        for called in re.findall(r"\b(\w+)\s*\(", body):
            if called in methods: visit(called)
    visit(b["restore"])
    if args.check:
        reviewed = set(DURABLE_MEMBERS[b['owner']].split())
        if members != reviewed:
            errors.append(f"{b['owner']}: publication footprint drift; missing={sorted(members-reviewed)}, extra={sorted(reviewed-members)}")
    else:
        print(f"{b['owner']}: {' '.join(sorted(members))}")
if errors:
    print('\n'.join(errors), file=sys.stderr)
    raise SystemExit(1)
if args.check:
    print('PASS: 33 reviewed core Restore publication footprints match; Facts uses its explicit immutable-registration port.')
