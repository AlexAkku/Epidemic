---
aliases: []
type: "guide"
source_snapshot: "2026-10-10"
---

# Auditor CLI: run, view, compare, export

Рабочий каталог CLI в примерах ниже: `.epidemic-auditor/` после `Start-Auditor.ps1 -SetupOnly`. В PowerShell `$py` указывает на созданный venv.

```powershell
Set-Location .\.epidemic-auditor
$py = '..\.ai\venv\auditor\Scripts\python.exe'

& $py -B -m ep_auditor.cli --mode Fast discover
& $py -B -m ep_auditor.cli --mode Fast test-all

# Исследовать уже завершённый sealed Run, не стартуя binary заново
& $py -B -m ep_auditor.cli view '<run-directory>' 'Dashboard' --limit 50
& $py -B -m ep_auditor.cli event-log '<run-directory>' --offset 0 --limit 50
& $py -B -m ep_auditor.cli action '<run-directory>' '<ActionId>'
& $py -B -m ep_auditor.cli compare '<baseline-run>' '<current-run>' --limit 100
& $py -B -m ep_auditor.cli export '<run-directory>'
& $py -B -m ep_auditor.cli investigate '<run-directory>' 'Action' '<ActionId>'
```

Команды `view`, `compare`, `investigate`, `history`, `stress`, `view-field` обнаружены в `ep_auditor/cli.py` данного снимка. Значения имен `view`/`kind` проверяются backend; перед автоматизацией опирайтесь на доступные Run IDs и вывод конкретной версии.

`stress --spec` читает JSON campaign configuration с конечными duration/budgets. Проверьте его на отдельном контролируемом workload и не допускайте глобальных injection вне owned process tree.

`Run-Auditor.ps1` является более узкой обёрткой с параметром `Command` (`configure`, `build`, `discover`, `test-all`, `test`, `run`, `build-run`, `static`) и `-Mode Fast/Full/Deep`; для read-only анализов используйте Python CLI непосредственно.

---

## От RAW к диагностике

Проверяйте sealed run, затем `python -B -m ep_auditor.cli event-log <sealed-run> --offset 0 --limit 50`, после выбора существующего ActionId используйте `python -B -m ep_auditor.cli action <sealed-run> <ActionId>`. Оба пути читают те же immutable records; они не создают потраченных во время запуска точных входов/выходов функций задним числом. Для сравнения run предварительно сверяйте compiler, profile, symbols и capture scope.
