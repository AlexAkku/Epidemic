# F-II.0. Технический отчёт и ADR

**Статус F-II.0 — PASS. F-II.0a PASS, F-II.0b PASS.** Native Graph roundtrip и настоящий Windows CEF sandbox квалифицированы в штатном продукте. Code Space/managed по уточнённому заданию закреплены окончательными контрактами F-II.2. [Отчёт F-II.0b](FII0B_GRAPH_ROUNDTRIP.md) · [Baseline](FII_BASELINE.md) · [Матрица](FII_FEATURE_MATRIX.md) · [Проверки](FII_QA_MATRIX.md) · [Приёмка](FII_ACCEPTANCE.md).

## Зависимости на Windows 11 x64

| Зависимость | Версия и лицензия | Размер и offline |
|---|---|---|
| IntelliJ Platform | 253.31033.145, 353 закреплённых артефакта с индивидуальными LICENSE/NOTICE/POM в продукте. | `app` базового EXE занимает 378 572 966 байт без сгенерированного smoke-профиля. Пакет работает с локальным runtime и локальным companion. |
| Vanilla JBR | 21.0.10+1-b1163.108, OpenJDK GPLv2 с Classpath Exception и сопроводительные notices. | Пакетный `runtime` 166 469 319 байт. В запущенной Platform `JBCefApp.isSupported() == false`. |
| JBR с JCEF | Публичный Windows x64 `jbr_jcef-21.0.10-windows-x64-b1163.108`. Проверен SHA512 издателя `f231d37bf2f358486bc38c21931651b2240a5d13c86b6b86467e43bd5c4db50e4e5585f1e4515ab7fd7b70b05efeaaef644a692799f52bc0270d9e8aba6cb34a`. Runtime legal, JCEF/CEF BSD и полный Chromium credits сохранены; публичный redistribution review остаётся отдельной задачей. | Архив 259 461 195 байт, распакованный runtime 541 529 756 байт. В архиве есть `jcef.dll`, `libcef.dll`, `chrome_elf.dll`, helpers, `*.pak` и ICU. [Публикация JetBrains](https://github.com/JetBrains/JetBrainsRuntime/releases/tag/jbr-release-21.0.10b1163.108). |
| Rete и renderer | MIT. `rete@2.0.6`, `rete-area-plugin@2.3.2`, `rete-connection-plugin@2.0.5`, `rete-react-plugin@2.1.2`, React/React DOM 19.1.1, styled-components 6.1.19. esbuild 0.25.9 используется при сборке. | Минифицированный JS 310 334 байта плюс notices. Загрузка локальных HTML и JS, CSP запрещает сетевые подключения. Первое разрешение npm-зависимостей использует сеть. |
| Native Windows launcher | Собственный EXE + официальный CEF sandbox SDK 137.0.17 (BSD/Chromium notices), matching JBR/JCEF. SHA512 закреплён в Build-Launcher.ps1. | Release EXE 1 050 624 байт. SDK архив 202 011 285 байт — build-time cache, не runtime dependency; повторная упаковка offline после первичного разрешения. |
| C# language service | `csharp-ls@0.28.0`, MIT, Roslyn, net10.0. LSP4IJ 0.19.3 — кандидат EPL-2.0. В release notes этой версии явно указана поддержка Platform 253. | Изолированная установка csharp-ls 102 966 567 байт, требует .NET SDK. Проверен локальный процесс и проект без внешних package references. Размер и интеграция LSP4IJ в standalone — NOT RUN. [Пакет csharp-ls](https://www.nuget.org/packages/csharp-ls/0.28.0) · [LSP4IJ 0.19.3](https://github.com/redhat-developer/lsp4ij/releases/tag/0.19.3) · [Лицензия LSP4IJ](https://github.com/redhat-developer/lsp4ij/blob/main/LICENSE). |
| .NET и Rider | Установлены .NET SDK 10.0.402, hostfxr 10.0.13 для managed-пробы. Rider является отдельной установкой JetBrains. | .NET SDK и Rider используются как инструменты машины. Rider в проверенных путях не найден. [Синтаксис открытия файла](https://www.jetbrains.com/help/rider/Opening_Files_from_Command_Line.html). |

## A. Graph Canvas и JCEF

Официальный `jbr_jcef-21.0.10-windows-x64-b1163.108` интегрирован в штатную сборку. SHA512 проверяется перед извлечением; offline-источник — закреплённый архив. Runtime legal, JCEF/CEF/Chromium BSD и полные встроенные credits для 542 компонентов сохранены. Rete собирается локально с npm integrity lock; обычная упаковка использует `npm ci --offline`, первоначальное разрешение — `Build-Graph.ps1 -Resolve`.

Rete отображает настоящий GraphDefinition через Graph FileEditor → JBCefJSQuery → Java Bridge → private IPC v4 / DTO v1 → EditorWorkspace::Execute. Native owner выделяет IDs, проверяет типы и публикует snapshot. Op 38 допускает create/connect/disconnect/move/delete. Шаблоны Event/Native Call/Wait используют текущую native schema. Отдельного сохраняемого JS-графа нет.

Layout принадлежит EditorDocument, runtime GraphDefinition сохраняет семантический контракт. Отсутствующий layout v0 использует устойчивую сетку; архивный блок `graph_layout.version=1` хранит явные позиции. Неизвестные версии/узлы, дубликаты и недопустимые координаты отклоняются. MoveGraphNode проходит штатные native Undo/Redo, dirty и persistence.

В штатном EXE прошли browser/native authoring, React DOM, FileEditor close/reopen с disposal browser/query и отдельный HWND. Production Bridge test сохранил IDs/координаты при Save/Open во втором native-процессе. CSP и request handler запрещают внешние ресурсы, popup и OS protocol execution; JS Bridge ограничен allowlist, nonce, bound DocumentKey и generation/revision. Проверены отказ file/HTTPS/loopback fetch, eval и Bridge open. Полные результаты: [F-II.0b](FII0B_GRAPH_ROUNDTRIP.md).

**Решение launcher — реализовано и PASS.** Собственный Windows EXE статически связывает matching официальный CEF sandbox SDK 137.0.17 и создаёт CefScopedSandboxInfo до JVM. Дочерние Chromium процессы входят через публичный JBR jcef_helper.dll!execute_subprocess того же EXE, без JVM; native pointer передаётся hexadecimal вместе с точной CEF version. JNI classpath явно перечисляет locked JARs с продуктовым epidemic.jar первым, сохраняя приоритет bootstrap corrections DirectoryLock/MainImpl. JCEF работает in-process, Graph отвергает no_sandbox/remote и отсутствие зарегистрированного JNI. Unsandboxed smoke исключение удалено. SDK закреплён SHA512, используется offline из проверенного архива. [Контракты, pin и build](../../tools/editor-ui-host/standalone/native-launcher/README.md).

В штатном EXE renderer проверен OS API: restricted token, integrity RID 0, Win32kDisabled=true; job membership не единственное доказательство. AppContainer не заявляется. CSP и Bridge denial, disposal/reopen, отдельный HWND и отсутствие owned CEF процессов после shutdown прошли. Адресный maximized Windows input прошёл create/connect/reject/Escape/move/Ctrl+Z/Y/S, floating view и Save/Open в новом Editor-процессе. Удерживаемый drag/Escape/release отдельно проверен JCEF input APIs. Полный CU и 500/2000-node stress не заявляются.

## B. Code Space, LSP и Rider

- В запущенной Platform подтверждены `EditorFactory`, настоящий `Scripts/Turret.cs`, состояние dirty, Save через `FileDocumentManager`, явное обновление после внешней записи и release editor. Это тест API в диагностической копии с новым профилем. Встраивание в текущий Code Space, Ctrl+S, подсветка и внешний конфликт требуют отдельного теста.
- `csharp-ls 0.28.0` ответил на initialize и hover, вернул Roslyn `CS0029` для несохранённого текста. Completion, definition, references и rename объявлены сервером. Вызовы этих методов и подключение клиента LSP4IJ к продукту — NOT RUN. Для проверки процесса MSBuild требовался доступ к локальному именованному каналу.
- Предложенный порядок Rider discovery использует заданный `rider64.exe`, каталоги Toolbox, Windows App Paths и регистрации установки, затем PATH. Запуск передаёт аргументы `--line`, `--column` и абсолютный путь отдельно. Вариант при отсутствии Rider — встроенный редактор. При отсутствии LSP сохраняется редактирование и вывод `dotnet build` в Problems. Реальный запуск Rider — NOT RUN.

## C. Managed lifecycle

Существующий opt-in bridge вызывает `hostfxr_initialize_for_runtime_config`, получает `load_assembly_and_get_function_pointer` и загружает `GetApi` по assembly-qualified type. Сессия держит host и native callbacks. Вызов привязан к потоку открытия. `Close` освобождает callable и отказывает старому handle. Тесты проверяют reentry, Busy, callback lease, исключения, независимые сессии и освобождение managed callable.

Временный fixture собрал две net10.0 DLL. Native probe через штатный hostfxr вызвал `Turret.ApplyDamage` версии V1, получил 89 и 78, закрыл сессию и проверил отказ старого handle. V2 с другим assembly name и удвоенным уроном вернул 78 при первом вызове. После Close его handle также был отозван. Артефакт `Temp/U/fii0/turret/build/Debug/FiiTurretProbe.exe`.

В существующем загрузчике нет публичного контракта выгрузки DLL через collectible ALC. Для F-II.2 выбран отдельный managed worker на поколение Play и отзыв handles на Stop согласно окончательному контракту; реализация ещё не выполнена. Сохраняем действующий C ABI и проверяем загрузку новой версии отдельно от выгрузки старой. [Microsoft Assembly unloadability](https://learn.microsoft.com/en-us/dotnet/standard/assembly/unloadability).

## Исторические пробы F-II.0a (не текущая сборка)

- Базовый Release и CTest повторяются по командам из [baseline](FII_BASELINE.md).
- Локальные файлы браузера, npm lock и исходники диагностического расширения находятся в `Temp/U/fii0/rete` и `Temp/U/fii0`. Бандл собран через `esbuild probe.js --bundle --minify --legal-comments=linked --outfile=bundle.js`. Диагностический запуск использует копию app image, отдельный проверенный JBR и временные классы `PlatformStartup.java` / `FiiBrowserProbe.java`, собранные Microsoft JDK 21 с classpath штатного продукта. В копии JAR эти классы подключаются при старте, `EPIDEMIC_FII0_RETE_PAGE` указывает на локальный `rete/index.html`, запуск выполняется с `--smoke`. HTML содержит CSP с SHA-256 скрипта и `connect-src 'none'`.
- LSP проверен через `python Temp/U/fii0/lsp_probe.py` с локально установленным csharp-ls и SDK 10. Временная CMake-цель `FiiTurretProbe` линкуется с имеющимся Debug managed bridge и принимает hostfxr.dll и каталоги двух DLL.

## Решение на выходе F-II.0b

**PASS: native Windows launcher, настоящий CEF sandbox и native Graph roundtrip.** [Квалификация и подготовка F-II.1](FII0B_GRAPH_ROUNDTRIP.md).

[Контракт Code Space / managed](FII_CODESPACE_MANAGED_CONTRACT.md) закрепляет задачи F-II.2. LSP, Rider и C# Play сейчас не реализованы. F-II.1 READY; реализация не начата. Его расширенные Graph authoring/compile/VM и performance проверки остаются отдельным gate.

## D. F-II.1: принятое решение

Батч от `8aca4a9`: существующий Rete/React preset сохранён без новых frameworks или rete-engine. Selection/menu/minimap — transient проекция; native bounded clipboard и EditRequest формируют единую историю. Graph DTO v2 содержит catalogue/settings/diagnostics; optional graph_authoring v1 мигрирует независимо от прежнего graph_layout v1. Native VM preview использует GraphScheduler с изолированным Time/Simulation; gameplay NativeCall без owner явно unavailable, реальные domain bindings не подменяются. Inspector участвует в Save/close/selection draft pipeline. JCEF size/depth/rate и HTML encoding проверены; реальный CEF sandbox не менялся. [Квалификация, memory samples и ограничения](FII1_GRAPH_AUTHORING.md). F-II.1-GATE PASS по ручной приёмке 2026-11-14; реализация F-II.2 от `451ba62`.


F-II.1-GATE **PASS**: владелец подтвердил ручную приёмку 2026-11-14 («Проверил, всё работает»). Исходная реализация `451ba62`; ранее зафиксированные ограничения производительности и исторические static diagnostics сохраняются. F-II.2 начинается от этого коммита.


## E. F-II.2: реализованный профиль

Реальный Platform Editor/VFS без собственного Swing text editor; native described registry и project schema v1; Graph DTO v3, worker RPC v2 поверх managed ABI v1. Один worker на Play, scope/field/type/generation validation, Job Object/timeout, native live field authority. Выполнение сохранённого поколения при непроверенных/изменённых источниках явно обозначается. SDK и Rider внешние; user/project editor choices machine-local. Offline LSP fallback выбран по §5.3 спецификации: LSP candidate не включён до собственной квалификации. Schema v1 отвергает несовместимые поля/defaults; операции i64→i64. [Отчёт](FII2_SCHEMA_CODESPACE.md), [ручной gate](FII2_MANUAL_QA.md). F-II.3 требует отдельного typed-reference контракта и настоящего cross-HWND DnD; i64 токены нельзя выдавать за готовую реализацию ссылок.

## F-II.3 — текущий implementation batch

Native GraphReference payload v1, typed compiler/IR/VM resolver, atomic command 40 и Graph DTO v4 подключены к одному project-scoped Windows OLE DnD coordinator. [Контракт и фактические проверки](FII3_GRAPH_SCENE.md), [ручная квалификация](FII3_MANUAL_QA.md). F-II.3-GATE ожидает физической межоконной DnD/DPI проверки владельцем; NOT RUN не переименован в PASS.


## F-II.4 — native execution authority (17 ноября 2026)

Непрерывный Play исполняется одним C++ effect owner: PlaySession/Runtime.Time/Scheduler/Graph VM/ComponentStore. Java Bridge отправляет guarded commands и читает immutable bounded snapshots; Swing presenter и polling не продвигают игровое время и не исполняют Graph/C#. Runtime Inspector read-only. Actor transforms/Health/Sprite являются runtime данными, Stop/Restart не сохраняют их в authoring. Native step tick и cooperative step to node различаются по времени и интерфейсу.

Диагностика использует существующий ProfileScope/IProfileCollector SPI с forwarding прежнему collector, current native Renderer/process/resource providers; недоступные GPU/allocations/JCEF OS counters явно Unavailable. Не добавлен второй performance owner. [Подробный контракт](FII4_RUNTIME_CONTRACT.md), [проверки и измеренные бюджеты](FII4_REPORT.md). Функциональные automation PASS; окончательный F-II.4-GATE требует ручного подтверждения.

## F-II.5 — shared native gameplay (17 ноября 2026)

[Контракт](FII5_GAMEPLAY_CONTRACT.md): Game/Composition composes existing EntityService/ComponentStore/PrefabComposer/Scene/Runtime clock; no Java gameplay owner. Additive native Schema defaults and registered versioned operations preserve authoring formats. Private worker RPC v3 introduces bounded generation/scope-checked native calls from ScriptApi.Gameplay; public managed C ABI v1 and Editor IPC v4 remain unchanged. Scalar arguments retain explicit units and do not pretend to be typed Entity references; Arena also exercises the existing real GraphReference resolver. Minimal AABB/Circle overlap profile has documented rotation/trigger limits, no external physics dependency. Session-only progress does not overwrite authoring. F-II.5 implementation and functional qualification complete: native CTest162/162, standalone Release/JCEF, packaged Graph/C#/two-HWND and IPC/architecture PASS. Static retains inherited FAIL/backlog. Manual F-II.4/F-II.5 pending; see FII5_REPORT.md.
