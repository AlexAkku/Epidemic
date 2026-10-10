---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "gameplay_integration"]
source_path: "EngineFramework/IntegrationLayer/GameplayIntegration"
---

# EngineFramework / GameplayIntegration

Модуль: **GameplayIntegration**. Сопряжение gameplay owners.

**Реализация:** `EngineFramework/IntegrationLayer/GameplayIntegration/`.
**CMake targets:** `EpidemicGameFrameworkGameplayIntegration`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Goal 4 block: `b08_integration_adapters`. Freeze unit: `EngineFramework/IntegrationLayer/GameplayIntegration`. Admission: 72 public API, 132 mutation obligations, 5 lifecycle, 31 stale-identity, 0 external-boundary candidates.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/GameplayIntegration/gameplay_adapters.h|gameplay_adapters.h]]: `EngineFramework/IntegrationLayer/GameplayIntegration/include/Epidemic/GameFramework/GameplayIntegration/gameplay_adapters.h`; объявленные типы: `CombatDamageEffectPayload`, `CombatDamageEffectHandler`, `ProgressionCombatRole`, `ProgressionCombatMapping`, `ProgressionCombatModifierProvider`, `CombatAbilityResourceMapping`, `CombatAbilityResourceProvider`, `ReservationToken`, `AbilityEffectDeliveryKey`, `AbilityEffectDeliveryRecord`, `AbilityEffectsCheckpoint`, `AbilityEffectsDispatcher`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/GameplayIntegration|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
