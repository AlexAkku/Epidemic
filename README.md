# Epidemic Engine

Epidemic Engine — Windows-only C++23 движок, построенный как несколько строго направленных слоев. Нижние слои предоставляют стабильные механизмы, верхние добавляют игровое значение и зависят только от разрешённых нижних слоёв.

Порядок слоёв от нижнего к верхнему; разрешённые зависимости направлены сверху вниз (`Game → EngineFramework → EngineRuntime → EngineBase`):

```text
EngineBase
    ↓
EngineRuntime
    ↓
EngineFramework (GameFramework)
    ↓
Game
```

`EngineBase` содержит минимальную платформу исполнения: общие типы, память и диагностику, microkernel и lifecycle приложения, Win32 platform/windowing, raw input и минимальный RHI с D3D11 backend. `EngineRuntime` содержит независимые engine majors: assets/resources, persistence, time/environment, scene/world/streaming, renderer, physics, navigation, animation, audio и simulation. Между Runtime majors нет прямых зависимостей; их технические связи собирает только `EngineRuntime/Support`.

`GameFramework` должен содержать переиспользуемые gameplay-системы, а `Game` — конкретные правила, контент и конечный composition root.

Подробное устройство слоев описано в [архитектуре](<Documentation/Epidemic Engine Vault/90 Первоисточники/Documentation/architecture.md>). Документация EngineBase: [обзор](<Documentation/Epidemic Engine Vault/90 Первоисточники/Documentation/EngineBase/README.md>) и [практическое использование](<Documentation/Epidemic Engine Vault/90 Первоисточники/Documentation/EngineBase/using_enginebase.md>). Документация EngineRuntime: [обзор](<Documentation/Epidemic Engine Vault/90 Первоисточники/Documentation/EngineRuntime/README.md>) и [практическое использование](<Documentation/Epidemic Engine Vault/90 Первоисточники/Documentation/EngineRuntime/using_engineruntime.md>). Документация GameFramework: [обзор и правила использования](<Documentation/Epidemic Engine Vault/90 Первоисточники/Documentation/EngineFramework/README.md>).

## Сборка

Из корня репозитория:

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

## Установленный SDK

`cmake --build build/full-release --config Release --target EpidemicSDK` собирает
библиотеки без запуска/сборки всего набора тестов. Затем
`cmake --install build/full-release --config Release --prefix <sdk>` создаёт
переносимую поставку для `find_package(Epidemic 1.0.1 CONFIG REQUIRED)`.
Пример независимого клиента находится в `Examples/MinimalGame`; задайте
`CMAKE_PREFIX_PATH=<sdk>`. `--headless` не создаёт окно, `--tool` пропускает
заставку, обычный запуск показывает её перед первым игровым кадром.
`EPIDEMIC_REQUIRED_ABI=1` проверяет совместимость при конфигурации.
Идентификаторы доступны в `Epidemic/build_info.h`, хеши библиотек — в
`share/Epidemic/sdk.json`. SDK включает Presentation и Session, ресурсы ASCENT
не требуются. Существующий SDK ASCENT собирается независимо.

## Тесты модулей

В Auditor `capture.api_timeout_seconds` задаёт native exception/API-наблюдению
бюджет до 60 секунд (по умолчанию 15). Внешний timeout оставляет ещё 10 секунд
на завершение helper. Ограниченный verifier-профиль сохраняет отдельный бюджет;
таймаут означает неполное наблюдение и сам по себе не доказывает crash/deadlock.

```powershell
ctest --test-dir build -C Debug --output-on-failure
```

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

Логи запускаемых вручную smoke-приложений зависят от рабочего каталога; для разработки запускайте их с рабочим каталогом `.ai/`, чтобы логи оставались в `.ai/logs/`.

## Подготовка рабочего пространства

Единый контракт работы агента: [`Development/AgentsRules/AGENT_RULES.md`](Development/AgentsRules/AGENT_RULES.md). Подробное описание процесса: [`Development/AgentsRules/WORKFLOW.ru.md`](Development/AgentsRules/WORKFLOW.ru.md).

```powershell
Development/AgentsRules/Scripts/Setup-AgentTools.ps1
Development/AgentsRules/Scripts/Invoke-Dev.ps1 -Action Build -Profile full -Configuration Debug
```

C++ сборки находятся в `build/`. Все остальные создаваемые инструменты, виртуальные окружения, индексы, модели, временные файлы и логи находятся в `.ai/`. Исходники и настройки инструментов хранятся в `Development/AgentsRules/`.

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
  - [x] Цель 5. Доказать persistence и determinism всего движка
  - [x] Цель 6. Build independent observer — частично завершена; полное закрытие отложено до завершения MS8 и появления первой тестовой игры в `Game`.
  - [ ] Цель 7. Hardering
  - [ ] Цель 8. Integration qualification
  - [ ] Цель 9. Destructive/stress/fault qualification
- [ ] Framework Documentation cleanup, freeze
- [ ] Base and Runtime docs checkup
- [ ] Комментарии
  -   [ ] Base перевод комментариев на русский
  -   [ ] Runtime добавить комментарии в код на английском и русском
  -   [ ] Framework добавить комментарии в код на английском и русском
- [ ] Добавление заготовки под DX11/12/Vulacan/Metal для мультиплатформенности
- [ ] Интеграция Angel Script
- [ ] Добавление дефолтного проекта
- [ ] Полноценная мультиплатформенность Win/Mac/Android
- [ ] Переход на С++ 26 после выхода stable версии языка

Текущая базовая платформа — Windows 11 / Win32 / D3D11. Cross-platform поддержка не является целью этого этапа.

## License

This project is licensed under the PolyForm Noncommercial License 1.0.0.

Non-commercial use, study, research, modification, and hobby projects are permitted under the terms of the license.

Commercial use requires a separate license from the copyright holder.

See [LICENSE](LICENSE) for the full license terms.
