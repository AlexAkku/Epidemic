# Приёмка F-II.0

**F-II.0a PASS. F-II.0b BLOCKED. F-II.0 PARTIAL.**

Сквозной Graph authoring и native Save/Open во втором процессе прошли. Подтверждённый технический блокер — Windows CEF sandbox: jpackage launcher не инициализирует его до JVM. Обычный Graph FileEditor показывает Unavailable. Изолированный unsandboxed smoke с явным LIMIT не является security PASS.

После исправления launcher остаются адресные проверки **NOT RUN**:

1. Мышью создать/соединить Event, Native Call, Wait; несовместимое Control → Data должно отклоняться.
2. Переместить узел, Ctrl+Z/Ctrl+Y; начать новый drag, нажать Escape до отпускания мыши и проверить отсутствие native edit.
3. Настоящий клавиатурный фокус и Ctrl+S/Z/Y в основной Graph tab и отдельном окне Windows; возврат вкладки обратно.
4. Проверка фактического Windows sandbox, повтор security и bounded CEF shutdown на новом launcher.

Программно проверены JSQuery/native authoring, rejected edits, второй процесс, layout history, FileEditor disposal/reopen, независимый HWND, preferred focus API и CSP/ограниченный Bridge. Полный повтор U/CUA не требуется.

Code Space/LSP/Rider/managed Play реализуются в F-II.2. В текущей задаче требовалось закрепить их [контракты](FII_CODESPACE_MANAGED_CONTRACT.md), это выполнено.

[Точный отчёт, регрессии и подготовка F-II.1](FII0B_GRAPH_ROUNDTRIP.md).
