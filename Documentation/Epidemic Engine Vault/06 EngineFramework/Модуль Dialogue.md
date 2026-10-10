---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "dialogue"]
source_path: "EngineFramework/GameplayWorldStateOwners/Dialogue"
---

# EngineFramework / Dialogue

Модуль: **Dialogue**. Dialogue state и контракты разговора.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/Dialogue/`.
**CMake targets:** `EpidemicGameFrameworkDialogue`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: B07 Goal 4 review. Local module contract only.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Dialogue/dialogue.h|dialogue.h]]: `EngineFramework/GameplayWorldStateOwners/Dialogue/include/Epidemic/GameFramework/Dialogue/dialogue.h`; объявленные типы: `name`, `IdHash`, `ConversationState`, `DialogueConditionState`, `DialogueConsequenceState`, `DialogueOptionRepeatPolicy`, `DialogueChangeKind`, `DialogueTextRef`, `DialogueConditionDefinition`, `DialogueConsequenceDefinition`, `DialogueOptionDefinition`, `DialogueNodeDefinition`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/modules/dialogue|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
