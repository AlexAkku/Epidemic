# Epidemic Engine

Epidemic Engine — Windows-only C++23 движок, построенный как несколько строго направленных слоев. Нижние слои предоставляют стабильные механизмы, верхние добавляют игровое значение и никогда не протягивают зависимости обратно вниз.

Текущая структура проекта:

```text
EngineBase
    ↓
EngineRuntime
    ↓
GameFramework
    ↓
Game
```

`EngineBase` содержит минимальную платформу исполнения: общие типы, память и диагностику, microkernel и lifecycle приложения, Win32 platform/windowing, raw input и минимальный RHI с D3D11 backend. `EngineRuntime` содержит независимые engine majors: assets/resources, persistence, time/environment, scene/world/streaming, renderer, physics, navigation, animation, audio и simulation. Между Runtime majors нет прямых зависимостей; их технические связи собирает только `EngineRuntime/Support`.

`GameFramework` должен содержать переиспользуемые gameplay-системы, а `Game` — конкретные правила, контент и конечный composition root.

Подробное устройство слоев описано в [`docs/architecture.md`](docs/architecture.md). Документация EngineBase находится в [`docs/EngineBase/README.md`](docs/EngineBase/README.md), практическое использование — в [`docs/EngineBase/using_enginebase.md`](docs/EngineBase/using_enginebase.md). Для Runtime аналогичные документы находятся в [`docs/EngineRuntime/README.md`](docs/EngineRuntime/README.md) и [`docs/EngineRuntime/using_engineruntime.md`](docs/EngineRuntime/using_engineruntime.md).

## Сборка

Из корня репозитория:

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

## Тесты

```powershell
ctest --test-dir build -C Debug --output-on-failure
```

### Единый локальный CI и отладка

Проект использует адаптированную архитектуру
[AgentEnforcer2](https://github.com/Artemonim/AgentEnforcer2). Главная точка входа:

```powershell
./run.ps1                                      # все Full Debug+Release тесты
./run.ps1 -Mode Matrix                         # вся Base/Runtime/Full матрица
./run.ps1 -Mode Module -Module Simulation      # тесты одного модуля
./run.ps1 -Mode Debug -Module Combat           # подробный Debug-прогон
./run.ps1 -Mode ListTests                      # список зарегистрированных тестов
```

Новые CTest-тесты обнаруживаются автоматически. Если меняется ожидаемый состав матрицы,
необходимо также осознанно обновить `docs/freeze/ctest_manifest.json`.

Краткий итог последнего прогона находится в `.enforcer/Enforcer_last_check.md`,
структурированный отчёт — в `.ci_cache/report.json`, полные выводы команд — в
`.ci_cache/logs/`.

`-NoCache` отключает доверенные stamps проверок, но не заставляет перекомпилировать
готовые объектные файлы; `-Clean` очищает CI cache, а не build-каталоги.
Измеренный холодный прогон Matrix и повторный Full описаны в
`docs/freeze/goal5_delta2_execution.md` (Ryzen 5 2600, 32 ГБ RAM, шесть build jobs).

Goal 5 persistence stages (M5.1 / M5.2.A-E):

```powershell
./run.ps1 -Mode Module -TestRegex 'SaveParticipantTests|FrameworkSaveComposition|GameFrameworkSaveGameTests|RuntimeSupportTests|Persistence.*Truth' -NoCache
python docs/freeze/goal5_participant_contract.py --check --implementation
python docs/freeze/goal5_participant_contract.py --check --qualification
python docs/freeze/goal5_participant_contract.py --self-test
python tools/generate_goal5_participants.py --check
python docs/freeze/goal5_schema_contract.py --check
python docs/freeze/goal5_runtime_durable_contract.py --check
python docs/freeze/goal5_stage52_contract.py --check
```

Состав — 47 границ сохранения: 34 Framework owners и 13 integration checkpoints;
45 новых bindings размещены в 12 существующих IntegrationLayer modules.
Контракт и генератор не используют milestone/work-plan Markdown как вход.
`--implementation` проверяет девять typed executable obligations каждой границы,
включая непустой durable round-trip.
`--qualification` дополнительно требует всю шестипрофильную Matrix и точные
fingerprints source/graph; stale receipt не считается PASS. Matrix не присваивает
semantic IMPLEMENTED. После успешной Matrix receipt записывается командой
`python tools/record_goal5_participant_qualification.py --record`.
Добавление границы начинается с явного semantic review 5.1 и mapping в
`docs/freeze/goal5_participant_bindings.py`, fixture и теста, затем owning generators
обновляют review/contract и typed bindings. Маппинг 5.2 не изменяет решения 5.1;
несовпадение блокирует генерацию. Reviewed evidence отдельно записывается через
`tools/record_goal5_participant_evidence.py --record`. Restore mechanics и fault seam
доступны только через PRIVATE include; consumer compile fixtures проверяются при
Framework configure. Канонический freeze обновляется из
raw handoff своим merge, а не ручным редактированием JSON.

В Full теперь 109 CTest targets; Base/Runtime остаются 14/34. API композиции
`SaveGameOrchestrator::RegisterComposition` атомарно проверяет exact set, регистрирует
миграции и закрывает реестр. Schema policy: v1-only, `NO_PRIOR_SUPPORTED_VERSION`,
без фиктивной v0; fixtures в `Tests/Fixtures/SaveGame` неизменяемы и не
перегенерируются CI. Whole-image codec — canonical field-wise LE с bounds и
checksum; `ISavePackageStore` отделяет SaveGame от I/O. FileSavePackageStore
использует узкий Base Platform port: same-directory temp, flush, atomic replacement.
В failure log сохраняются phase/path/OS error. Все 5.1/5.2 validators включены
в общий runner. Labels: `persistence-schema`, `persistence-wire`, `persistence-storage`.
Runtime Support предоставляет независимые bounded v1 пакеты Runtime Time
(`CommitRuntimeTimeCheckpoint`/`LoadRuntimeTimeCheckpoint`) и полный файловый
`IPersistenceBackend` (`CreateFilePersistenceBackend`). Они сохраняют fractional
clock remainder и все Persistence records; fresh-process smoke восстанавливает
новые clock/store services. Runtime fixtures также immutable:
`Tests/Fixtures/RuntimePersistence`; MemoryOnly default не считается durable backend.
Unified cross-plane consistent cut и
whole-engine restore/reconciliation остаются 5.3/5.4; determinism — 5.5/5.6.

Только Runtime:

```powershell
ctest --test-dir build -C Debug -R EpidemicRuntime --output-on-failure
```

## Smoke-приложения EngineBase

После Debug-сборки:

```powershell
.\build\EngineBase\Apps\HeadlessCoreApp\EpidemicHeadlessCoreApp.exe
.\build\EngineBase\Apps\WindowSmokeApp\EpidemicWindowSmokeApp.exe
.\build\EngineBase\Apps\InputSmokeApp\EpidemicInputSmokeApp.exe
.\build\EngineBase\Apps\RhiClearScreenApp\EpidemicRhiClearScreenApp.exe
```

Логи записываются в `logs/epidemic.log`.

## Главное правило зависимостей

Нижний слой никогда не зависит от верхнего. Внутри `EngineRuntime` один major не включает и не линкует другой major; взаимодействие между ними осуществляется через public contracts и adapters в `Support`. Gameplay-смысл не добавляется в EngineBase или Runtime ради удобства верхнего кода.

Текущая базовая платформа — Windows 11 / Win32 / D3D11.

## TODO

- [x] Переход на C++ 23
- [ ] Engine Freeze
  - [x] Цель 1. Проверяемый baseline и единый freeze contract
  - [x] Цель 2. Полный локальный freeze-аудит EngineBase
  - [x] Цель 3. Полный локальный freeze-аудит EngineRuntime
  - [x] Цель 4. Полный локальный freeze-аудит EngineFramework
  - [ ] Цель 5. Доказать persistence и determinism всего движка
  - [ ] Цель 6. Доказать memory/lifetime и concurrency/async safety
  - [ ] Цель 7. Второй круг: проверить смысловые кластеры и причинные цепочки
  - [ ] Цель 8. Провести failure, regression, load, scale и degradation qualification
  - [ ] Цель 9. Финальная test saturation, whole-engine qualification и окончательный freeze
- [ ] Framework Documentation
- [ ] Base and Runtime docs checkup
- [ ] Комментарии
  -   [ ] Base перевод комментариев на русский
  -   [ ] Runtime добавить комментарии в код на английском и русском
  -   [ ] Framework добавить комментарии в код на английском и русском
  -   [ ] Переписать документацию из ии-слопа в нормальный текст
- [ ] Добавление заготовки под DX11/12/Vulacan/Metal для мультиплатформенности
- [ ] Интеграция Angel Script
- [ ] Добавление инструментов мониторинга и контроля ресурсов приложения
- [ ] Добавление дефолтного проекта
- [ ] Полноценная мультиплатформенность Win/Mac/Android
- [ ] Переход на С++ 26 после выхода stable версии языка

Текущая базовая платформа — Windows 11 / Win32 / D3D11. Cross-platform поддержка не является целью этого этапа.

## License

This project is licensed under the PolyForm Noncommercial License 1.0.0.

Non-commercial use, study, research, modification, and hobby projects are permitted under the terms of the license.

Commercial use requires a separate license from the copyright holder.

See [LICENSE](LICENSE) for the full license terms.
