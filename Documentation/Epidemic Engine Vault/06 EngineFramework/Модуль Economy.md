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

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Responsibility and ownership

Owns currencies, accounts, fund reservations, offers, trades, debts, contracts, economy revisions/generators and the economy journal.

The service remains the single authoritative owner of the state listed below. Derived indexes and journals are not independent semantic owners.

#### Authoritative state and indexes

Admission inventory: 105 public callables, 108 mutation obligations, 1 lifecycle candidates, 27 stale-identity candidates and 0 external-boundary candidates.

Primary records, secondary indexes, ID generators, revision/change sequence and bounded journal state are reviewed together. Mutations that span several containers must complete all fallible staging before the no-fail authoritative commit point. Query ordering and index-derived views are required to agree with primary state.

#### Public contracts and failure atomicity

Every admission callable below is classified and reviewed. Mutators have success, no-op, invalid/precondition and failure decisions in the B03 coverage projection. Revision, ID-generator and journal publication is part of the mutation contract. Allocation/publication failure must not expose a partially advanced generator, revision, primary record, derived index or journal entry.

B03 allocation evidence uses source-private module seams rather than the shared process-global allocator override. These seams are test-only implementation details and do not expand the public API.

#### Lifecycle and identity

Lifecycle candidates and stale identity candidates from the admission inventory were reviewed. Invalid, removed, duplicate and stale IDs are required to be rejected or treated as the documented no-op without publishing false state. Terminal records are not silently resurrected by retry.

#### Persistence

Snapshot/restore is reviewed as a candidate-state operation: validate first, build off-state, preserve all live state on failure, then publish the candidate. Successful restore preserves the persistent ID-generator/revision/journal boundary represented by the snapshot contract.

#### External boundaries

Price/provider failures are explicit service inputs. Admission emitted no separate external-boundary candidate rows.

External callback/provider failure is converted to the module error/result contract. Retry must not duplicate an already committed semantic side effect, and failed compensation that requires reconciliation stays represented by owned state.

#### Threading contract

No additional internal synchronization guarantee is introduced by B03. These owners are treated as externally serialized/owner-thread services for local correctness evidence. Later Goal 6 performs the engine-wide concurrency qualification.

#### Regression evidence

Registered module target: `EpidemicGameFrameworkEconomyTests` from `EngineFramework/DevelopmentInfrastructure/Tests/economy_tests.cpp`.

Источник контрактов: `Documentation/EngineFramework/Economy.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `AccountState` (`EngineFramework/GameplayWorldStateOwners/Economy/include/Epidemic/GameFramework/Economy/economy.h`): `Active`, `Frozen`, `Closed`.
- `FundsReservationState` (`EngineFramework/GameplayWorldStateOwners/Economy/include/Epidemic/GameFramework/Economy/economy.h`): `Active`, `Committed`, `Released`.
- `OfferState` (`EngineFramework/GameplayWorldStateOwners/Economy/include/Epidemic/GameFramework/Economy/economy.h`): `Active`, `Accepted`, `Expired`, `Cancelled`.
- `TradeTransactionState` (`EngineFramework/GameplayWorldStateOwners/Economy/include/Epidemic/GameFramework/Economy/economy.h`): `Prepared`, `Reserved`, `Committing`, `Committed`, `Compensating`, `Cancelled`, `Failed`.
- `DebtState` (`EngineFramework/GameplayWorldStateOwners/Economy/include/Epidemic/GameFramework/Economy/economy.h`): `Active`, `Paid`, `Forgiven`, `Defaulted`, `Expired`.
- `ContractState` (`EngineFramework/GameplayWorldStateOwners/Economy/include/Epidemic/GameFramework/Economy/economy.h`): `Draft`, `Active`, `Fulfilled`, `Breached`, `Cancelled`, `Expired`.
- `EconomyChangeKind` (`EngineFramework/GameplayWorldStateOwners/Economy/include/Epidemic/GameFramework/Economy/economy.h`): `AccountCreated`, `AccountStateChanged`, `BalanceChanged`, `FundsReserved`, `FundsReleased`, `FundsCommitted`, `MarketCreated`, `MarketChanged`, `OfferCreated`, `OfferChanged`, `TradeChanged`, `DebtChanged`, `ContractChanged`, `DebtCompacted` и другие значения.

### Операции и хранилища состояния

#### `EconomyService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Economy/include/Epidemic/GameFramework/Economy/economy.h:351`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
EconomyService();
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<CurrencyId> RegisterCurrency(CurrencyDefinition definition);
void Freeze() noexcept;
[[nodiscard]] foundation::Result<void> AddPriceProvider(PriceProviderId id, std::int32_t priority, const IPriceProvider *provider);
[[nodiscard]] foundation::Result<void> RegisterContractTermsSchema(ContractTermsSchema schema);
[[nodiscard]] foundation::Result<EconomicAccountId> CreateAccount(EconomicAccount account);
[[nodiscard]] const EconomicAccount *FindAccount(EconomicAccountId id) const noexcept;
[[nodiscard]] std::optional<EconomicAccount> FindAccountCopy(EconomicAccountId id) const noexcept;
[[nodiscard]] Fixed GetBalance(EconomicAccountId id) const noexcept;
[[nodiscard]] Fixed GetAvailableBalance(EconomicAccountId id) const noexcept;
[[nodiscard]] foundation::Result<MarketId> CreateMarket(MarketState market);
[[nodiscard]] foundation::Result<void> SetMarketIndicator(MarketIndicator indicator);
[[nodiscard]] std::optional<MarketIndicator> GetMarketIndicator(MarketId market, EconomicCommodityId commodity) const;
[[nodiscard]] std::optional<PriceQuote> GetPriceQuote(const PriceQuoteRequest &request) const;
[[nodiscard]] foundation::Result<OfferId> CreateOffer(EconomicOffer offer);
[[nodiscard]] foundation::Result<TradeTransactionId> AcceptOffer(OfferAcceptanceRequest request);
[[nodiscard]] foundation::Result<TradeTransactionId> PrepareTrade(TradePlan plan);
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `, OfferState state = OfferState::Active) const`; `bool frozen_ = false`; `std::unordered_map<CurrencyId, CurrencyDefinition, IdHash> currencies_`; `std::unordered_map<EconomicAccountId, EconomicAccount, IdHash> accounts_`; `std::unordered_map<FundsReservationId, FundsReservation, IdHash> reservations_`; `std::unordered_map<MarketId, MarketState, IdHash> markets_`; `std::unordered_map<IndicatorKey, MarketIndicator, IndicatorKeyHash> indicators_`; `std::unordered_map<OfferId, EconomicOffer, IdHash> offers_`.

#### `IPriceProvider`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Economy/include/Epidemic/GameFramework/Economy/economy.h:345`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IPriceProvider() = default;
[[nodiscard]] virtual std::optional<PriceQuote> Quote(const PriceQuoteRequest &request) const = 0;
```

#### `IdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Economy/include/Epidemic/GameFramework/Economy/economy.h:62`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `IndicatorKeyHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Economy/include/Epidemic/GameFramework/Economy/economy.h:448`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] std::size_t operator()(const IndicatorKey &k) const noexcept;
```

#### `name`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Economy/include/Epidemic/GameFramework/Economy/economy.h:32`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
\ static constexpr name FromRaw(std::uint64_t h, std::uint64_t l) noexcept \;
```

#### `CurrencyDefinition`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Economy/include/Epidemic/GameFramework/Economy/economy.h:134`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::string canonical_name`; `Fixed smallest_unit = 1`; `GameplayTagSet tags`.

#### `EconomicAccount`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Economy/include/Epidemic/GameFramework/Economy/economy.h:142`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `Fixed balance = 0`; `AccountState state = AccountState::Active`.

#### `FundsReservation`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Economy/include/Epidemic/GameFramework/Economy/economy.h:151`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `Fixed amount = 0`; `FundsReservationState state = FundsReservationState::Active`.

#### `MarketState`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Economy/include/Epidemic/GameFramework/Economy/economy.h:161`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `GameplayTagSet tags`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/Economy/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp` (1526 строк): `Error::Create`, `EconomyService::EconomyService`, `EconomyService::NextRevision`, `EconomyService::Freeze`, `EconomyService::AddPriceProvider`, `EconomyService::RegisterContractTermsSchema`, `EconomyService::RegisterCurrency`, `CurrencyId::FromString`, `EconomyService::CreateAccount`, `EconomyService::FindAccount`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `gameplay.economy.invalid_price_provider`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:119`
- `gameplay.economy.invalid_contract_schema`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:127`
- `gameplay.economy.frozen`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:134`
- `gameplay.economy.invalid_currency`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:142`
- `gameplay.registry_not_frozen`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:1132`
- `gameplay.economy.invalid_account`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:156`
- `gameplay.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:1213`
- `gameplay.economy.duplicate_account`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:168`
- `gameplay.economy.allocation_failure`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:1173`
- `gameplay.economy.account_state`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:208`
- `gameplay.economy.account_missing`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:202`
- `gameplay.economy.credit_invalid`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:249`
- `gameplay.economy.balance_overflow`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:899`
- `gameplay.economy.insufficient_funds`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:788`
- `gameplay.economy.id_exhausted`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:806`
- `gameplay.economy.reservation_missing`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:336`
- `gameplay.economy.commit_invalid`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:363`
- `gameplay.economy.invalid_market`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:391`
- `gameplay.economy.duplicate_market`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:401`
- `gameplay.economy.invalid_indicator`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:427`
- `gameplay.economy.stale_indicator`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:433`
- `gameplay.economy.invalid_offer`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:501`
- `gameplay.economy.duplicate_offer`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:511`
- `gameplay.economy.offer_missing`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:560`
- `gameplay.economy.offer_state`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:566`
- `gameplay.economy.offer_total_overflow`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:572`
- `gameplay.economy.offer_accounts`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:587`
- `gameplay.economy.invalid_money`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:703`
- `gameplay.economy.transfer_invalid`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:713`
- `gameplay.economy.trade_invalid`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:723`
- `gameplay.economy.duplicate_trade`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:740`
- `gameplay.economy.trade_state`: `EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:962`

### Конкретные тестовые якоря

- `EngineFramework/DevelopmentInfrastructure/Tests/economy_tests.cpp`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `gameplay.economy.invalid_price_provider` (`EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:119`)

```cpp
117     if (frozen_ || !id.IsValid() || !provider ||
  118         std::any_of(price_providers_.begin(), price_providers_.end(), [&](const auto &v) { return v.id == id; }))
  119         return foundation::Result<void>::Failure(Error("gameplay.economy.invalid_price_provider", "invalid/duplicate price provider"));
  120     price_providers_.push_back({id, priority, provider});
  121     return foundation::Result<void>::Success();
```

#### `gameplay.economy.invalid_contract_schema` (`EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:127`)

```cpp
125     if (frozen_ || !schema.type.IsValid() || !schema.schema.IsValid() || schema.max_payload_bytes == 0 ||
  126         contract_terms_schemas_.contains(schema.type))
  127         return foundation::Result<void>::Failure(Error("gameplay.economy.invalid_contract_schema", "invalid/duplicate contract terms schema"));
  128     contract_terms_schemas_.emplace(schema.type, schema);
  129     return foundation::Result<void>::Success();
```

#### `gameplay.economy.frozen` (`EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:134`)

```cpp
132 {
  133     if (frozen_)
  134         return foundation::Result<CurrencyId>::Failure(Error("gameplay.economy.frozen", "economy definitions frozen"));
  135     if (d.canonical_name.empty())
  136         return foundation::Result<CurrencyId>::Failure(Error("gameplay.economy.invalid_currency", "currency canonical name missing"));
```

#### `gameplay.economy.invalid_currency` (`EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:142`)

```cpp
140     if (!d.id.IsValid() || d.id != expected || d.smallest_unit <= 0 || currencies_.contains(d.id))
  141         return foundation::Result<CurrencyId>::Failure(
  142             Error("gameplay.economy.invalid_currency", "invalid/duplicate currency"));
  143     d.revision = Revision{1};
  144     const auto id = d.id;
```

#### `gameplay.registry_not_frozen` (`EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:1132`)

```cpp
1130     if (!frozen_)
 1131         return foundation::Result<EconomicContractId>::Failure(
 1132             Error("gameplay.registry_not_frozen", "economy definitions must be frozen before runtime mutation"));
 1133     std::unordered_set<GameplayObjectRef> unique_parties;
 1134     const bool parties_valid = std::all_of(c.parties.begin(), c.parties.end(),
```

#### `gameplay.economy.invalid_account` (`EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:156`)

```cpp
154         !currencies_.contains(a.currency) || a.balance < 0 || !ValidateMoneyAmount(a.currency, a.balance, true))
  155         return foundation::Result<EconomicAccountId>::Failure(
  156             Error("gameplay.economy.invalid_account", "invalid account"));
  157     const auto next_revision = NextRevision();
  158     if (!next_revision)
```

#### `gameplay.revision_exhausted` (`EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:1213`)

```cpp
1211     const auto next_revision = NextRevision();
 1212     if (!next_revision)
 1213         return foundation::Result<void>::Failure(Error("gameplay.revision_exhausted", "economy revision counter is exhausted"));
 1214     const auto copy = it->second;
 1215     contracts_.erase(it);
```

#### `gameplay.economy.duplicate_account` (`EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:168`)

```cpp
166         if (!a.id.IsValid() || accounts_.contains(a.id))
  167             return foundation::Result<EconomicAccountId>::Failure(
  168                 Error("gameplay.economy.duplicate_account", "duplicate account"));
  169         a.revision = *next_revision;
  170         auto staged_accounts = accounts_;
```

#### `gameplay.economy.allocation_failure` (`EngineFramework/GameplayWorldStateOwners/Economy/src/economy.cpp:1173`)

```cpp
1171     {
 1172         return foundation::Result<EconomicContractId>::Failure(
 1173             Error("gameplay.economy.allocation_failure", "failed to publish economic contract"));
 1174     }
 1175 }
```
