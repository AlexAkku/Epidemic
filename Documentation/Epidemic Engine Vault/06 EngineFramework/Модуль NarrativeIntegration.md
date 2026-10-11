---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "narrative_integration"]
source_path: "EngineFramework/IntegrationLayer/NarrativeIntegration"
---

# EngineFramework / NarrativeIntegration

Модуль: **NarrativeIntegration**. Связь narrative services.

**Реализация:** `EngineFramework/IntegrationLayer/NarrativeIntegration/`.
**CMake targets:** `EpidemicGameFrameworkNarrativeIntegration`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: B07 Goal 4 review. Local integration contract only.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/NarrativeIntegration/narrative_adapters.h|narrative_adapters.h]]: `EngineFramework/IntegrationLayer/NarrativeIntegration/include/Epidemic/GameFramework/NarrativeIntegration/narrative_adapters.h`; объявленные типы: `NarrativeEventContract`, `NarrativeIntegrationContractRegistry`, `NarrativeSemanticEventAdapter`, `ExternalConsequenceDeliveryState`, `NarrativeExternalConsequenceDelivery`, `NarrativeExternalConsequenceSnapshot`, `NarrativeExternalConsequenceOutbox`, `NarrativeExternalConsequenceSaveParticipant`, `KnowledgeNarrativeReference`, `KnowledgeNarrativeAdapter`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/modules/narrative_integration|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Состояния и доменные перечисления

- `ExternalConsequenceDeliveryState` (`EngineFramework/IntegrationLayer/NarrativeIntegration/include/Epidemic/GameFramework/NarrativeIntegration/narrative_adapters.h`): `Pending`, `Retryable`, `Applied`, `FailedTerminal`.

### Операции и хранилища состояния

#### `NarrativeIntegrationContractRegistry`

Объявлен в `EngineFramework/IntegrationLayer/NarrativeIntegration/include/Epidemic/GameFramework/NarrativeIntegration/narrative_adapters.h:23`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] static constexpr TypeId DiscoveryContractId() noexcept;
[[nodiscard]] static constexpr TypeId WorldEventContractId() noexcept;
[[nodiscard]] static constexpr TypeId EncounterCompletedContractId() noexcept;
[[nodiscard]] static constexpr narrative::NarrativeEventTypeId DiscoveryEventType() noexcept;
[[nodiscard]] static constexpr narrative::NarrativeEventTypeId WorldEventType() noexcept;
[[nodiscard]] static constexpr narrative::NarrativeEventTypeId EncounterCompletedEventType() noexcept;
[[nodiscard]] static constexpr TagId DiscoveryTag() noexcept;
[[nodiscard]] static constexpr TagId EncounterCompletedTag() noexcept;
[[nodiscard]] foundation::Result<void> RegisterContract(NarrativeEventContract contract);
[[nodiscard]] foundation::Result<void> RegisterStandardContracts();
[[nodiscard]] foundation::Result<void> Freeze();
[[nodiscard]] const NarrativeEventContract *Find(TypeId id) const noexcept;
[[nodiscard]] bool Frozen() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::unordered_map<TypeId, NarrativeEventContract> contracts_`; `bool frozen_ = false`.

#### `NarrativeExternalConsequenceOutbox`

Объявлен в `EngineFramework/IntegrationLayer/NarrativeIntegration/include/Epidemic/GameFramework/NarrativeIntegration/narrative_adapters.h:140`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
explicit NarrativeExternalConsequenceOutbox(std::size_t max_records = 4096) noexcept : max_records_(max_records);
[[nodiscard]] narrative::NarrativeConsequenceResult Execute( const narrative::NarrativeConsequenceDefinition &definition, const narrative::NarrativeConsequenceExecution &execution, const narrative::NarrativeExecutionContext &context) const override;
[[nodiscard]] std::vector<NarrativeExternalConsequenceDelivery> PendingDeliveries() const;
[[nodiscard]] const NarrativeExternalConsequenceDelivery *FindDelivery( narrative::NarrativeConsequenceExecutionId execution) const noexcept;
[[nodiscard]] NarrativeExternalConsequenceSnapshot CaptureSnapshot() const;
[[nodiscard]] foundation::Result<void> ValidateSnapshot(const NarrativeExternalConsequenceSnapshot &snapshot) const;
[[nodiscard]] foundation::Result<NarrativeExternalConsequenceSnapshot> PrepareSnapshotForRestore( NarrativeExternalConsequenceSnapshot snapshot) const;
void PublishPreparedSnapshot(NarrativeExternalConsequenceSnapshot &&snapshot) noexcept;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(NarrativeExternalConsequenceSnapshot snapshot);
[[nodiscard]] Revision CurrentRevision() const noexcept;
[[nodiscard]] std::size_t Capacity() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::size_t max_records_ = 4096`; `mutable std::vector<NarrativeExternalConsequenceDelivery> deliveries_`.

#### `NarrativeExternalConsequenceSaveParticipant`

Объявлен в `EngineFramework/IntegrationLayer/NarrativeIntegration/include/Epidemic/GameFramework/NarrativeIntegration/narrative_adapters.h:185`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
explicit NarrativeExternalConsequenceSaveParticipant(NarrativeExternalConsequenceOutbox &outbox) noexcept : outbox_(outbox);
[[nodiscard]] savegame::SaveParticipantId Id() const noexcept override;
[[nodiscard]] savegame::SaveSchemaVersion SchemaVersion() const noexcept override;
[[nodiscard]] std::vector<savegame::SaveParticipantId> Dependencies() const override;
[[nodiscard]] foundation::Result<savegame::SaveSection> CaptureSnapshot( const savegame::SaveContext &context) const override;
[[nodiscard]] foundation::Result<void> ValidateSnapshot(const savegame::SaveSection &section, const savegame::RestoreContext &context) const override;
[[nodiscard]] foundation::Result<std::unique_ptr<savegame::IRestoreStage>> StageRestore( const savegame::SaveSection &section, const savegame::RestoreContext &context) override;
void CommitRestore(savegame::IRestoreStage &stage) noexcept override;
```

#### `KnowledgeNarrativeAdapter`

Объявлен в `EngineFramework/IntegrationLayer/NarrativeIntegration/include/Epidemic/GameFramework/NarrativeIntegration/narrative_adapters.h:228`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] foundation::Result<narrative::ClueId> CreateClueFromKnowledge( narrative::NarrativeService &narrative_service, const knowledge::KnowledgeService &knowledge_service, knowledge::KnowledgeRecordId record, GameplayObjectRef area, GameplayContext context) const;
[[nodiscard]] foundation::Result<narrative::RumorId> CreateRumorFromKnowledge( narrative::NarrativeService &narrative_service, const knowledge::KnowledgeService &knowledge_service, knowledge::KnowledgeRecordId record, GameplayObjectRef scope, GameplayDuration lifetime, GameplayContext context) const;
[[nodiscard]] static foundation::Result<KnowledgeNarrativeReference> DecodeReference( std::span<const std::byte> payload);
```

#### `NarrativeSemanticEventAdapter`

Объявлен в `EngineFramework/IntegrationLayer/NarrativeIntegration/include/Epidemic/GameFramework/NarrativeIntegration/narrative_adapters.h:70`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
explicit NarrativeSemanticEventAdapter(const NarrativeIntegrationContractRegistry &contracts) noexcept : contracts_(contracts);
```

#### `NarrativeExternalConsequenceDelivery`

Объявлен в `EngineFramework/IntegrationLayer/NarrativeIntegration/include/Epidemic/GameFramework/NarrativeIntegration/narrative_adapters.h:117`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `ExternalConsequenceDeliveryState state = ExternalConsequenceDeliveryState::Pending`; `std::vector<std::byte> payload`.

#### `NarrativeExternalConsequenceSnapshot`

Объявлен в `EngineFramework/IntegrationLayer/NarrativeIntegration/include/Epidemic/GameFramework/NarrativeIntegration/narrative_adapters.h:134`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::vector<NarrativeExternalConsequenceDelivery> deliveries`.

#### `KnowledgeNarrativeReference`

Объявлен в `EngineFramework/IntegrationLayer/NarrativeIntegration/include/Epidemic/GameFramework/NarrativeIntegration/narrative_adapters.h:209`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `static constexpr std::uint32_t kSchemaVersion = 1`; `std::uint32_t schema_version = kSchemaVersion`; `knowledge::KnowledgeAssertionValue assertion = knowledge::KnowledgeAssertionValue::Unknown`; `knowledge::KnowledgeEpistemicState epistemic_state = knowledge::KnowledgeEpistemicState::Suspected`; `knowledge::KnowledgeConfidence confidence = knowledge::KnowledgeConfidence::None`; `std::uint32_t transmission_depth = 0`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/IntegrationLayer/NarrativeIntegration/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkNarrative`, `EpidemicFoundation`, `EpidemicGameFrameworkSaveGame`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/IntegrationLayer/NarrativeIntegration/src/save_participants.cpp` (81 строк)
- `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp` (980 строк): `Error::Create`, `GameplayObjectId::FromRaw`, `CorrelationId::FromRaw`, `OperationId::FromRaw`, `GameplayDomainId::FromRaw`, `TypeId::FromRaw`, `ClueId::FromRaw`, `NarrativeIntegrationContractRegistry::RegisterContract`, `NarrativeIntegrationContractRegistry::RegisterStandardContracts`, `NarrativeIntegrationContractRegistry::Freeze`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `framework.narrative`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/save_participants.cpp:79`
- `gameplay.narrative_integration.invalid_delivery_snapshot`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:791`
- `gameplay.narrative_integration.delivery_snapshot_capacity`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:780`
- `gameplay.narrative_integration.contracts_frozen`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:376`
- `gameplay.narrative_integration.invalid_contract`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:379`
- `gameplay.narrative_integration.duplicate_contract`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:382`
- `gameplay.narrative_integration.duplicate_event_type`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:388`
- `gameplay.narrative_integration.standard_contract_conflict`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:408`
- `gameplay.narrative_integration.standard_contract_missing`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:433`
- `gameplay.narrative_integration.contracts_not_frozen`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:450`
- `gameplay.narrative_integration.contract_missing`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:455`
- `gameplay.narrative_integration.invalid_discovery`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:469`
- `narrative.clue.discovery`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:477`
- `gameplay.narrative_integration.discovery_deferred`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:500`
- `gameplay.narrative_integration.invalid_world_event`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:518`
- `gameplay.narrative_integration.invalid_encounter_event`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:543`
- `outbox.execute.publish`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:633`
- `gameplay.narrative_integration.delivery_missing`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:728`
- `gameplay.narrative_integration.delivery_terminal`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:733`
- `gameplay.revision_exhausted`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:760`
- `gameplay.narrative_integration.invalid_external_operation`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:695`
- `gameplay.narrative_integration.delivery_conflict`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:702`
- `gameplay.narrative_integration.delivery_not_terminal`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:756`
- `integration.narrative.external_consequence_delivery`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:827`
- `gameplay.narrative_integration.delivery_snapshot_too_large`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:836`
- `gameplay.narrative_integration.invalid_save_section`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:867`
- `gameplay.narrative_integration.knowledge_missing`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:922`
- `narrative.clue.knowledge_reference`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:900`
- `gameplay.narrative_integration.invalid_knowledge_rumor`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:918`
- `gameplay.narrative_integration.knowledge_rumor_time_overflow`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:926`
- `gameplay.narrative_integration.invalid_knowledge_reference`: `EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:978`

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `framework.narrative` (`EngineFramework/IntegrationLayer/NarrativeIntegration/src/save_participants.cpp:79`)

```cpp
77 std::unique_ptr<ISaveParticipant> CreateNarrativeServiceSaveParticipant(epidemic::gameplay::narrative::NarrativeService& owner, SaveRestoreWorkspace& workspace)
   78 {
   79     return std::make_unique<detail::SnapshotParticipant<epidemic::gameplay::narrative::NarrativeService>>(owner, workspace, "framework.narrative", std::vector<SaveParticipantId>{});
   80 }
   81 }
```

#### `gameplay.narrative_integration.invalid_delivery_snapshot` (`EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:791`)

```cpp
789         {
  790             return foundation::Result<void>::Failure(
  791                 Error("gameplay.narrative_integration.invalid_delivery_snapshot", "invalid external consequence delivery snapshot"));
  792         }
  793     }
```

#### `gameplay.narrative_integration.delivery_snapshot_capacity` (`EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:780`)

```cpp
778     if (snapshot.deliveries.size() > max_records_)
  779         return foundation::Result<void>::Failure(
  780             Error("gameplay.narrative_integration.delivery_snapshot_capacity", "delivery snapshot exceeds configured capacity"));
  781     std::unordered_set<GameplayObjectId> ids;
  782     for (const auto &delivery : snapshot.deliveries)
```

#### `gameplay.narrative_integration.contracts_frozen` (`EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:376`)

```cpp
374     if (frozen_)
  375         return foundation::Result<void>::Failure(
  376             Error("gameplay.narrative_integration.contracts_frozen", "narrative integration contracts are frozen"));
  377     if (!contract.id.IsValid() || !contract.event_type.IsValid())
  378         return foundation::Result<void>::Failure(
```

#### `gameplay.narrative_integration.invalid_contract` (`EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:379`)

```cpp
377     if (!contract.id.IsValid() || !contract.event_type.IsValid())
  378         return foundation::Result<void>::Failure(
  379             Error("gameplay.narrative_integration.invalid_contract", "invalid narrative integration event contract"));
  380     if (contracts_.contains(contract.id))
  381         return foundation::Result<void>::Failure(
```

#### `gameplay.narrative_integration.duplicate_contract` (`EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:382`)

```cpp
380     if (contracts_.contains(contract.id))
  381         return foundation::Result<void>::Failure(
  382             Error("gameplay.narrative_integration.duplicate_contract", "duplicate narrative integration contract id"));
  383     for (const auto &[id, existing] : contracts_)
  384     {
```

#### `gameplay.narrative_integration.duplicate_event_type` (`EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:388`)

```cpp
386         if (existing.event_type == contract.event_type)
  387             return foundation::Result<void>::Failure(
  388                 Error("gameplay.narrative_integration.duplicate_event_type", "duplicate narrative event type contract"));
  389     }
  390     contracts_.emplace(contract.id, contract);
```

#### `gameplay.narrative_integration.standard_contract_conflict` (`EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:408`)

```cpp
406             if (existing->event_type != contract.event_type || existing->required_tag != contract.required_tag)
  407                 return foundation::Result<void>::Failure(
  408                     Error("gameplay.narrative_integration.standard_contract_conflict",
  409                           "standard narrative integration contract conflicts with existing registration"));
  410             continue;
```

#### `gameplay.narrative_integration.standard_contract_missing` (`EngineFramework/IntegrationLayer/NarrativeIntegration/src/narrative_adapters.cpp:433`)

```cpp
431     {
  432         return foundation::Result<void>::Failure(
  433             Error("gameplay.narrative_integration.standard_contract_missing",
  434                   "standard narrative integration contracts are missing or incompatible"));
  435     }
```
