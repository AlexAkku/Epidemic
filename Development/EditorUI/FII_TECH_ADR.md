# F-II.0. Технический отчёт и ADR

**Статус F-II.0 — PARTIAL.** Базовая ревизия, инвентаризация и повторная квалификация составляют **F-II.0a PASS**. F-II.0b BLOCKED по Windows CEF sandbox; native Graph roundtrip PASS. По уточнённому заданию Code Space/managed закреплены контрактами для F-II.2. [Отчёт F-II.0b](FII0B_GRAPH_ROUNDTRIP.md). [Базовая ревизия](FII_BASELINE.md) · [Матрица функций](FII_FEATURE_MATRIX.md) · [Проверки](FII_QA_MATRIX.md) · [Приёмка](FII_ACCEPTANCE.md).

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

Официальный `jbr_jcef-21.0.10-windows-x64-b1163.108` интегрирован в штатную сборку. SHA512 проверяется перед извлечением; offline-источник — закреплённый архив. Runtime legal, JCEF/CEF/Chromium BSD и полные встроенные credits для 542 компонентов сохранены. Rete собирается локально с npm integrity lock; обычная упаковка использует `npm ci --offline`, первоначальное разрешение — `Build-Graph.ps1 -Resolve`.

Rete отображает настоящий GraphDefinition через Graph FileEditor → JBCefJSQuery → Java Bridge → private IPC v4 / DTO v1 → EditorWorkspace::Execute. Native owner выделяет IDs, проверяет типы и публикует snapshot. Op 38 допускает create/connect/disconnect/move/delete. Шаблоны Event/Native Call/Wait используют текущую native schema. Отдельного сохраняемого JS-графа нет.

Layout принадлежит EditorDocument, runtime GraphDefinition сохраняет семантический контракт. Отсутствующий layout v0 использует устойчивую сетку; архивный блок `graph_layout.version=1` хранит явные позиции. Неизвестные версии/узлы, дубликаты и недопустимые координаты отклоняются. MoveGraphNode проходит штатные native Undo/Redo, dirty и persistence.

В штатном EXE прошли browser/native authoring, React DOM, FileEditor close/reopen с disposal browser/query и отдельный HWND. Production Bridge test сохранил IDs/координаты при Save/Open во втором native-процессе. CSP и request handler запрещают внешние ресурсы, popup и OS protocol execution; JS Bridge ограничен allowlist, nonce, bound DocumentKey и generation/revision. Проверены отказ file/HTTPS/loopback fetch, eval и Bridge open. Полные результаты: [F-II.0b](FII0B_GRAPH_ROUNDTRIP.md).

**Блокер:** jpackage launcher на Windows не создаёт CEF sandbox до JVM. Закреплённый SettingsHelper обнаруживает отсутствие `jcef.sandbox.ptr` и выключает sandbox. Обычный Graph FileEditor поэтому показывает Unavailable; только изолированный `--smoke` допускает локальную unsandboxed квалификацию с явным LIMIT. Нужен native launcher с настоящим sandbox info в основном EXE, совместимой CEF sandbox library и проверкой Windows renderer mitigations. Фиктивный указатель не используется. [CEF sandbox setup](https://chromiumembedded.github.io/cef/sandbox_setup).

Реальные mouse/keyboard gestures и Escape во время drag пока NOT RUN. Передача preferred focus JCEF реализована и проверена на уровне FileEditor API. До устранения sandbox F-II.1 закрыт.

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

## Решение на выходе F-II.0b

**BLOCKED: Windows CEF sandbox в launcher.** Native Graph roundtrip PASS. [Результаты и оставшиеся проверки](FII0B_GRAPH_ROUNDTRIP.md).

[Контракт Code Space / managed](FII_CODESPACE_MANAGED_CONTRACT.md) закрепляет задачи F-II.2. LSP, Rider и C# Play сейчас не реализованы. F-II.1 подготовлен, реализация не начата до PASS F-II.0b.
