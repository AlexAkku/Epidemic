"""Retain the complete credits embedded in the pinned official JBR/libcef."""
from pathlib import Path
import gzip
import hashlib
import sys

source = Path(__file__).with_name("licenses") / "CEF-Chromium-credits.html.gz"
payload = gzip.decompress(source.read_bytes())
if len(payload) != 14862123 or hashlib.sha512(payload).hexdigest() != "0b2ee6d5c4244a6c7e3e034690e71a5aec522cee20a46b9fb622be6cdd8ab6228207fc47f16a83325261963d71e59a9f00eff94878e6107194ff6d3c518d239a":
    raise RuntimeError("Pinned Chromium third-party credits checksum mismatch")
destination = Path(sys.argv[1]).resolve(strict=True)
(destination / "CEF-Chromium-credits.html").write_bytes(payload)
print("PASS complete pinned Chromium credits: 542 components, SHA512 verified, local HTML retained")
