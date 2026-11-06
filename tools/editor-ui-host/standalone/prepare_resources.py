"""Classpath product assembly: declare service-free upstream library wrapper modules.

The runtime JARs are real, dependency-locked artifacts on the core classpath.
Only intellij.libraries.* wrappers receive empty descriptors; Platform modules
must supply their actual descriptors and service implementations.
"""
import json
from pathlib import Path
import sys
import re
from zipfile import ZipFile

target=Path(sys.argv[1]).resolve(strict=True)
source=Path(__file__).with_name('library-modules.json')
data=json.loads(source.read_text(encoding='utf-8'))
for name in data['modules']:
    if not name.startswith('intellij.libraries.'):
        raise RuntimeError('Never synthesize a Platform/service module descriptor: '+name)
    destination=target/(name+'.xml')
    if not destination.exists():
        destination.write_text('<!-- Service-free upstream library wrapper; real locked bytecode is on the core classpath. -->\n<idea-plugin visibility="public"/>\n',encoding='utf-8')
print(f"Prepared {len(data['modules'])} service-free library module wrappers; no Platform service stubs")

# Compose the game's core descriptor from the exact upstream resources. The IDE
# internal theme-picker tool is not published with the open runtime artifacts.
# All actual Platform services/extensions are otherwise retained unchanged.
libraries=Path(sys.argv[2]).resolve(strict=True)
with ZipFile(libraries/'com.jetbrains.intellij.platform.resources-253.31033.145.jar') as archive:
    lang=archive.read('META-INF/PlatformLangPlugin.xml').decode('utf-8')
    extensions=archive.read('META-INF/PlatformExtensions.xml').decode('utf-8')
old='/META-INF/PlatformExtensions.xml'
assert lang.count(old)==1, 'Pinned upstream core descriptor changed'
lang=lang.replace(old,'/META-INF/EpidemicPlatformExtensions.xml')
extensions,count=re.subn(r'<toolWindow\s+factoryClass="com\.intellij\.internal\.inspector\.themePicker\.UiThemeColorPickerToolWindowFactory"[^>]*/>',
    '<!-- IDE internal Theme Color Picker is outside this game product. -->',extensions)
assert count==1, 'Pinned upstream internal-tool registration changed'
extensions,count=re.subn(r'<backgroundPostStartupActivity\s+implementation="com\.intellij\.diagnostic\.WindowsDefenderCheckerActivity"\s+os="windows"\s*/>',
    '<!-- IDE build/index exclusion suggestions are outside this empty shell container. -->',extensions)
assert count==1, 'Pinned upstream IDE performance-check registration changed'
meta=target/'META-INF'
meta.mkdir(exist_ok=True)
(meta/'EpidemicPlatformLangPlugin.xml').write_text(lang,encoding='utf-8')
(meta/'EpidemicPlatformExtensions.xml').write_text(extensions,encoding='utf-8')
