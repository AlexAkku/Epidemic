---
aliases: []
type: "guide"
source_snapshot: "2026-10-10"
---

# Установка и запуск Auditor

Из корня репозитория на Windows 11:

```powershell
# Подготовить изолированный Python venv и зависимости, не открывая GUI
.\.epidemic-auditor\Start-Auditor.ps1 -SetupOnly

# Открыть Tk GUI
.\.epidemic-auditor\Start-Auditor.ps1

# Запустить CLI без GUI после setup
.\.epidemic-auditor\Run-Auditor.ps1 discover -Mode Fast
.\.epidemic-auditor\Run-Auditor.ps1 test-all -Mode Fast
```

GUI предоставляет Configure, Build, Discover Tests, Run All Tests, Run Selected, Build+Run, Start Game, Stop; поддерживаются charts, Run views, Actions/Event Log и related investigations по совместимым captures. Не все функции доступны для каждого capture profile. `Start Game` требует заданного executable в `auditor.config.json`; в снимке поле `run.command` пустое, поэтому отсутствие Game launch из default конфига ожидаемо.

`Start-Auditor.ps1` создаёт venv в `.ai/venv/auditor`, устанавливает `psutil`. Native targets компилируются отдельно, предусмотрен bundle в `.ai/tools/auditor`. Для complete setup инструментированных профилей нужны MSVC/Windows инструменты из [[08 Auditor/Профили захвата]].

Скрипт `Self-Test.ps1` запускает `python -m unittest discover -s tests -v`. Его нужно выполнять без фильтров для полного текущего self-test; отдельные passing tests подтверждают только узкий scope.

[[08 Auditor/CLI и расследование]] · [[08 Auditor/Понимание evidence]].
