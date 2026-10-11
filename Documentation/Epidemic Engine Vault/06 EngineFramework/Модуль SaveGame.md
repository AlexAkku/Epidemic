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

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Состояния и доменные перечисления

- `ContentCompatibilityPolicy` (`EngineFramework/GameplayWorldStateOwners/SaveGame/include/Epidemic/GameFramework/SaveGame/save_game.h`): .
- `SaveParticipantRequirement` (`EngineFramework/GameplayWorldStateOwners/SaveGame/include/Epidemic/GameFramework/SaveGame/save_game.h`): `Required`, `Optional`.
- `SavePhase` (`EngineFramework/GameplayWorldStateOwners/SaveGame/include/Epidemic/GameFramework/SaveGame/save_game.h`): `Idle`, `Barrier`, `Capturing`, `Validating`, `Staging`, `Committing`, `Complete`, `Failed`.

### Операции и хранилища состояния

#### `ISaveParticipant`

Объявлен в `EngineFramework/GameplayWorldStateOwners/SaveGame/include/Epidemic/GameFramework/SaveGame/save_game.h:178`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ISaveParticipant() = default;
[[nodiscard]] virtual SaveParticipantId Id() const noexcept = 0;
[[nodiscard]] virtual SaveSchemaVersion SchemaVersion() const noexcept = 0;
[[nodiscard]] virtual SaveParticipantRequirement Requirement() const noexcept;
[[nodiscard]] virtual std::vector<SaveParticipantId> Dependencies() const = 0;
// A restore-wide private validation view follows Dependencies() order and // is discarded on every exit. It must never publish to gameplay owners. virtual void BeginRestoreValidation() noexcept;
virtual void EndRestoreValidation() noexcept;
[[nodiscard]] virtual foundation::Result<SaveSection> CaptureSnapshot(const SaveContext &) const = 0;
[[nodiscard]] virtual foundation::Result<void> ValidateSnapshot(const SaveSection &, const RestoreContext &) const = 0;
[[nodiscard]] virtual foundation::Result<std::unique_ptr<IRestoreStage>> StageRestore(const SaveSection &, const RestoreContext &) = 0;
```

#### `ISaveMigration`

Объявлен в `EngineFramework/GameplayWorldStateOwners/SaveGame/include/Epidemic/GameFramework/SaveGame/save_game.h:202`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ISaveMigration() = default;
[[nodiscard]] virtual SaveParticipantId Participant() const noexcept = 0;
[[nodiscard]] virtual SaveSchemaVersion FromVersion() const noexcept = 0;
[[nodiscard]] virtual SaveSchemaVersion ToVersion() const noexcept = 0;
[[nodiscard]] virtual foundation::Result<SaveSection> Migrate(const SaveSection &) const = 0;
```

#### `ISavePackageStore`

Объявлен в `EngineFramework/GameplayWorldStateOwners/SaveGame/include/Epidemic/GameFramework/SaveGame/save_game.h:150`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ISavePackageStore() = default;
[[nodiscard]] virtual foundation::Result<std::vector<std::byte>> Load() = 0;
[[nodiscard]] virtual foundation::Result<void> Commit(std::span<const std::byte> package) = 0;
```

#### `ISaveBarrier`

Объявлен в `EngineFramework/GameplayWorldStateOwners/SaveGame/include/Epidemic/GameFramework/SaveGame/save_game.h:166`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ISaveBarrier() = default;
[[nodiscard]] virtual foundation::Result<std::unique_ptr<ISaveBarrierLease>> AcquireCaptureLease() = 0;
[[nodiscard]] virtual foundation::Result<std::unique_ptr<ISaveBarrierLease>> AcquireRestoreLease() = 0;
```

#### `IdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/SaveGame/include/Epidemic/GameFramework/SaveGame/save_game.h:31`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] std::size_t operator()(SaveParticipantId id) const noexcept;
```

#### `ISaveBarrierLease`

Объявлен в `EngineFramework/GameplayWorldStateOwners/SaveGame/include/Epidemic/GameFramework/SaveGame/save_game.h:161`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ISaveBarrierLease() = default;
```

#### `IRestoreStage`

Объявлен в `EngineFramework/GameplayWorldStateOwners/SaveGame/include/Epidemic/GameFramework/SaveGame/save_game.h:173`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IRestoreStage() = default;
```

#### `SaveGameOrchestrator`

Объявлен в `EngineFramework/GameplayWorldStateOwners/SaveGame/include/Epidemic/GameFramework/SaveGame/save_game.h:223`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
) : format_(format), limits_(limits);
[[nodiscard]] foundation::Result<void> SetBarrier(ISaveBarrier &barrier) noexcept;
[[nodiscard]] foundation::Result<void> RegisterParticipant(ISaveParticipant &participant);
[[nodiscard]] foundation::Result<void> RegisterMigration(const ISaveMigration &migration);
[[nodiscard]] foundation::Result<void> FreezeRegistry();
[[nodiscard]] bool RegistryFrozen() const noexcept;
[[nodiscard]] foundation::Result<std::vector<SaveParticipantId>> ResolveOrder() const;
[[nodiscard]] foundation::Result<SaveGameImage> Capture(const SaveContext &context);
[[nodiscard]] foundation::Result<void> Restore(SaveGameImage image, const RestoreContext &context);
// Return an owned point-in-time snapshot, not a reference changing during later operations. // cppcheck-suppress returnByReference [[nodiscard]] SaveGameDiagnostics GetDiagnostics() const noexcept;
[[nodiscard]] static DataHash HashBytes(std::span<const std::byte> bytes) noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `, SaveSchemaVersion schema = 0) noexcept`; `std::unordered_map<SaveParticipantId, ParticipantRegistration, IdHash> participants_`; `std::unordered_map<MigrationKey, const ISaveMigration *, MigrationKeyHash> migrations_`; `bool registry_frozen_ = false`.

#### `SaveRestoreWorkspace`

Объявлен в `EngineFramework/GameplayWorldStateOwners/SaveGame/include/Epidemic/GameFramework/SaveGame/save_game.h:312`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
SaveRestoreWorkspace() = default;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::unordered_map<std::type_index, void*> live_`; `std::unordered_map<std::type_index, void*> staged_`; `std::unordered_map<std::type_index, ValidationRecord> validated_`; `std::size_t validation_users_ = 0`.

#### `MigrationKey`

Объявлен в `EngineFramework/GameplayWorldStateOwners/SaveGame/include/Epidemic/GameFramework/SaveGame/save_game.h:266`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `SaveSchemaVersion from = 0`.

#### `MigrationKeyHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/SaveGame/include/Epidemic/GameFramework/SaveGame/save_game.h:272`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] std::size_t operator()(const MigrationKey &k) const noexcept;
```

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/SaveGame/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_wire.cpp` (219 строк): `Error::Create`, `TypeId::FromRaw`, `SaveGameOrchestrator::HashBytes`
- `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp` (1022 строк): `Error::Create`, `SaveGameOrchestrator::HashBytes`, `SaveGameOrchestrator::MarkFailure`, `SaveGameOrchestrator::ValidateFormatConfiguration`, `SaveGameOrchestrator::SetBarrier`, `SaveGameOrchestrator::RegisterParticipant`, `SaveGameOrchestrator::RegisterMigration`, `SaveGameOrchestrator::RegisterComposition`, `SaveGameOrchestrator::FreezeRegistry`, `SaveGameOrchestrator::EnsureFrozen`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `gameplay.save.wire`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_wire.cpp:23`
- `store.commit`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_wire.cpp:206`
- `gameplay.save.store_exception`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_wire.cpp:217`
- `store.load`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_wire.cpp:216`
- `gameplay.save.callback_exception`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:47`
- `gameplay.save.invalid_format_support`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:128`
- `gameplay.save.invalid_resource_limits`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:134`
- `gameplay.save.registry_frozen`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:209`
- `gameplay.save.invalid_requirement`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:301`
- `gameplay.save.invalid_participant`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:163`
- `gameplay.save.too_many_dependencies`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:182`
- `gameplay.save.invalid_dependency`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:189`
- `gameplay.save.duplicate_dependency`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:196`
- `gameplay.save.invalid_migration`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:214`
- `gameplay.save.duplicate_migration`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:220`
- `gameplay.save.composition_not_empty`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:232`
- `gameplay.save.composition_set`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:236`
- `gameplay.save.composition_null`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:258`
- `gameplay.save.composition_contract`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:253`
- `gameplay.save.too_many_participants`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:293`
- `gameplay.save.dependency_missing`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:365`
- `gameplay.save.required_depends_on_optional`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:315`
- `gameplay.save.invalid_migration_target`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:327`
- `gameplay.save.dependency_cycle`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:375`
- `gameplay.save.barrier_missing`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:764`
- `gameplay.save.barrier_invalid`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:775`
- `gameplay.save.capture_failed`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:480`
- `gameplay.save.payload_limit`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:903`
- `gameplay.save.invalid_compatibility_policy`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:571`
- `gameplay.save.unsupported_format`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:533`
- `gameplay.save.compatibility_mismatch`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:563`
- `gameplay.save.manifest_structure`: `EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:603`

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `gameplay.save.wire` (`EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_wire.cpp:23`)

```cpp
21 foundation::Error WireError(std::string_view phase, const std::exception& error)
   22 {
   23     return foundation::Error::Create("gameplay.save.wire", "save package operation failed",
   24         std::string(phase) + ": " + error.what());
   25 }
```

#### `store.commit` (`EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_wire.cpp:206`)

```cpp
204     if (!encoded) return foundation::Result<void>::Failure(encoded.GetError());
  205     try { return store.Commit(encoded.Value()); }
  206     catch (const std::exception& error) { return foundation::Result<void>::Failure(WireError("store.commit",error)); }
  207     catch (...) { return foundation::Result<void>::Failure(foundation::Error::Create("gameplay.save.store_exception","unknown store commit exception")); }
  208 }
```

#### `gameplay.save.store_exception` (`EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_wire.cpp:217`)

```cpp
215     }
  216     catch (const std::exception& error) { return foundation::Result<SaveGameImage>::Failure(WireError("store.load",error)); }
  217     catch (...) { return foundation::Result<SaveGameImage>::Failure(foundation::Error::Create("gameplay.save.store_exception","unknown store load exception")); }
  218 }
  219 }
```

#### `store.load` (`EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_wire.cpp:216`)

```cpp
214         return DecodeSaveGameImage(bytes.Value(),limits);
  215     }
  216     catch (const std::exception& error) { return foundation::Result<SaveGameImage>::Failure(WireError("store.load",error)); }
  217     catch (...) { return foundation::Result<SaveGameImage>::Failure(foundation::Error::Create("gameplay.save.store_exception","unknown store load exception")); }
  218 }
```

#### `gameplay.save.callback_exception` (`EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:47`)

```cpp
45         detail += exception->what();
   46     }
   47     return foundation::Error::Create("gameplay.save.callback_exception", "save callback threw an exception",
   48                                      std::move(detail));
   49 }
```

#### `gameplay.save.invalid_format_support` (`EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:128`)

```cpp
126     {
  127         return foundation::Result<void>::Failure(
  128             Error("gameplay.save.invalid_format_support", "invalid save format support range"));
  129     }
  130     if (limits_.max_participants == 0 || limits_.max_section_bytes == 0 || limits_.max_total_payload_bytes == 0 ||
```

#### `gameplay.save.invalid_resource_limits` (`EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:134`)

```cpp
132     {
  133         return foundation::Result<void>::Failure(
  134             Error("gameplay.save.invalid_resource_limits", "invalid save resource limits"));
  135     }
  136     return foundation::Result<void>::Success();
```

#### `gameplay.save.registry_frozen` (`EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:209`)

```cpp
207     {
  208         return foundation::Result<void>::Failure(
  209             Error("gameplay.save.registry_frozen", "save migration registry is frozen"));
  210     }
  211     if (!migration.Participant().IsValid() || migration.FromVersion() == 0 || migration.ToVersion() == 0 ||
```

#### `gameplay.save.invalid_requirement` (`EngineFramework/GameplayWorldStateOwners/SaveGame/src/save_game.cpp:301`)

```cpp
299         {
  300             return foundation::Result<void>::Failure(
  301                 Error("gameplay.save.invalid_requirement", "invalid save participant requirement"));
  302         }
  303         for (const auto dependency : registration.dependencies)
```
