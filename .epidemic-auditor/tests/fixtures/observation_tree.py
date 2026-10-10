"""Controlled root/child/grandchild lifetimes; no reported metrics are accepted as truth."""
import json
from pathlib import Path
import subprocess
import sys
import time

directory = Path(sys.argv[2])
role = sys.argv[1]
if role in {"root", "child"}:
    subprocess.Popen([sys._base_executable, __file__, "child" if role == "root" else "grandchild", str(directory)])
(directory / (role + ".json")).write_text(json.dumps({"pid": __import__("os").getpid()}))
while not (directory / ("exit-root" if role == "root" else "exit-descendants")).exists():
    time.sleep(0.01)
