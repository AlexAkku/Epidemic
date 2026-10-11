---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "support"]
source_path: "EngineRuntime/Support"
---

# EngineRuntime / Support

Модуль: **Support**. Композиция сервисов и adapters.

**Реализация:** `EngineRuntime/Support/`.
**CMake targets:** `EpidemicRuntimeSupport`
**Локальные test targets:** `EpidemicRuntimeSupportTests`, `EpidemicRuntimeSaveGenerationPackageTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/Support/runtime_support.h|runtime_support.h]]: `EngineRuntime/Support/include/Epidemic/Runtime/Support/runtime_support.h`; объявленные типы: `RuntimeFoundationRegistration`, `RuntimeProfile`, `RuntimeAdapterKind`, `RuntimeUpdateStep`, `RuntimeShutdownStep`, `ShutdownProposalPolicy`, `ChunkResourceRequirement`, `ChunkStreamingManifest`, `IChunkStreamingManifestSource`, `PreparedChunkDataSnapshot`, `IStreamingPreparedChunkDataQuery`, `IAnimationResourceMapper`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/support|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Runtime и Support|Runtime Support]] как место technical cross-major adapters.

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Назначение

`EngineRuntime/Support` — официальный composition layer Runtime. Он создает полный набор service bundles, соединяет независимые majors через typed adapters и предоставляет `IEngineRuntimeCoordinator`. Support не является еще одним major и не хранит собственную копию World, Scene, Resources или Simulation state.

#### Подготовка и регистрация

Основной вход — `PrepareEngineRuntime(options, dependencies)`. Функция проверяет зависимости, создает majors, adapters и coordinator во временном `PreparedEngineRuntime`. На этом этапе `Application` не меняется. После успешной подготовки `CommitPreparedRuntime()` атомарно регистрирует один aggregate `EngineRuntimeServices`.

`RegisterDefaultEngineRuntime()` объединяет эти два шага. Для инструментов и изолированных тестов отдельные majors по-прежнему можно создавать напрямую через их factories.

`EngineRuntimeServices` владеет bundles всех включенных majors, `RuntimeIntegrationServices` и coordinator. Один adapter instance передается всем своим interface roles и одновременно удерживается integration aggregate, поэтому wiring не имеет скрытых дубликатов.

#### Profiles и dependencies

`RuntimeProfile::Reference` создает reference implementations тех backend ports, которые позволяют поднять полный нейтральный runtime для разработки и тестов. `RuntimeProfile::Production` требует внешние production implementations там, где Runtime не должен подменять настоящий backend: renderer command sink, physics backend, navigation backend, animation evaluator, audio backend и simulation commit target.

Streaming является исключением только в смысле wiring: стандартная связь `World + Resources + Persistence → Streaming` принадлежит Support и используется в обоих profiles. В Production для нее необходимо передать настоящий `IChunkStreamingManifestSource`. Application может полностью заменить Streaming integration, но core roles передаются только полным комплектом; partial override отклоняется. То же правило применяется к составным стандартным projections: Navigation `data_source + cost_provider + obstacle_source` и Physics `transform_source + transform_sink` заменяются только целиком. Это исключает смешивание внешней и reference ownership внутри одного logical adapter.

#### Основные adapters

Support соединяет Scene с Renderer, Resources с Renderer, Scene с Physics и Audio, Resources с Animation и Audio, Animation с Renderer, World/Resources/Persistence со Streaming, Environment с Navigation и Time с Simulation.

Physics world-transform writes сначала попадают в `ISceneProjectionQueue` и применяются в отдельной `SceneProjectionCommit` phase. `Flush()` сортирует writes deterministically. После каждого successful Scene write ownership этой записи считается переданным окончательно; late failure или exception сохраняет для retry только неуспешный suffix, поэтому уже принятый prefix не применяется повторно.

Animation публикует pose с `RuntimeObjectId owner`; pose bridge преобразует его в нейтральный `renderer::RenderPoseBuffer`, который Renderer запрашивает по owner. Два pose indexes, по animator и owner, готовятся как единая candidate transaction и публикуются одновременно. Ошибка подготовки не меняет ни один index. Static proxy может не иметь pose.

Standard Streaming adapter получает chunk manifest, переводит принадлежащий ему chunk `Unloaded → Loading`, читает detached Persistence override, постепенно получает Resource leases и после успешной подготовки commit-ит `Resident/Active`. `IStreamingPreparedChunkDataQuery` предоставляет подготовленный persistence snapshot следующему слою, пока chunk остается загруженным. При unload adapter сначала переводит chunk в `Unloading`, затем освобождает active leases и только после этого завершает `Unloaded`. Failed cleanup сохраняет ownership для retry.

Animation resource adapter сначала подготавливает local ownership slot и только затем запрашивает `ResourceLease`. После копирования готового skeleton/clip descriptor lease освобождается; failed release сохраняется в adapter ownership до lifecycle cleanup. Между successful acquisition и публикацией owner нет allocation gap. Exceptions ResourceManager не пересекают adapter boundary.

#### Coordinator Tick

Coordinator использует следующий порядок:

```text
Time
MainThreadCommits
Resources
Streaming
Simulation
Navigation
Animation
Physics
SceneProjectionCommit
Audio
Renderer
DiagnosticsEvents
```

В `RuntimeTickResult.executed_steps` записываются реально выполненные phases. Recoverable ошибка major попадает в `RuntimeTickResult.failures`, но не останавливает независимые последующие phases. Simulation proposal commits ограничиваются per-frame budget. Game delta берется только из результата Time.

`DiagnosticsEvents` собирает frame events и phase failures в detached batch. `IRuntimeEventSink` имеет atomic publication contract: Result failure или exception означает, что batch не был externally committed. Reference sink сначала полностью строит candidate batch и только затем публикует его no-fail swap. Source buffers очищаются только после confirmed success.

Coordinator хранит private pending-frame state. После принятия phase её successful result и phase cursor получают durable ownership до перехода к следующей phase. Если поздняя Runtime-local allocation прерывает Tick, повтор с тем же `RuntimeFrameInput` продолжает незавершенный suffix и не повторяет уже accepted phases; другой input до reconciliation отклоняется с `runtime_support.frame_retry_required`. В частности, successful `Time::Advance`, Streaming и Simulation results сохраняются до fallible result materialization.

#### Shutdown

Первый вызов coordinator shutdown переводит Runtime в terminal lifecycle: новые кадры больше не принимаются. Cleanup выполняется best-effort по всем системам и adapters. Первая ошибка возвращается вызывающему коду, но независимые cleanup steps продолжаются. Completion каждого shutdown step и каждого составного adapter cleanup хранится durably. Повторный `Shutdown()` выполняет только незавершенные steps и не повторяет уже confirmed cleanup prefix. Объекты, handles и leases, которые не удалось освободить, не забываются. `IsShutdownComplete()` становится true только после полного освобождения owned Runtime state.

Simulation proposals обрабатываются согласно `ShutdownProposalPolicy`; они не удаляются молча. После успешного shutdown повторный вызов идемпотентен.

#### Стабильность

Support является единственным местом для стандартного технического cross-major wiring и после Goal 3/canonical evidence convergence имеет статус `LOCAL_READY`; это не означает системный `FROZEN` до Goals 5–9. Gameplay-связи сюда не добавляются. Новая связь между majors должна появляться в Support только тогда, когда она выражает нейтральную инфраструктуру; связь с игровым смыслом принадлежит `GameFramework`.

Источник контрактов: `Documentation/EngineRuntime/modules/support.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `RuntimeProfile` (`EngineRuntime/Support/include/Epidemic/Runtime/Support/runtime_support.h`): `Reference`, `Production`.
- `RuntimeAdapterKind` (`EngineRuntime/Support/include/Epidemic/Runtime/Support/runtime_support.h`): `SceneToRenderer`, `ResourcesToRenderer`, `SceneToPhysics`, `SceneToAudio`, `ResourcesToAnimation`, `AnimationToRenderer`, `ResourcesToAudio`, `WorldResourcesPersistenceToStreaming`, `TimeToSimulation`, `SimulationToDomain`, `EnvironmentToNavigation`.
- `RuntimeUpdateStep` (`EngineRuntime/Support/include/Epidemic/Runtime/Support/runtime_support.h`): `Time`, `MainThreadCommits`, `Resources`, `Streaming`, `Simulation`, `Navigation`, `Animation`, `Physics`, `SceneProjectionCommit`, `Audio`, `Renderer`, `DiagnosticsEvents`.
- `RuntimeShutdownStep` (`EngineRuntime/Support/include/Epidemic/Runtime/Support/runtime_support.h`): `StopNewWork`, `ResolveSimulationWork`, `ShutdownStreaming`, `ShutdownAudio`, `ShutdownPhysics`, `FlushSceneProjections`, `ReleaseAnimationResources`, `ShutdownRenderer`, `ReleaseResourceAdapters`, `ClearNavigationAndEvents`, `DestroyAdapters`, `Complete`.
- `ShutdownProposalPolicy` (`EngineRuntime/Support/include/Epidemic/Runtime/Support/runtime_support.h`): `CommitValid`, `DiscardWithReason`, `FailIfPending`.

### Операции и хранилища состояния

#### `IEngineRuntimeCoordinator`

Объявлен в `EngineRuntime/Support/include/Epidemic/Runtime/Support/runtime_support.h:359`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IEngineRuntimeCoordinator() = default;
[[nodiscard]] virtual foundation::Result<RuntimeTickResult> Tick(const RuntimeFrameInput& input) = 0;
[[nodiscard]] virtual foundation::Result<void> Shutdown() = 0;
[[nodiscard]] virtual bool IsShutdownStarted() const noexcept = 0;
[[nodiscard]] virtual bool IsShutdownComplete() const noexcept = 0;
```

#### `ISceneProjectionQueue`

Объявлен в `EngineRuntime/Support/include/Epidemic/Runtime/Support/runtime_support.h:182`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ISceneProjectionQueue() = default;
[[nodiscard]] virtual foundation::Result<std::size_t> Flush() = 0;
[[nodiscard]] virtual std::size_t PendingCount() const noexcept = 0;
[[nodiscard]] virtual foundation::Result<void> DiscardPending() = 0;
```

#### `IAnimationResourceMapper`

Объявлен в `EngineRuntime/Support/include/Epidemic/Runtime/Support/runtime_support.h:146`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IAnimationResourceMapper() = default;
[[nodiscard]] virtual foundation::Result<ResourceRequest> ResolveSkeleton(animation::SkeletonId id) const = 0;
[[nodiscard]] virtual foundation::Result<ResourceRequest> ResolveClip(animation::AnimationClipId id) const = 0;
```

#### `IRuntimePoseCache`

Объявлен в `EngineRuntime/Support/include/Epidemic/Runtime/Support/runtime_support.h:192`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IRuntimePoseCache() = default;
[[nodiscard]] virtual foundation::Result<std::shared_ptr<const animation::PoseBuffer>> GetPose(animation::AnimatorHandle animator) const = 0;
[[nodiscard]] virtual std::size_t PoseCount() const noexcept = 0;
```

#### `IChunkStreamingManifestSource`

Объявлен в `EngineRuntime/Support/include/Epidemic/Runtime/Support/runtime_support.h:121`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IChunkStreamingManifestSource() = default;
[[nodiscard]] virtual foundation::Result<ChunkStreamingManifest> GetManifest(ChunkId chunk) const = 0;
```

#### `IStreamingPreparedChunkDataQuery`

Объявлен в `EngineRuntime/Support/include/Epidemic/Runtime/Support/runtime_support.h:137`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IStreamingPreparedChunkDataQuery() = default;
[[nodiscard]] virtual foundation::Result<PreparedChunkDataSnapshot> GetPreparedData(ChunkId chunk) const = 0;
```

#### `IAnimationSkeletonResourcePayload`

Объявлен в `EngineRuntime/Support/include/Epidemic/Runtime/Support/runtime_support.h:155`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IAnimationSkeletonResourcePayload() = default;
[[nodiscard]] virtual const animation::SkeletonDesc& GetSkeleton() const noexcept = 0;
```

#### `IAnimationClipResourcePayload`

Объявлен в `EngineRuntime/Support/include/Epidemic/Runtime/Support/runtime_support.h:162`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IAnimationClipResourcePayload() = default;
[[nodiscard]] virtual const animation::AnimationClipDesc& GetClip() const noexcept = 0;
```

#### `IAudioResourceMapper`

Объявлен в `EngineRuntime/Support/include/Epidemic/Runtime/Support/runtime_support.h:169`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IAudioResourceMapper() = default;
[[nodiscard]] virtual foundation::Result<ResourceRequest> ResolveSound(audio::SoundId id) const = 0;
```

#### `IRuntimeAdapterLifecycle`

Объявлен в `EngineRuntime/Support/include/Epidemic/Runtime/Support/runtime_support.h:228`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IRuntimeAdapterLifecycle() = default;
[[nodiscard]] virtual foundation::Result<void> Shutdown() = 0;
```

#### `IReferenceSimulationCommitLog`

Объявлен в `EngineRuntime/Support/include/Epidemic/Runtime/Support/runtime_support.h:235`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IReferenceSimulationCommitLog() = default;
[[nodiscard]] virtual std::span<const simulation::SimulationProposalBatch> CommittedBatches() const noexcept = 0;
```

#### `IRuntimeEventSink`

Объявлен в `EngineRuntime/Support/include/Epidemic/Runtime/Support/runtime_support.h:216`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IRuntimeEventSink() = default;
```

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineRuntime/Support/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicCore`, `EpidemicPlatform`, `EpidemicRuntimeFoundation`, `EpidemicRuntimeAssets`, `EpidemicRuntimeSerialization`, `EpidemicRuntimeResources`, `EpidemicRuntimePersistence`, `EpidemicRuntimeTime`, `EpidemicRuntimeEnvironment`, `EpidemicRuntimeScene`, `EpidemicRuntimeWorld`, `EpidemicRuntimeStreaming`, `EpidemicRuntimeSimulation`, `EpidemicRuntimeNavigation`, `EpidemicRuntimeAnimation`, `EpidemicRuntimePhysics`, `EpidemicRuntimeAudio`, `EpidemicRuntimeRenderer`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineRuntime/Support/src/runtime_packages.cpp` (246 строк): `Error::Create`
- `EngineRuntime/Support/src/runtime_support.cpp` (4911 строк): `Error::Create`, `StringId::FromString`, `ResourceId::FromString`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `runtime.package.invalid`: `EngineRuntime/Support/src/runtime_packages.cpp:17`
- `persistence.commit`: `EngineRuntime/Support/src/runtime_packages.cpp:174`
- `time.encode`: `EngineRuntime/Support/src/runtime_packages.cpp:183`
- `time.decode`: `EngineRuntime/Support/src/runtime_packages.cpp:188`
- `persistence.encode`: `EngineRuntime/Support/src/runtime_packages.cpp:193`
- `persistence.decode`: `EngineRuntime/Support/src/runtime_packages.cpp:198`
- `runtime.package.persistence_changed`: `EngineRuntime/Support/src/runtime_packages.cpp:213`
- `persistence.capture`: `EngineRuntime/Support/src/runtime_packages.cpp:220`
- `runtime.package.persistence_capture_exception`: `EngineRuntime/Support/src/runtime_packages.cpp:223`
- `persistence.factory`: `EngineRuntime/Support/src/runtime_packages.cpp:244`
- `runtime_support.missing_dependency`: `EngineRuntime/Support/src/runtime_support.cpp:3614`
- `runtime_support.duplicate_registration`: `EngineRuntime/Support/src/runtime_support.cpp:4824`
- `runtime_support.registration_failed`: `EngineRuntime/Support/src/runtime_support.cpp:134`
- `animation.skeleton`: `EngineRuntime/Support/src/runtime_support.cpp:151`
- `animation.clip`: `EngineRuntime/Support/src/runtime_support.cpp:156`
- `audio.clip`: `EngineRuntime/Support/src/runtime_support.cpp:161`
- `runtime_support.adapter_shutdown`: `EngineRuntime/Support/src/runtime_support.cpp:1909`
- `runtime_support.render_payload_cleanup_pending`: `EngineRuntime/Support/src/runtime_support.cpp:423`
- `runtime_support.resource_manager_exception`: `EngineRuntime/Support/src/runtime_support.cpp:2725`
- `renderer.invalid_resource`: `EngineRuntime/Support/src/runtime_support.cpp:353`
- `runtime_support.reference_overflow`: `EngineRuntime/Support/src/runtime_support.cpp:360`
- `runtime_support.render_lease_publication_conflict`: `EngineRuntime/Support/src/runtime_support.cpp:375`
- `runtime_support.allocation_failure`: `EngineRuntime/Support/src/runtime_support.cpp:2833`
- `runtime_support.render_lease_publication_failed`: `EngineRuntime/Support/src/runtime_support.cpp:419`
- `renderer.resource_not_ready`: `EngineRuntime/Support/src/runtime_support.cpp:462`
- `renderer.resource_failed`: `EngineRuntime/Support/src/runtime_support.cpp:468`
- `renderer.transform_missing`: `EngineRuntime/Support/src/runtime_support.cpp:499`
- `runtime_support.transform_missing`: `EngineRuntime/Support/src/runtime_support.cpp:528`
- `runtime_support.invalid_transform`: `EngineRuntime/Support/src/runtime_support.cpp:542`
- `runtime_support.scene_projection_exception`: `EngineRuntime/Support/src/runtime_support.cpp:581`
- `runtime_support.pending_scene_projections`: `EngineRuntime/Support/src/runtime_support.cpp:618`
- `runtime_support.unsupported_simulation_domain`: `EngineRuntime/Support/src/runtime_support.cpp:650`

### Конкретные тестовые якоря

- `EngineRuntime/Support/tests/runtime_support_tests.cpp`: `TestRenderLeaseRollbackAndCompositeRetry`, `TestStreamingPublicationDuplicateAndPlanAtomicity`, `TestStreamingPrepareDataAndCleanupRetry`, `TestMainViewCreationRollback`, `TestIndividualRegistration`, `TestIndividualRegistrationFailuresAreAtomic`, `TestAtomicDefaultCompositionAndTypedOwnership`, `TestPreparedCommitDuplicateIsAtomic`, `TestProductionPreflightIsAtomic`, `TestSceneProjectionQueue`, `TestStreamingWorldLifecycle`, `TestSceneProjectionPrefixRetry`, `TestCoordinatorRejectsInvalidInputBeforeMutation`
- `EngineRuntime/Support/tests/runtime_package_tests.cpp`: `TestRuntimeDurablePackages`
- `EngineRuntime/Support/tests/runtime_generation_package_tests.cpp`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `runtime.package.invalid` (`EngineRuntime/Support/src/runtime_packages.cpp:17`)

```cpp
15 void Need(bool condition, const char* reason) { if (!condition) throw std::runtime_error(reason); }
   16 foundation::Error PackageError(std::string_view phase, const std::exception& error)
   17 { return foundation::Error::Create("runtime.package.invalid","Runtime durable package rejected",std::string(phase)+": "+error.what()); }
   18 std::uint64_t Checksum(std::span<const std::byte> bytes)
   19 {
```

#### `persistence.commit` (`EngineRuntime/Support/src/runtime_packages.cpp:174`)

```cpp
172             return platform::CommitFilePackage(path_,bytes.Value(),kPackageLimit);
  173         }
  174         catch (const std::exception& error) { return foundation::Result<void>::Failure(PackageError("persistence.commit",error)); }
  175     }
  176   private:
```

#### `time.encode` (`EngineRuntime/Support/src/runtime_packages.cpp:183`)

```cpp
181 {
  182     try { return foundation::Result<std::vector<std::byte>>::Success(Encode(checkpoint,"EPIRTIM1")); }
  183     catch (const std::exception& error) { return foundation::Result<std::vector<std::byte>>::Failure(PackageError("time.encode",error)); }
  184 }
  185 foundation::Result<TimeCheckpoint> DecodeRuntimeTimeCheckpointPackage(std::span<const std::byte> package)
```

#### `time.decode` (`EngineRuntime/Support/src/runtime_packages.cpp:188`)

```cpp
186 {
  187     try { return foundation::Result<TimeCheckpoint>::Success(Decode<TimeCheckpoint>(package,"EPIRTIM1")); }
  188     catch (const std::exception& error) { return foundation::Result<TimeCheckpoint>::Failure(PackageError("time.decode",error)); }
  189 }
  190 foundation::Result<std::vector<std::byte>> EncodeRuntimePersistencePackage(const PersistenceSnapshot& snapshot)
```

#### `persistence.encode` (`EngineRuntime/Support/src/runtime_packages.cpp:193`)

```cpp
191 {
  192     try { return foundation::Result<std::vector<std::byte>>::Success(Encode(snapshot,"EPIRPER1")); }
  193     catch (const std::exception& error) { return foundation::Result<std::vector<std::byte>>::Failure(PackageError("persistence.encode",error)); }
  194 }
  195 foundation::Result<PersistenceSnapshot> DecodeRuntimePersistencePackage(std::span<const std::byte> package)
```

#### `persistence.decode` (`EngineRuntime/Support/src/runtime_packages.cpp:198`)

```cpp
196 {
  197     try { return foundation::Result<PersistenceSnapshot>::Success(Decode<PersistenceSnapshot>(package,"EPIRPER1")); }
  198     catch (const std::exception& error) { return foundation::Result<PersistenceSnapshot>::Failure(PackageError("persistence.decode",error)); }
  199 }
  200 foundation::Result<PersistenceSnapshot> CaptureRuntimePersistenceSnapshot(const IPersistenceQuery& query)
```

#### `runtime.package.persistence_changed` (`EngineRuntime/Support/src/runtime_packages.cpp:213`)

```cpp
211         if (before!=after)
  212             return foundation::Result<PersistenceSnapshot>::Failure(foundation::Error::Create(
  213                 "runtime.package.persistence_changed","Runtime Persistence changed during quiesced snapshot capture"));
  214         snapshot.current_revision=after;
  215         auto valid=ValidatePersistenceSnapshot(snapshot);
```

#### `persistence.capture` (`EngineRuntime/Support/src/runtime_packages.cpp:220`)

```cpp
218     }
  219     catch (const std::exception& error)
  220     { return foundation::Result<PersistenceSnapshot>::Failure(PackageError("persistence.capture",error)); }
  221     catch (...)
  222     { return foundation::Result<PersistenceSnapshot>::Failure(foundation::Error::Create(
```

#### `runtime.package.persistence_capture_exception` (`EngineRuntime/Support/src/runtime_packages.cpp:223`)

```cpp
221     catch (...)
  222     { return foundation::Result<PersistenceSnapshot>::Failure(foundation::Error::Create(
  223         "runtime.package.persistence_capture_exception","Runtime Persistence query threw during snapshot capture")); }
  224 }
  225 foundation::Result<void> CommitRuntimeTimeCheckpoint(const foundation::Path& path,const TimeCheckpoint& checkpoint)
```
