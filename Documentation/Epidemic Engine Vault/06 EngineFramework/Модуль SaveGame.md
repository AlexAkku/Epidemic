---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "save_game"]
source_path: "EngineFramework/GameplayWorldStateOwners/SaveGame"
---

# EngineFramework / SaveGame

Модуль: **SaveGame**. Gameplay save/load, participants и wire.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/SaveGame/`.
**CMake targets:** `EpidemicGameFrameworkSaveGame`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: B07 Goal 4 review. Local module contract only.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/SaveGame/save_game.h|save_game.h]]: `EngineFramework/GameplayWorldStateOwners/SaveGame/include/Epidemic/GameFramework/SaveGame/save_game.h`; объявленные типы: `SaveParticipantId`, `IdHash`, `ContentCompatibilityPolicy`, `SaveParticipantRequirement`, `SavePhase`, `SaveSection`, `SaveParticipantManifest`, `SaveGameManifest`, `SaveGameImage`, `SaveContext`, `RestoreContext`, `SaveFormatSupport`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/modules/save_game|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
