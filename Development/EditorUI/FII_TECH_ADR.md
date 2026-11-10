# F-II.0. Технический отчёт и ADR

**Статус F-II.0 — PARTIAL.** Базовая ревизия, инвентаризация и повторная квалификация составляют **F-II.0a PASS**. Полный сквозной граф из §3.1 и интеграция Code Space из §3.2 остаются условиями входа в F-II.1. [Базовая ревизия](FII_BASELINE.md) · [Матрица функций](FII_FEATURE_MATRIX.md) · [Проверки](FII_QA_MATRIX.md) · [Приёмка](FII_ACCEPTANCE.md).

## Зависимости на Windows 11 x64

| Зависимость | Версия и лицензия | Размер и offline |
|---|---|---|
| IntelliJ Platform | 253.31033.145, 353 закреплённых артефакта с индивидуальными LICENSE/NOTICE/POM в продукте. | `app` базового EXE занимает 378 572 966 байт без сгенерированного smoke-профиля. Пакет работает с локальным runtime и локальным companion. |
| Vanilla JBR | 21.0.10+1-b1163.108, OpenJDK GPLv2 с Classpath Exception и сопроводительные notices. | Пакетный `runtime` 166 469 319 байт. В запущенной Platform `JBCefApp.isSupported() == false`. |
| JBR с JCEF | Публичный Windows x64 `jbr_jcef-21.0.10-windows-x64-b1163.108`. Проверен SHA512 издателя `f231d37bf2f358486bc38c21931651b2240a5d13c86b6b86467e43bd5c4db50e4e5585f1e4515ab7fd7b70b05efeaaef644a692799f52bc0270d9e8aba6cb34a`. Лицензии JBR плюс уведомления CEF/Chromium требуют инвентаризации перед распространением. | Архив 259 461 195 байт, распакованный runtime 541 529 756 байт. В архиве есть `jcef.dll`, `libcef.dll`, `chrome_elf.dll`, helpers, `*.pak` и ICU. [Публикация JetBrains](https://github.com/JetBrains/JetBrainsRuntime/releases/tag/jbr-release-21.0.10b1163.108). |
| Rete и renderer | MIT. `rete@2.0.6`, `rete-area-plugin@2.3.2`, `rete-connection-plugin@2.0.5`, `rete-react-plugin@2.1.2`, React/React DOM 19.1.1, styled-components 6.1.19. esbuild 0.25.9 используется при сборке. | Минифицированный JS 308 537 байт плюс notices. Загрузка локальных HTML и JS, CSP запрещает сетевые подключения. Первое разрешение npm-зависимостей использует сеть. |
| C# language service | `csharp-ls@0.28.0`, MIT, Roslyn, net10.0. LSP4IJ 0.19.3 — кандидат EPL-2.0. В release notes этой версии явно указана поддержка Platform 253. | Изолированная установка csharp-ls 102 966 567 байт, требует .NET SDK. Проверен локальный процесс и проект без внешних package references. Размер и интеграция LSP4IJ в standalone — NOT RUN. [Пакет csharp-ls](https://www.nuget.org/packages/csharp-ls/0.28.0) · [LSP4IJ 0.19.3](https://github.com/redhat-developer/lsp4ij/releases/tag/0.19.3) · [Лицензия LSP4IJ](https://github.com/redhat-developer/lsp4ij/blob/main/LICENSE). |
| .NET и Rider | Установлены .NET SDK 10.0.402, hostfxr 10.0.13 для managed-пробы. Rider является отдельной установкой JetBrains. | .NET SDK и Rider используются как инструменты машины. Rider в проверенных путях не найден. [Синтаксис открытия файла](https://www.jetbrains.com/help/rider/Opening_Files_from_Command_Line.html). |

## A. Graph Canvas и JCEF

Официальная сборка JBR с JCEF того же релиза подтверждена для пробы F-II.0b. В копии EXE с этим runtime проверка поддержки вернула true, Chromium 137 загрузил локальный Rete внутри Graph FileEditor. React DOM содержал три узла и одно ребро. Java получила результат через JBCefJSQuery и освободила browser и query. Размер runtime диагностической копии с CRT — 542 829 324 байта. Product-слой уже содержит `ui-jcef:253.31033.145` и Java API JCEF, зафиксированный в lock. Для интеграции нужен feature capability и экран Unavailable с результатом проверки JCEF и диагностикой runtime. [IntelliJ JCEF SDK](https://plugins.jetbrains.com/docs/intellij/embedded-browser-jcef.html).

Выбран `rete-react-plugin` с классическим preset. Он поддерживает документированный API render/area/connection и совместим с закреплённым React 19. Для первой пробы подходят стандартные nodes, sockets и edges. Альтернативные renderer plugins нужны только при обнаружении конкретного ограничения. Бандл спайка использует техническую схему из трёх узлов с портами i64 и одним ребром. [Руководство Rete React](https://retejs.org/docs/guides/renderers/react/) · [Лицензия Rete](https://github.com/retejs/rete/blob/main/LICENSE) · [Лицензия renderer](https://github.com/retejs/react-plugin/blob/main/LICENSE).

Native roundtrip и browser probe проверяют отдельные части. Диагностическая копия подставляет только временные классы продукта и runtime. Техническая схема Rete не является native snapshot. Из первой попытки сохранён лог инициализации JCEF в remote-режиме и сообщение о выключенном sandbox в Java-процессе. Параметры sandbox и перечень notices открыты перед продуктовой упаковкой. Сразу после выхода EXE диагностический запрос видел четыре процесса из каталога пробы. При повторном запросе они завершились, точное время завершения не измерено. Для полного сквозного пути требуются DTO портов и соединений и команда из JavaScript, проходящая через JVM и текущий private IPC в `EditorWorkspace::Execute`. Координат в `GraphDefinition` сейчас нет. Хранение координат и их Undo/Redo нужно согласовать в узком редакторском контракте вместе с миграцией старых проектов.

## B. Code Space, LSP и Rider

- В запущенной Platform подтверждены `EditorFactory`, настоящий `Scripts/Turret.cs`, состояние dirty, Save через `FileDocumentManager`, явное обновление после внешней записи и release editor. Это тест API в диагностической копии с новым профилем. Встраивание в текущий Code Space, Ctrl+S, подсветка и внешний конфликт требуют отдельного теста.
- `csharp-ls 0.28.0` ответил на initialize и hover, вернул Roslyn `CS0029` для несохранённого текста. Completion, definition, references и rename объявлены сервером. Вызовы этих методов и подключение клиента LSP4IJ к продукту — NOT RUN. Для проверки процесса MSBuild требовался доступ к локальному именованному каналу.
- Предложенный порядок Rider discovery использует заданный `rider64.exe`, каталоги Toolbox, Windows App Paths и регистрации установки, затем PATH. Запуск передаёт аргументы `--line`, `--column` и абсолютный путь отдельно. Вариант при отсутствии Rider — встроенный редактор. При отсутствии LSP сохраняется редактирование и вывод `dotnet build` в Problems. Реальный запуск Rider — NOT RUN.

## C. Managed lifecycle

Существующий opt-in bridge вызывает `hostfxr_initialize_for_runtime_config`, получает `load_assembly_and_get_function_pointer` и загружает `GetApi` по assembly-qualified type. Сессия держит host и native callbacks. Вызов привязан к потоку открытия. `Close` освобождает callable и отказывает старому handle. Тесты проверяют reentry, Busy, callback lease, исключения, независимые сессии и освобождение managed callable.

Временный fixture собрал две net10.0 DLL. Native probe через штатный hostfxr вызвал `Turret.ApplyDamage` версии V1, получил 89 и 78, закрыл сессию и проверил отказ старого handle. V2 с другим assembly name и удвоенным уроном вернул 78 при первом вызове. После Close его handle также был отозван. Артефакт `Temp/U/fii0/turret/build/Debug/FiiTurretProbe.exe`.

В существующем загрузчике нет публичного контракта выгрузки DLL через collectible ALC. Для F-II.2 требуется выбрать модель поколений и срок жизни сборки относительно Play/Stop. Сохраняем действующий C ABI и проверяем загрузку новой версии отдельно от выгрузки старой. [Microsoft Assembly unloadability](https://learn.microsoft.com/en-us/dotnet/standard/assembly/unloadability).

## Воспроизводимость проб

- Базовый Release и CTest повторяются по командам из [baseline](FII_BASELINE.md).
- Локальные файлы браузера, npm lock и исходники диагностического расширения находятся в `Temp/U/fii0/rete` и `Temp/U/fii0`. Бандл собран через `esbuild probe.js --bundle --minify --legal-comments=linked --outfile=bundle.js`. Диагностический запуск использует копию app image, отдельный проверенный JBR и временные классы `PlatformStartup.java` / `FiiBrowserProbe.java`, собранные Microsoft JDK 21 с classpath штатного продукта. В копии JAR эти классы подключаются при старте, `EPIDEMIC_FII0_RETE_PAGE` указывает на локальный `rete/index.html`, запуск выполняется с `--smoke`. HTML содержит CSP с SHA-256 скрипта и `connect-src 'none'`.
- LSP проверен через `python Temp/U/fii0/lsp_probe.py` с локально установленным csharp-ls и SDK 10. Временная CMake-цель `FiiTurretProbe` линкуется с имеющимся Debug managed bridge и принимает hostfxr.dll и каталоги двух DLL.

## Следующая компактная задача F-II.0b

Открыть сохранённый Graph на копии проекта с тремя узлами и реальной связью. Добавить одну команду `ConnectPorts` из Graph tab, подтверждать результат новым native snapshot и проверить Undo/Redo, Save/Open в другом процессе. Один gestural move потребует отдельно согласованного хранения layout. Изолировать пробу за capability.

| Участок | Точные файлы и проверка |
|---|---|
| Native snapshot, guard и EditRequest | `tools/editor-ui-host/backend.cpp`, `Game/Editor/include/Epidemic/Game/Editor/editor_commands.h`, `Game/Editor/src/editor_commands.cpp`. Порты и рёбра с обоими IDs, expected generation/revision, проверка типов, отказ неверных endpoints. CTest `EpidemicRuntimeGraphTests`, `EpidemicGraphSerializationTests`, `EpidemicEditorCoreTests`, `EpidemicEditorWriteProject`, `EpidemicEditorReadPlayProject`. |
| JVM и браузер | `tools/editor-ui-host/java/epidemic/editor/Bridge.java`, `tools/editor-ui-host/java-intellij/epidemic/editor/platform/PlatformSession.java`, `PanelEditorProvider.java`. Малый JCEF adapter, owned Disposable на FileEditor и строковые ID для JavaScript. `tools/editor-ui-host/tests/test_protocol.py` проверяет границы фрейма, дополнительные тесты проверят несовместимую версию DTO и stale revision. |
| Release и лицензии | `tools/editor-ui-host/Resolve-Runtime.ps1`, `Build-Standalone.ps1`, `standalone/dependencies.lock.json` и notices при изменении состава. Публичный JBR с проверяемым checksum. Повтор упаковки, запуск без CDN, закрытие tab и shutdown. |
| Ручная приёмка | Graph tab в самостоятельном HWND, перенос обратно, мышь и клавиатурный фокус. До подтверждения эти сценарии остаются NOT RUN. |
