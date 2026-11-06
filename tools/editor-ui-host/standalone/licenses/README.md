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
