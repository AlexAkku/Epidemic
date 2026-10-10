---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "economy"]
source_path: "EngineFramework/GameplayWorldStateOwners/Economy"
---

# EngineFramework / Economy

Модуль: **Economy**. Gameplay economy transactions.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/Economy/`.
**CMake targets:** `EpidemicGameFrameworkEconomy`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: block-local `LOCAL_READY` projection for Goal 4. This is not a whole-Framework `FROZEN` claim.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Economy/economy.h|economy.h]]: `EngineFramework/GameplayWorldStateOwners/Economy/include/Epidemic/GameFramework/Economy/economy.h`; объявленные типы: `name`, `IdHash`, `AccountState`, `FundsReservationState`, `OfferState`, `TradeTransactionState`, `DebtState`, `ContractState`, `EconomyChangeKind`, `CurrencyDefinition`, `EconomicAccount`, `FundsReservation`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/Economy|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.
