---
aliases: []
type: "guide"
source_snapshot: "2026-10-19"
---

# Установка и запуск Auditor

Auditor работает отдельным процессом. Можно наблюдать готовую игру через
**Start Game**, а можно отдельно Configure/Build инструментированный проект.
Готовый EXE не нужно пересобирать только ради метрик процесса и stdout/stderr.

## Подготовка UI и native helpers

Из корня исходников движка, Windows x64, PowerShell, Python 3.11+ с Tk и MSVC:

```powershell
$engineRoot = (Get-Location).Path
# Setup меняет рабочий каталог, поэтому следующие пути снова задаём от корня.
& "$engineRoot/.epidemic-auditor/Start-Auditor.ps1" -SetupOnly
Set-Location $engineRoot
$auditorPython = Join-Path $engineRoot '.ai/venv/auditor/Scripts/python.exe'
cmake -S .epidemic-auditor/native -B build/auditor-native `
  -G "Visual Studio 18 2026" -A x64 "-DPython3_EXECUTABLE=$auditorPython"
cmake --build build/auditor-native --config Release --target EpidemicAuditor --parallel 2
```

После каждой команды проверяйте `$LASTEXITCODE`; при ошибке остановитесь.
Generator заменяется на установленный у вас Visual Studio generator.
`-SetupOnly` готовит Python/psutil и восстанавливает bundle из уже собранных helpers,
но не компилирует их впервые. Target `EpidemicAuditor` собирает launcher,
Observation и ETW broker, затем разворачивает приватный runtime в `.ai/tools/auditor`.
Fixture targets нужны для self-test соответствующих возможностей, не для обычного
запуска игровой копии. Установка broker как системного сервиса — отдельный шаг
для ETW/WPR; сборка target сама сервис не устанавливает.

## Наблюдать готовую игру, сохранив исходную сборку

Скопируйте полный runnable-каталог в `LocalGame/<game-version>/`: EXE, нужные DLL,
внешние ресурсы/локализации и при наличии соответствующие PDB. Оставьте оригинал
на месте. Изолируйте профиль средствами клиента; у ASCENT это `profile-scope.txt`
с отдельным GUID либо smoke-прогон с отдельным `--capture-dir`.

Создайте `.ai/game-ui/auditor.config.json`. Пример для layout, где Game и
EpidemicDev — соседние каталоги; абсолютные пути замените на свои:

```json
{
  "project_root": "E:/workspace/Game",
  "build_dir": "E:/workspace/EpidemicDev/build/game-observation/audit",
  "cmake": {"build_config": "Release", "parallel": 2},
  "run": {
    "command": ["E:/workspace/EpidemicDev/LocalGame/ASCENT/Pdoom.exe"],
    "working_dir": "E:/workspace/EpidemicDev/LocalGame/ASCENT",
    "environment": {}
  },
  "capture": {"selected_mode": "Full", "sample_interval_ms": 200},
  "storage": {
    "runs_dir": "E:/workspace/EpidemicDev/.ai/game-ui/runs",
    "database": "E:/workspace/EpidemicDev/.ai/game-ui/auditor.sqlite3",
    "keep_raw_console": true,
    "auto_export_agent_pack": false
  },
  "static_scan": {
    "roots": ["E:/workspace/Game/Source"],
    "extensions": [".h", ".hpp", ".cpp"],
    "exclude": ["build", ".git", ".ai", "vendor"]
  }
}
```

`project_root` — каталог CMake-проекта игры, а не папка с EXE. `build_dir` здесь
зарезервирован для отдельной диагностической сборки; **Start Game** не требует
её Configure/Build. `run.command` — массив EXE и отдельных аргументов, не строка
командной оболочки. Относительные пути разрешаются от каталога конфига.
`auto_export_agent_pack=false` отключает автоматический большой экспорт после
операций, сохраняя сбор и проверку capture; явный экспорт остаётся доступен.

Открыть native UI с этим конфигом:

```powershell
& "$engineRoot/.ai/tools/auditor/EpidemicAuditor.exe" `
  --config "$engineRoot/.ai/game-ui/auditor.config.json"
```

1. Проверьте Capture: **Full**, нажмите **Start Game**. Не нажимайте Build+Run,
   если собираетесь только наблюдать уже готовый EXE.
2. Дождитесь окна игры, выполните нужный сценарий и штатно выйдите из игры.
   **Stop** прерывает процесс; это другой результат, чем обычное завершение.
3. После **Finished: start-game** откройте **Runs**, выберите строку запуска,
   нажмите **Load Dashboard**. Dashboard читает завершённый проверенный capture.
4. Для метрик процесса откройте **Threads**, нажмите **Load** для выбранного run,
   затем выберите строку PID: справа доступны CPU, RSS, threads, I/O и ссылка на
   raw evidence. **Next page** открывает следующие выборки; одна страница не весь run.

## Что действительно проверено и чего ожидать от UI

На ASCENT 0.1.19d этот маршрут проверен через реальную кнопку Start Game в режиме
Full: 431 выборка за примерно 102 секунды, штатный exit code 0. В Runs показан
PASS, в Threads — числовые данные `Pdoom.exe`. Это проверка запуска и наблюдения
процесса, а не доказательство отсутствия игровых ошибок или утечек.

На проверенной версии UI вкладка **Process summary** осталась пустой; график
Threads сообщил об отсутствии числового ряда, хотя справа в выбранных строках
были CPU/RSS/threads. Просматривайте строки Threads и их raw evidence. Эти
ограничения UI не означают, что сбор метрик не происходил. При запуске также
наблюдался диалог дочернего `git.exe` с кодом `0xc0000142`; сеанс завершился после
закрытия диалога. Причина в этом прогоне не установлена, отключать Git как
универсальное решение не следует.

Переключатель Capture не добавляет ASan, coverage или exact hooks в готовый EXE.
Для них нужны инструментированные исходники/бинарники соответствующего профиля,
для ETW — WPR и работающий ограниченный broker. Недоступные категории остаются
UNKNOWN/BLOCKED; process PASS и отсутствие событий не доказывают безопасность.

## Если нужна диагностическая пересборка

Ошибка `build directory lacks Auditor ownership; normal cache cannot be adopted`
означает, что выбрана обычная CMake-сборка, которую Auditor не создавал. Сохраните
её и укажите новый пустой `build_dir`, затем выполняйте Configure из Auditor.
Не создавайте ownership-файлы вручную и не копируйте CMakeCache из обычной сборки.

Режим выбирает физический профиль: Fast/Full → `audit`, Deep/Stress/Targeted Exact
→ `audit-deep`, Memory → `audit-memory`, Coverage → `audit-coverage`.
Чтобы запускать бинарник именно выбранной сборки, используйте явный шаблон
`{build_dir}` в `run.command`, например `{build_dir}/Release/MyGame.exe` для
соответствующего Visual Studio target. Путь зависит от output layout клиента;
Auditor не ищет EXE автоматически. Фиксированный путь к готовой копии остаётся
фиксированным при переключении режима. Тот же шаблон поддерживают working_dir
и значения run.environment.

`capture.api_timeout_seconds` задаёт бюджет native exception/API helper (15 секунд
по умолчанию, до 60); внешний timeout оставляет ещё 10 секунд на завершение.
Таймаут означает неполное наблюдение, а не автоматически crash или deadlock.

[[08 Auditor/Профили захвата]] · [[08 Auditor/Понимание evidence]] ·
[[03 Работа с движком/Практическая интеграция игры]]


## Сценарий и версия контента: current отдельно от legacy

Добавлено 21.10.2026. Старый `run.command` поддерживается, но без явно выбранного
сценария его metadata маркируется `unspecified`, а не автоматически current.
Для воспроизводимых игровых замеров задайте `run.selected_scenario`:

```json
{
  "run": {
    "selected_scenario": "current-play",
    "allow_legacy": false,
    "working_dir": "E:/workspace/LocalGame/Ascent",
    "scenarios": {
      "current-play": {
        "kind": "current",
        "content_version": "Ascent 0.1.20 desktop",
        "command": ["E:/workspace/LocalGame/Ascent/Pdoom.exe"]
      },
      "legacy-regression": {
        "kind": "legacy",
        "content_version": "Ascent 0.1.20 legacy smoke",
        "command": ["E:/workspace/LocalGame/Ascent/Pdoom.exe", "--benchmark"]
      }
    }
  }
}
```

Пути замените на свою полную runnable-копию. Для legacy нужно одновременно выбрать
его имя и явно задать `allow_legacy=true`; иначе admission отклоняется. Сценарий
пишется в `scenario.json`, summary и название Run. Версия — декларация конфигурации,
не автоматическое доказательство содержимого EXE. В текущем Ascent `--benchmark`
запускает исторический смешанный smoke и может показывать удалённые игровые экраны.
Не используйте его как доказательство обычного Play. Аргументы конкретной игры
не являются встроенными командами движка.

## Изменения и границы диагностики после аудита

- Exact runtime защищён TLS guard до non-recursive lock, в том числе при
  инициализации/flush. Recorder-internal повторные hooks не входят в наблюдаемую
  игровую последовательность. После насыщения буфера без rolling kernel queries
  пропускаются, но потери продолжают учитываться; COMPLETE при потере не обещается.
- xperf processing: общий бюджет decoder jobs 90 с, этап не более 30 с;
  tracestats не более 25 с, отдельный native header helper — до 20 с.
  Мониторятся размеры reports и diagnostics; Stop отменяет текущий xperf stage.
  `xperf/reports/progress.json` и UI log показывают этап. Незавершённый report
  не публикуется как готовый; ETL и предыдущие готовые reports сохраняются.
  Это бюджеты decoder subprocesses, а не гарантия полного wall time всего Run
  при любом диске/ОС. Profile summary остаётся trace-wide; stack/cpudisk scoped по PID.
- Architecture scanner принимает явно перечисленные внешние source files;
  они помечаются `@external/…`. Это не автоматическое включение всего SDK/STL.
- `EPIDEMIC_LOG_FRAME_EVENTS=1` в окружении процесса до старта включает старые текстовые frame
  begin/end. По умолчанию они выключены; counters и probes независимы от текста.

Изменения реализации не прогонялись заново по просьбе владельца. Исходные FAIL,
TIMEOUT и INCOMPLETE в [архивном аудите](<../../Reports/Archive/Engine Audit 2026-10-21.md>)
сохранены. Не объявляйте Deep/exact/coverage/ASan полностью квалифицированными
по факту наличия этих исправлений. [Оставшиеся задачи](../../maintenance-todo.md).
