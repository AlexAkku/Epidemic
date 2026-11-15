# F-II.2: Schema, C# и Code Space

Отправная точка: `451ba62` (`v2`). F-II.1-GATE PASS подтверждён владельцем 14 ноября 2026 года. Computer Use не используется; push не выполняется.

## Реализация

- Code Space использует настоящий Platform Editor/Document и VFS. Один канонический `.cs` разделяет Document между окнами; Undo текста остаётся у Platform, история authoring — у native Workspace. Есть offline-подсветка, поиск, переход к строке, Save/Save All, Save/Discard/Cancel и конфликт внешней записи с Compare/Reload/Save As. Автосохранение тоже проверяет fingerprint.
- В Project Explorer отображаются реальные Scripts; доступны открытие, Open With и создание C# класса. Type Registry получает данные из native ComponentRegistry. Настройки built-in/Rider/custom и проектное переопределение хранятся в локальных пользовательских preferences; физический EXE-путь не записывается в проект.
- Явный Build & Register работает вне EDT, сохраняет sources, проверяет containment и fingerprints, поддерживает отмену, выдаёт diagnostics file:line:column. Проектные Directory.Build props/targets не импортируются. Сборка использует собственный derived csproj, offline NuGet.Config и локальный ScriptApi; пользовательский csproj не переписывается. Shared compilation/node reuse отключены.
- ScriptApi предоставляет атрибуты ScriptComponent, ScriptField, GraphOperation. Экспорт поддерживает четыре scalar-типа: i64/double/bool/string. Native ProjectDefinition хранит versioned descriptors и относительный путь/SHA256 assembly; данные компонентов проходят существующие команды SetComponent/SetProperty, Undo/Redo и архив проекта. Validation использует временный реестр; публикация поколения и восстановление реестра при Undo/Redo согласованы.
- Palette строится из native schema, не из отдельной модели Java/JS. Сигнатура C# операции текущего Graph ABI — публичный instance-метод `long Method(long)`. Runtime проверяет canonical component ID, operation ID, hash assembly, поколение, типы полей и scope. Native GraphCompiler/VM/Scheduler вызывают существующий managed ABI v1 через отдельный worker; второй simulation/Graph VM не создаётся.
- На поколение Play используется один worker для всех методов/объектов. Authoring defaults и runtime values разделены; native ComponentStore передаёт и принимает live-поля. Stop отзывает worker, старые handles и owned process tree. Есть timeout вызова, ограничение памяти процесса и явная ошибка при исключении/несовместимости. Изолированный Graph Preview имеет собственный короткоживущий worker.
- Open C# Source из Node Inspector использует portable PDB; Find Graph Uses показывает ссылки native graph nodes. Ошибка SDK не заменяет последнюю зарегистрированную schema. Источники после изменения помечаются Rebuild Required; запуск сохранённого поколения требует явного выбора, если источники не проверены текущей успешной сборкой.

## Контракты и ограничения

IPC envelope остаётся v4; Graph DTO v3 добавляет source/line к template. Команда 39 регистрирует ограниченный schema-export. Schema-export и persisted component schema имеют version 1; внутренний worker RPC — v2, существующий managed ABI — v1.

Schema v1 сохраняет идентификаторы через явные canonical IDs. Изменение состава, порядка, типа или default полей требует явной миграции/нового canonical type ID; несовместимая регистрация отвергается с сохранением прежних данных. Удаление используемого типа/операции также не удаляет данные молча. Новая реализация метода с прежней сигнатурой/IDs регистрируется без rebuild Editor.

Source policy: Scripts внутри проекта, до 1 MiB на source, до 512 C# файлов/4096 элементов дерева; escape через reparse/symlink отвергается. Source set/hash проверяются после компиляции; устаревший результат не выдаётся за текущую сборку. Build не выполняется при открытии проекта. C# — доверенный код, worker/ALC не объявляются security sandbox. CEF sandbox и JS Bridge не получают файлового/SDK RPC.

LSP не поставляется. Спецификация §5.3 допускает offline fallback; completion/hover/definition/references/rename через language server не заявляются. Закреплённые кандидаты LSP из F-II.0 остаются backlog. Реальное взаимодействие с установленным Rider и физическая работа с двумя окнами относятся к адресной ручной приёмке.

Typed Entity/Component/Asset/Graph ports, расширенные сигнатуры, subscriptions/events API, полноценный debugger и живой Game View не объявляются реализованными этим scalar/unary профилем; соответствующие этапы F-II.3–F-II.5 продолжаются отдельно. Native graph limit 1024 не изменён.

## Квалификация

Полный Debug CTest: **161/161 PASS**, включая архитектуру/public headers и C++-only профиль (`EPIDEMIC_BUILD_MANAGED_BRIDGE=OFF`). IPC v4 adversarial 16 cases и исходный UI catalog PASS. Существующие native managed bridge/contract проверки PASS.

Native SDK/schema/Graph roundtrip PASS: V1/V2, stable IDs, native VM, Save/Open новым процессом, malformed export rejection. Headless Play проверяет два C# метода, изменение native live-компонента, три Play/Stop, один worker, отсутствие дочернего worker после Stop и отказ старого handle.

Standalone smoke проверяет реальный Source Editor, Unicode Save, внешнюю dirty-запись, Reload и bounded release при close/reopen. Штатный Build & Register проверяется отдельно от прямого SDK-теста: native publication, compiler diagnostics, failed-build last-good preservation, запрет Directory.Build.targets и native Registry Undo. Во время аудита исправлена настоящая гонка отложенной очистки owner при повторном открытии Source Editor; тест утечки не исключён.

Static: штатный Verify-Changed Fast проверил 73 TU/6 headers; итог FAIL из-за прежних предупреждений в неизменённом коде. Семь новых диагностик (move захватов, lifetime, ширина/битовые операции/WinAPI cast) исправлены; focused repeat компонентов и backend с прежними checks/header scope не содержит ошибок в изменённых местах. Остались baseline Scene signed-bitwise, Composition/Editor constructors, ThumbnailCache parameters, native_viewport float/switch и тестовые narrowing; подавлений нет. Финальный самостоятельный Release smoke и native SDK/Graph/Core повтор PASS. Проверенный образ: `Temp/U/verification-images/74e9d7b7c4cc434a94c5b7fba6d49eb2/EpidemicEditor/EpidemicEditor.exe`. **F-II.2-GATE не объявлен PASS до ручной приёмки владельца.**

[Ручная проверка](FII2_MANUAL_QA.md).

