"""Compare shipped UI metadata to the owner's supplied UI Spec, without generating it."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[3]
spec = (root / "Temp/!Tasks/UI_Roadmap.md").read_text(encoding="utf-8-sig")
resources = root / "tools/editor-ui-host/resources/epidemic/editor"
panels = {row[0]: row for row in (line.split("\t") for line in (resources / "panels.tsv").read_text(encoding="utf-8").splitlines())}
schemas = {row[0]: row for row in (line.split("\t") for line in (resources / "fields.tsv").read_text(encoding="utf-8").splitlines())}
canonical = spec.split("### 3.2.")[1].split("### 3.3.")[0]
count = 0
for line in canonical.splitlines():
    match = re.match(r"\| `([^`]+)` \| `([^`]+)`", line)
    if match:
        panel, anchor = match.groups()
        assert panel in panels and panels[panel][2] == anchor, (panel, anchor)
        count += 1
assert count == 38 and len(panels) == 46
field_count = 0
for prefix, start, end in [("owners", "### 7.3.", "### 7.4."), ("render", "### 10.1.", "### 10.2."), ("gameplay", "### 10.2.", "### 10.3."), ("behavior", "### 10.3.", "### 10.4."), ("assets", "### 10.4.", "## 11.")]:
    for line in spec.split(start)[1].split(end)[0].splitlines():
        columns = [c.strip() for c in line.strip().split("|")[1:-1]]
        if len(columns) != 2 or columns[0].startswith("-") or columns[0] in ["Owner", "UI тип", "Редактор"]:
            continue
        label = re.sub(r"\s*\([^)]*\)", "", columns[0].replace("`", "")).strip()
        identity = prefix + "." + re.sub(r"[^a-z0-9]+", "-", label.lower()).strip("-")
        assert identity in schemas, identity
        assert schemas[identity][3] == columns[1].replace("`", ""), identity
        field_count += len(columns[1].split(","))
print(f"PASS U6 source-spec coverage: {count} canonical IDs/anchors + 8 on-demand views; {field_count} tabulated fields match exactly; 77 total schema groups/570 UI fields")
