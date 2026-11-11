# F-II.0b: native Graph roundtrip

Дата: 11 ноября 2026. Отправная точка: `5324262`, поверх замороженного F-I `ce06956`. Push не выполнялся.

**F-II.0b BLOCKED. Native Graph roundtrip PASS.**

## Реализовано

1. Штатная сборка использует официальный `jbr_jcef-21.0.10-windows-x64-b1163.108`. Архив проверяется publisher SHA512, повторно извлекается из проверенного offline cache. Vanilla JBR больше не является default. Runtime legal сохранён. Добавлены точные JCEF/CEF/Chromium BSD и полный встроенный credits: 542 компонента, 14 862 123 байта HTML, SHA512 проверяется при упаковке. Rete/React/plugins/esbuild закреплены package-lock с integrity; зависимости разрешаются один раз, нормальная сборка использует npm offline.
2. Настоящий Graph FileEditor содержит Rete. Его сообщения проходят JSQuery → Java Bridge → private stdio IPC v4 → EditorWorkspace::Execute. Ответный native snapshot — источник отображения. Авторитетная модель остаётся GraphDefinition. Узлы Event/Native Call/Wait происходят из текущих зарегистрированных native операций; IDs выделяет C++.
3. DTO v1 содержит graph/document, node/type/name, порты id/name/kind/direction/type, edges с двумя port IDs, координаты. ID и revision передаются в JavaScript строками. Native команды create/connect/disconnect/move/delete имеют обычные generation/revision и Play read-only guards.
4. EditorDocument хранит только редакторские позиции. Архив `graph_layout.version=1`; отсутствующий v0 мигрирует к implicit grid. MoveGraphNode участвует в native Undo/Redo и dirty/Save. Неизвестная версия, недопустимые/дублирующиеся node IDs и non-finite/out-of-range координаты отклоняются. При удалении узла очищаются координаты и связи.
5. Browser/query принадлежат одной FileEditor view и освобождаются через Disposer. При rebind поколения создаётся новый adapter. Preferred focus делегируется JCEF. JS имеет обработчики Ctrl+S/Z/Y и Escape; во время перестроения проекции pointer input блокируется. Отмена drag не создаёт native-команду.
6. HTML/JS поставляются локально. CSP разрешает hash bundle и inline style renderer, запрещает остальные источники. Request handler ограничен точным виртуальным file-origin экземпляра, блокирует прочие file/network ресурсы, навигацию, popup и OS protocol execution. Bridge допускает только graph edits и Save/Undo/Redo, проверяет nonce, DocumentKey, revision/generation, размер и число строк. Произвольных filesystem/shell RPC нет.
7. Окончательные контракты Code Space и managed lifecycle вынесены в [F-II.2](FII_CODESPACE_MANAGED_CONTRACT.md). Выбран worker на поколение Play с C++ владельцем simulation, сохранением ABI v1 и отзывом handles на Stop. Это контракт будущей реализации: LSP, Rider и C# из Play здесь не подключались.

## Выполненные проверки

- **PASS:** production Java Bridge test: три native-узла, соединение, отказ несовместимого/неизвестных endpoints, stale revision, отказ NaN/выхода за диапазон, Undo/Redo связи и перемещения, Save/Open во втором companion-процессе с точными IDs/layout, delete/Undo с восстановлением рёбер и координат.
- **PASS:** layout migration в EditorCoreTests: архив v0 без поля, повторное чтение после миграции, v1 coordinates, отказ v2. `fii0b-layout-migration.stdout.log`.
- **PASS:** 19/19 адресных CTest с пересборкой изменённых editor consumers; Graph definition/VM/control/scheduler/time/serialization, schema, renderer, Editor Core/Workspace/SceneView/Headless и архитектура. `fii0b-regressions.stdout.log`. Полный профиль 161/161 не повторялся.
- **PASS:** IPC v4 adversarial framing, 16 случаев; catalog coverage. `fii0b-qualification.stdout.log`.
- **PASS:** штатный standalone Release EXE с JBR/JCEF и offline npm: JSQuery/native create/connect/move/Undo/Redo/Save; React DOM содержит native узлы/ребро; FileEditor close/reopen освобождает browser/query; отдельный HWND; после shutdown нет принадлежащих CEF helpers в пределах 15 секунд. В `platform-status.log` есть явный LIMIT об unsandboxed qualification.
- **PASS:** в этом же EXE CSP отклонила file/HTTPS/loopback fetch и eval, Bridge отклонил open без изменения native revision.
- **KNOWN DIAGNOSTICS:** адресный clang-analyzer/bugprone на editor_commands.cpp, project_archive.cpp, backend.cpp. Новое замечание integer division в implicit grid исправлено. В повторном backend scan нет diagnostics на изменённых строках; сохраняются ранее известные категории signed-bitwise, exception-escape, unsafe-to-allow-exceptions, easily-swappable-parameters в Scene/Composition/EditorDocument/CreateDocument/ThumbnailCache. Статический анализ не объявляется чистым PASS и его checks не менялись.

Журналы временные: `.ai/logs/fii0b-*`.

**Финальный Release smoke PASS:** `.ai/logs/fii0b-release-focus.stdout.log`.
Артефакт: `Temp/U/verification-images/a2affd15934c441f93311fd1d0c07514/EpidemicEditor/EpidemicEditor.exe`.
Проверена передача FileEditor preferred focus браузеру JCEF. При холодном старте найден и исправлен race фиксированной задержки: теперь canvas сообщает явный ready через ограниченный JSQuery, а тест ждёт readiness с timeout. В финальном EXE прошли Graph roundtrip, CSP/Bridge rejection, disposal/reopen, HWND, существующий standalone smoke и CEF shutdown. Проверки физического ввода остаются NOT RUN.

Это квалификационный side-by-side image; рабочий пользовательский дистрибутив не заменялся.

## Точный блокер и оставшаяся квалификация

**Технический блокер — Windows CEF sandbox в launcher.** jpackage EXE не создаёт sandbox info до JVM. В закреплённом Platform SettingsHelper отсутствие `jcef.sandbox.ptr` приводит к `no_sandbox=true`; лог прямо это подтверждает. В обычном запуске Graph поэтому закрыт с Unavailable. Только изолированный `--smoke` допускает локальный unsandboxed сценарий. Это не production/security PASS.

Нужен native Windows launcher с sandbox info, созданным именно в основном EXE, совместимой CEF sandbox library/version, затем проверка renderer mitigations и повтор ограниченного security/lifecycle smoke. Простая установка свойства или фиктивного указателя не исправляет этот контракт. Основание: [CEF sandbox setup](https://chromiumembedded.github.io/cef/sandbox_setup) и SettingsHelper закреплённого ui-jcef 253.31033.145.

**NOT RUN:** физические мышиные connection/move/Escape, клавиатура в основной и отдельной Graph tab, возврат вкладки, нагрузка 500 nodes. Проверка HWND и preferred-focus API не выдаётся за физический ввод. После исправления launcher требуется только адресный сценарий из [Acceptance](FII_ACCEPTANCE.md), без полного U/Computer Use прогона.

## Повтор адресного native roundtrip

Из корня, после штатной Release-сборки:

```powershell
New-Item -ItemType Directory -Force .ai/graph-tests | Out-Null
& C:/Users/Akku/.jdks/ms-21.0.7/bin/javac.exe --release 21 -encoding UTF-8 -d .ai/graph-tests tools/editor-ui-host/java/epidemic/editor/Bridge.java tools/editor-ui-host/tests/GraphTests.java
& C:/Users/Akku/.jdks/ms-21.0.7/bin/java.exe -cp .ai/graph-tests epidemic.editor.GraphTests Temp/b/ui-host/Release/EpidemicEditorBackend.exe .ai/graph-tests
python tools/editor-ui-host/tests/test_protocol.py Temp/b/ui-host/Release/EpidemicEditorBackend.exe
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/editor-ui-host/Build-Standalone.ps1 -JdkRoot C:/Users/Akku/.jdks/ms-21.0.7 -SideBySide -Smoke
```

Первичное наполнение npm offline cache: `powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/editor-ui-host/Build-Graph.ps1 -Resolve`. Для offline упаковки также нужны уже разрешённые 353 Maven artifacts и закреплённый JBR tar.gz. Полностью пустой cache не объявляется offline-ready.

## Подготовка F-II.1

Реализация **не начата**, вход закрыт F-II.0b. После sandbox/приёмки первый батч расширяет существующий Graph adapter: schema-derived palette, Selection/Inspector, удаление/разрыв из UI, диагностика ссылок, load/stress. Остаются тот же native owner, op 38 и versioned DTO; не создаётся альтернативный граф или новая docking оболочка. Code Space/managed остаются в F-II.2 согласно отдельному контракту.
