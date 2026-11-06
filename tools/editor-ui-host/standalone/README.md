# Open Platform product assembly

Pinned source: https://github.com/JetBrains/intellij-community/tree/idea/253.31033.145 (Apache-2.0; LICENSE and NOTICE retained). Public artifacts come from JetBrains releases/dependency repositories and Maven Central. `pom.xml` declares roots; `dependencies.lock.json` pins coordinates and SHA-256. JAR filenames retain group IDs: Platform and Fleet both publish `kernel` at the same version.

The product owns its entry point, app starter, application info, icon and descriptor. `com.intellij.idea.MainImpl` reproduces the small upstream Apache bootstrap bridge absent from published artifacts. No installed IDEA product is a distribution source.

The upstream essential set references two identifier-highlighting modules absent from the Community module list and public Maven. `EpidemicModules.xml` retains the available essential services and omits those IDE features. `prepare_resources.py` derives core extensions from the pinned resources JAR, excluding the unavailable IDE internal Theme Color Picker and the IDE startup activity suggesting antivirus exclusions for build/index folders. The game product has no IDE build folders; antivirus configuration is untouched. The assembler asserts the upstream descriptor shape. No Platform service is replaced with a stub.

Published library JARs omit generated product wrapper descriptors. Service-free `intellij.libraries.*` descriptors describe real locked bytecode on the shared core classpath. Platform descriptors/implementations come from actual artifacts. Upstream `idea.force.use.core.classloader` preserves class identity in this flattened Maven layout.

Known warnings: this pinned Platform supports a deprecated distribution-descriptor path without `plugin-classpath.txt`; a later upgrade must migrate assembly. The default product bundles a separately downloaded public JBR 21.0.10+1-b1163.108, pinned by publisher SHA512 in Resolve-Runtime.ps1. The optional -StandardOpenJdk diagnostic build uses basic font selection and the Platform picker fallback. Neither warning establishes physical DPI acceptance. Product-specific external support/update URLs and a clean-machine legal/deployment review remain work; retaining notices is not a legal-audit PASS.

Ship the complete app image, runtime legal files and notices. Local smoke cannot prove clean-machine C++ runtime installation, GPU/driver availability, IME or physical monitor behavior.

The pinned DirectoryLock correction and TCP cache migration are documented in `upstream/README.md`. Owner activation and stale-lock recovery are exercised by `../tests/Test-StandaloneRestart.ps1` against the packaged EXE. The product includes its own empty productivity-feature catalogue; shared Platform features still come from their actual resources.

File selection goes through Platform FileChooser so modality/focus are registered with the application. On Microsoft OpenJDK, the pinned factory deliberately uses the Platform chooser; native Windows chooser support checks for JetBrains Runtime. The default public JBR product selects the native chooser through this upstream factory; the standard-OpenJDK diagnostic fallback is not labelled a Windows native-picker test.
