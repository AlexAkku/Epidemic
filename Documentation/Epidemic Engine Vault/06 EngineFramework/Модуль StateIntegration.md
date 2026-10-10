---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "state_integration"]
source_path: "EngineFramework/IntegrationLayer/StateIntegration"
---

# EngineFramework / StateIntegration

Модуль: **StateIntegration**. Согласование gameplay state.

**Реализация:** `EngineFramework/IntegrationLayer/StateIntegration/`.
**CMake targets:** `EpidemicGameFrameworkStateIntegration`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Goal 4 block: `b08_integration_adapters`. Freeze unit: `EngineFramework/IntegrationLayer/StateIntegration`. Admission: 52 public API, 72 mutation obligations, 0 lifecycle, 13 stale-identity, 0 external-boundary candidates.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/StateIntegration/state_adapters.h|state_adapters.h]]: `EngineFramework/IntegrationLayer/StateIntegration/include/Epidemic/GameFramework/StateIntegration/state_adapters.h`; объявленные типы: `GetEntityQuery`, `FindEntitiesQuery`, `GetMaterialSlotsQuery`, `GetConditionsQuery`, `StateQueryAdapter`, `ConditionFactValue`, `StateFactsCheckpoint`, `StateFactsAdapter`, `EntityCreateEffectPayload`, `EntityConvertEffectPayload`, `EntityTagEffectPayload`, `MaterialStimulusEffectPayload`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/StateIntegration|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
