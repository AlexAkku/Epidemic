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

Полный U/Computer Use прогон не выполнялся. На выходе F-II.0 нагрузочные 500/2000 nodes и расширенное редактирование/compile/VM оставались NOT RUN; текущая квалификация F-II.1 приведена ниже. Исторические анализаторные предупреждения остаются backlog.

[Отчёт и подготовленный вход F-II.1](FII0B_GRAPH_ROUNDTRIP.md) · [Контракты F-II.2](FII_CODESPACE_MANAGED_CONTRACT.md).

## F-II.1

Реализация завершена; функциональная автоматическая квалификация PASS. **F-II.1-GATE ожидает ручную приёмку владельца (NOT RUN)**. [Результат, static diagnostics и ограничения](FII1_GRAPH_AUTHORING.md) · [Пошаговая инструкция](FII1_MANUAL_QA.md). Полный Computer Use не запускался; 2000-node render не заявляется. NativeCall preview требует gameplay scope; C#/Variables остаются F-II.2/3.
