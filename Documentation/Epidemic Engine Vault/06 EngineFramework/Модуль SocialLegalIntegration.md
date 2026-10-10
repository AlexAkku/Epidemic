---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "social_legal_integration"]
source_path: "EngineFramework/IntegrationLayer/SocialLegalIntegration"
---

# EngineFramework / SocialLegalIntegration

Модуль: **SocialLegalIntegration**. Связь социальных и юридических систем.

**Реализация:** `EngineFramework/IntegrationLayer/SocialLegalIntegration/`.
**CMake targets:** `EpidemicGameFrameworkSocialLegalIntegration`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Goal 4 block: `b08_integration_adapters`. Freeze unit: `EngineFramework/IntegrationLayer/SocialLegalIntegration`. Admission: 28 public API, 40 mutation obligations, 1 lifecycle, 10 stale-identity, 0 external-boundary candidates.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/SocialLegalIntegration/social_legal_adapters.h|social_legal_adapters.h]]: `EngineFramework/IntegrationLayer/SocialLegalIntegration/include/Epidemic/GameFramework/SocialLegalIntegration/social_legal_adapters.h`; объявленные типы: `SocialLegalMappingId`, `MappingIdHash`, `PropertyCrimeVictimPolicy`, `PropertyCrimeMapping`, `CrimeRelationshipConsequenceMapping`, `AuthorityResponseMapping`, `PropertyCrimeDisposition`, `TheftResolution`, `RelationshipPenaltyDelivery`, `RelationshipPenaltyDeliveryHash`, `SocialLegalCheckpoint`, `SocialLegalMappings`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/SocialLegalIntegration|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
