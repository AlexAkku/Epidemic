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

---

## Как организованы независимые запуски

Auditor запускается отдельным Python/Tk процессом и не линкуется в движок как обычная игровая библиотека. Рабочая конфигурация находится в `.epidemic-auditor/auditor.config.json`; generated venv и bundle принадлежат `.ai/`, CMake native artifacts принадлежат `build/auditor-native/`. Для первого запуска native probes соберите соответствующие targets, поскольку `-SetupOnly` восстанавливает окружение и разворачивает уже существующие compiled helpers, но не гарантирует их первоначальную компиляцию.

```powershell
# Из корня исходников Epidemic
.\.epidemic-auditor\Start-Auditor.ps1 -SetupOnly
# Собрать native test helpers, если они еще не были собраны.
cmake --build build/auditor-native --config Release --target EpidemicAuditorObservation EpidemicAuditorShadowFixture EpidemicAuditorMemoryFixture --parallel 2
# Нужна заранее выполненная конфигурация build/auditor-native.
.\.epidemic-auditor\Start-Auditor.ps1
```

Не смешивайте «настроен Python venv», «собран native helper», «установлен privileged ETW broker» и «завершена полная квалификация». Это четыре различных состояния. Начальный режим Fast должен работать без Windows WPR/Verifier, но Deep и Memory имеют дополнительные требования и ограничения observation. Не включайте привилегии на постоянной основе ради удобства.

## Протокол проверки run

Выберите профиль и точный executable/test, убедитесь, что CTest selection действительно зарегистрирован. После Run проверьте exit status, manifest, timestamps, raw records, event loss и сравнимость binary identities. Затем откройте `Actions / Event Log`, выберите recorded action, raw evidence link и связанный graph. Если исходные records неполны, его profile может оставаться `UNKNOWN` или `INCOMPLETE` даже при успехе самого процесса.
