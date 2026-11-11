# Upstream notices

The build retains every runtime artifact's POM and embedded META-INF license/notice files under a coordinate-specific directory. Platform Apache LICENSE/NOTICE are copied from the pinned `idea/253.31033.145` Community source. jlink retains OpenJDK runtime legal files.

Additional texts missing from the published JARs:

| File | Applies to | Upstream text |
|---|---|---|
| jediterm-LGPL-3.0.txt and GPL-3.0.txt | org.jetbrains.jediterm:jediterm-core / jediterm-ui:3.61 | https://github.com/JetBrains/jediterm/blob/master/LICENSE-LGPLv3.txt and https://www.gnu.org/licenses/gpl-3.0.txt |
| pty4j-EPL-1.0.txt | org.jetbrains.pty4j:pty4j:0.13.11 | https://github.com/JetBrains/pty4j/blob/master/LICENSE |
| java-compatibility-GPL-2.0-with-classpath-exception.txt | org.jetbrains.intellij.deps:java-compatibility:1.0.1 | https://github.com/JetBrains/intellij-deps-java-compatibility/blob/master/LICENSE |

Text line endings/trailing whitespace are normalized; license wording is retained. The Java Compatibility file includes the actual linking exception, not just a generic GPL label. The corresponding license choices are also declared by the pinned Platform source at `platform/build-scripts/src/org/jetbrains/intellij/build/CommunityLibraryLicenses.kt`.

Unmodified library source locations are the repositories above; exact binary coordinates/hashes are in `dependencies.lock.json`. This collection is not a completed source-offer or full redistribution review. The local qualification image must not be described as legally audited for publication. Remaining review includes exact corresponding source/build material for copyleft components and complete non-META-INF notices, including font/icon attribution.


## Pinned JCEF / Chromium

The standard runtime is the official `jbr_jcef-21.0.10-windows-x64-b1163.108`,
verified against the publisher SHA512 in `Resolve-Runtime.ps1`. Its unmodified
`legal` directory stays in the packaged runtime. Additional retained texts:

- `JCEF-BSD.txt`: https://raw.githubusercontent.com/JetBrains/jcef/e397b569415d3c67b114495985454e5201955573/LICENSE.txt
- `CEF-BSD.txt`: https://raw.githubusercontent.com/chromiumembedded/cef/f354b0eaa12bc11adb0591911ebaa528b83e6cd6/LICENSE.txt
- `Chromium-BSD.txt`: https://chromium.googlesource.com/chromium/src/+/137.0.7151.104/LICENSE
- `CEF-Chromium-credits.html.gz`: complete `chrome://credits/` DOM from the
  checksum-verified runtime's libcef, extracted locally on 2026-11-11, 542 product
  entries, 14,862,123 UTF-8 bytes. Deterministic gzip is used to retain the complete
  upstream notice payload. `expand_jcef_notices.py` verifies its SHA512 and writes
  the readable HTML into the product licenses directory without network access.
  Links to Chromium's internal stylesheet are preserved; all notice text is in
  the HTML itself. Extraction used a disposable JCEF process with no project,
  no JavaScript bridge, local `chrome://credits/` and explicit unsandboxed mode.
- `licenses/rete` in the packaged application contains exact npm package metadata,
  source/lock, and full LICENSE/NOTICE files for the installed dependency tree.
  `npm ci` verifies the registry integrity hashes in the committed lock; packaging
  uses `--offline` unless `-Resolve` was explicitly supplied.

This preserves the bundled notices; the general distribution/source-offer review
above remains distinct from local F-II qualification.

## Native CEF sandbox launcher

The product EXE statically links the matching official CEF sandbox library. Its
exact SDK archive/SHA512 and public source contracts are in
`../native-launcher/README.md` and `../../Build-Launcher.ps1`. CEF BSD and the full
Chromium credits above apply; SDK headers/CMake and sandbox library are taken
from that verified archive. Product packaging additionally retains this own
launcher's C++/CMake sources, manifest, README and build script under
`licenses/native-launcher`. No installed IDEA or proprietary launcher binary is
used. SDK cache is build-time only; running the packaged EXE needs no downloads.
