# Приёмка F-II.0

**F-II.0a PASS. F-II.0b PASS. F-II.0 PASS.**

Сквозной Graph authoring работает в штатном самостоятельном Editor с настоящим Windows CEF sandbox. Native launcher создаёт sandbox до JVM; renderer имеет restricted token, Untrusted integrity и Win32k lockdown. Unsandboxed smoke исключение удалено.

| Обязательный результат F-II.0b | Статус |
|---|---|
| Native GraphDefinition через Rete/Java Bridge/IPC v4/Execute; узлы/порты/связи | PASS |
| Native create/connect/type reject, координаты, миграция, Undo/Redo, Save/Open в другом процессе | PASS |
| Официальный JBR/JCEF, проверяемый offline cache, SHA512, notices | PASS |
| Настоящий sandbox, CSP/file/network/eval/Bridge ограничения | PASS |
| FileEditor close/reopen, независимое Windows окно, keyboard focus, bounded shutdown | PASS |
| Мышь: compatible/rejected connections, Escape; клавиатура Ctrl+S/Z/Y в основной и отдельной view | PASS, адресный Windows сценарий |
| Удерживаемый node drag→Escape→release; следующий drag | PASS через реальные JCEF input APIs: ноль edits, затем ровно один commit |
| Адресные 19 CTest/архитектура, IPC/catalog, два launcher TU static analysis, startup lock regression | PASS |
| Code Space и managed lifecycle | PASS по окончательным контрактам F-II.2; LSP/Rider/C# Play не реализованы |

Полный U/Computer Use прогон не выполнялся. Нагрузочные 500/2000 nodes и расширенное редактирование/compile/VM составляют F-II.1 и пока NOT RUN. Исторические анализаторные предупреждения остаются backlog.

[Отчёт и подготовленный вход F-II.1](FII0B_GRAPH_ROUNDTRIP.md) · [Контракты F-II.2](FII_CODESPACE_MANAGED_CONTRACT.md).
