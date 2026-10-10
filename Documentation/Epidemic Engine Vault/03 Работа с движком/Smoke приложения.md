---
aliases: []
type: "guide"
source_snapshot: "2026-10-10"
---

# Smoke приложения EngineBase

В дереве обнаружены четыре CMake executable target в `EngineBase/Apps/`: `EpidemicHeadlessCoreApp`, `EpidemicWindowSmokeApp`, `EpidemicInputSmokeApp`, `EpidemicRhiClearScreenApp`.

Команды ниже соответствуют типичной раскладке Debug при `cmake -S . -B build` с Visual Studio. Если используете `build/full-debug`, замените `build` в путях, а фактическое расположение `.exe` проверьте после сборки:

```powershell
.\build\EngineBase\Apps\HeadlessCoreApp\Debug\EpidemicHeadlessCoreApp.exe
.\build\EngineBase\Apps\WindowSmokeApp\Debug\EpidemicWindowSmokeApp.exe
.\build\EngineBase\Apps\InputSmokeApp\Debug\EpidemicInputSmokeApp.exe
.\build\EngineBase\Apps\RhiClearScreenApp\Debug\EpidemicRhiClearScreenApp.exe
```

Фактическое расположение зависит от CMake output directory; часть проекта задаёт его явно. Проверьте: `Get-ChildItem -Path .\build -Recurse -Filter EpidemicHeadlessCoreApp.exe`. Для ручных smoke запусков исходный `README.md` рекомендует рабочий каталог `.ai/`, чтобы пути логов оставались в `.ai/logs/`.

Smoke подтверждает отдельную возможность (headless lifecycle, создание окна, ввод, очистка RHI), а не готовность renderer/gameplay или интеграционное acceptance. Исходные CMake и entrypoints: `EngineBase/Apps/*`.
