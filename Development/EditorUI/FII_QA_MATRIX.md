# Проверки F-II

**F-II.0a PASS. F-II.0b BLOCKED: Windows CEF sandbox в launcher.**

| Проверка F-II.0b | Статус | Что действительно проверено |
|---|---|---|
| Native Graph через production Java Bridge / IPC v4 | PASS | Три узла, полный DTO портов/рёбер, совместимое соединение, отказ несовместимого/неизвестных endpoints, stale revision. `tests/GraphTests.java`. |
| Move, Undo/Redo, Save/Open в другом native-процессе | PASS | Стабильные IDs, связи и точные double-координаты; Undo удаления восстанавливает incident edges и layout. `.ai/logs/fii0b-qualification.stdout.log`. |
| Миграция layout | PASS | Реальный архив без layout читается, повторная сериализация стабильна; v1 сохраняет координаты, v2 отклоняется. `EpidemicEditorCoreTests`, `.ai/logs/fii0b-layout-migration.stdout.log`. |
| Rete → JSQuery → Java Bridge → Execute | PASS | Программные команды браузера создали native-узлы/ребро, move/history/Save; React DOM отобразил ответный native snapshot. |
| FileEditor close/reopen и независимое окно | PASS | Browser/query disposed, новая view создана; независимый HWND проверен Platform API. Это не mouse docking test. |
| JCEF shutdown | PASS | После штатного EXE smoke нет принадлежащих image `cef_server` / `jcef_helper` в пределах 15 секунд. |
| CSP / ресурсы / ограниченный Bridge | PASS в локальном smoke | File, HTTPS, loopback fetch и eval отклонены; Bridge open отклонён без смены revision. Nonce/origin/DocumentKey guards реализованы; это не OS sandbox PASS. |
| Official JBR/JCEF, offline build, notices | PASS | Publisher SHA512, Maven lock 353 artifacts, npm ci offline, локальный bundle, JBR legal и complete Chromium credits для 542 компонентов. |
| Адресные CTest / архитектура | PASS | 19/19, свежие Editor Core/Workspace/SceneView/Headless builds; Graph VM/serialization/schema/renderer и архитектурные проверки. `.ai/logs/fii0b-regressions.stdout.log`. |
| IPC framing / catalog | PASS | 16 protocol cases и покрытие каталога. Native envelope v4 сохранён, Graph DTO имеет свою версию 1. |
| Focus delegation | PASS API / NOT RUN физический ввод | Финальный standalone smoke проверил preferred focused component JCEF; клавиатурный ввод в Windows не подменён программной проверкой. |
| Реальный Windows CEF sandbox | BLOCKED | SettingsHelper выключает его без native launcher sandbox info. Обычный Graph Unavailable; smoke разрешает только изолированную локальную пробу с явным LIMIT. |
| Mouse connection/move/cancel и keyboard input | NOT RUN | Требуется адресная квалификация после исправления launcher; программные сообщения не выданы за физические жесты. |
| 500 nodes / idle metrics / расширенный multiwindow stress | NOT RUN | Не закрыты одним roundtrip и bounded shutdown. |
| Полный CTest | NOT RUN в F-II.0b | В этой задаче адресная выборка. Старые 161/161 относятся к F-I. |
| Статические анализаторы | KNOWN DIAGNOSTICS | clang-analyzer/bugprone на трёх изменённых TU; новая integer-division диагностика grid исправлена. Существующие signed-bitwise в Scene, exception-escape/unsafe-to-allow-exceptions в Composition/EditorDocument/CreateDocument и easily-swappable-parameters в ThumbnailCache остаются; checks не ослаблены. |

Предыдущие отдельные EditorFactory/LSP/managed API пробы остаются результатами [F-II.0a baseline](FII_BASELINE.md), а не реализованным Code Space или C# Play. Их новые контрактные задачи закреплены в [F-II.2](FII_CODESPACE_MANAGED_CONTRACT.md).

[Точный результат F-II.0b и команды](FII0B_GRAPH_ROUNDTRIP.md).
