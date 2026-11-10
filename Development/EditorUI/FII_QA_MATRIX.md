# Проверки F-II.0

| Проверка | Статус | Артефакт или результат |
|---|---|---|
| Сверка с GitHub и базовый Release smoke | PASS | Свежий ответ `git ls-remote`, `.ai/logs/fii0-release.stdout.log`, `platform-status.log` в [Release baseline](FII_BASELINE.md). |
| CTest Graph, Schema, Renderer, Editor и архитектура | PASS | 17/17, `Temp/U/fii0-native.xml`. |
| Native Graph edit / Undo/Redo / Save и новый процесс | PASS | `EpidemicEditorWriteProject`, `EpidemicEditorReadPlayProject` 2/2, `Temp/U/fii0-graph-roundtrip.xml`. Headless fixture использует настоящие `AddGraphNode` и `ConnectPorts` и вызывает VM после открытия. |
| IPC framing и coverage каталогов | PASS | 16 framing cases и 38 canonical panels + 8 on-demand views в Release batch. |
| Managed bridge / managed contract | PASS | 2/2 после пересборки, `Temp/U/fii0-managed.xml`. |
| Turret, hostfxr compile/load/invoke/Close, новый бинарник | PASS | `Temp/U/fii0/turret`. V1 вернул 89 и 78, Close отозвал handle. V2 с новым assembly name вернул 78 при первом вызове. Вызов после Close снова отклонён. |
| EditorFactory на настоящем файле, dirty, save, explicit external reload | PASS | `Temp/U/fii0/VanillaProbe/app/smoke-state/log/fii0-probe.txt`. Временный тест владел редактором и освободил его через EditorFactory. |
| .NET build и LSP diagnostics/hover | PASS | `Temp/U/fii0/lsp_probe.py`, `lsp-stderr.log`. Roslyn `CS0029` на несохранённом тексте и ответ hover. |
| Vanilla JBR JCEF | FAIL | Внутри запущенного EXE `JBCefApp.isSupported=false`. |
| Кандидат JBR JCEF и локальный renderer | PASS для технической схемы | `JBCefApp.isSupported=true`, Chromium 137, три DOM-узла и одно ребро. JSQuery вернул результат. Browser и query освобождены. `Temp/U/fii0/JcefProbe/app/smoke-state/log/fii0-probe.txt`. |
| Процессы JCEF после выхода EXE | PARTIAL | Немедленная проверка насчитала четыре процесса из каталога пробы. При повторном запросе их уже не было. Точный предел времени завершения не измерен. |
| Native Graph → Rete → EditRequest → Save/Open | NOT RUN | Native roundtrip и browser renderer проверены отдельными тестами. |
| Code Space с продуктовым Save/Dirty/conflict и LSP4IJ | NOT RUN | Проверки Java API и LSP сервера завершены по отдельности. |
| Rider file:line | NOT RUN | В проверенных местах установка Rider не обнаружена. |
| Новая managed DLL из Play и выгрузка assembly/ALC | NOT RUN | Hostfxr test проверяет сессии и новую версию DLL под другим именем. |
| Startup/idle, 500 nodes, две вкладки и HWND, повторный browser close/open | NOT RUN | Базовый smoke даёт один startup sample. RAM/CPU/latency по полному нагрузочному профилю требуют отдельного теста. |

Журналы и временные исходники спайков находятся в `Temp/U/fii0`. Базовая сборка и штатные тесты воспроизводятся командой из [baseline](FII_BASELINE.md).
