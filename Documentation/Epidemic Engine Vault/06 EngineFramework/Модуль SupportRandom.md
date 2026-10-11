---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "support_random"]
source_path: "EngineFramework/BaseInfrastructure/SupportRandom"
---

# EngineFramework / SupportRandom

Модуль: **SupportRandom**. Детерминированная псевдослучайность.

**Реализация:** `EngineFramework/BaseInfrastructure/SupportRandom/`.
**CMake targets:** `EpidemicGameFrameworkSupportRandom`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: B01 Goal 4 review. Local module contract only.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/SupportRandom/deterministic_random.h|deterministic_random.h]]: `EngineFramework/BaseInfrastructure/SupportRandom/include/Epidemic/GameFramework/SupportRandom/deterministic_random.h`; объявленные типы: `RandomSeed`, `RandomStream`, `RandomSequenceSnapshot`, `RandomSequence`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/modules/support_random|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Операции и хранилища состояния

#### `RandomSequence`

Объявлен в `EngineFramework/BaseInfrastructure/SupportRandom/include/Epidemic/GameFramework/SupportRandom/deterministic_random.h:45`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
RandomSequence(RandomSeed seed, RandomStream stream, std::uint64_t sequence = 0);
[[nodiscard]] static constexpr bool IsValidSnapshot(const RandomSequenceSnapshot& snapshot) noexcept;
[[nodiscard]] static std::optional<RandomSequence> TryFromSnapshot(RandomSequenceSnapshot snapshot) noexcept;
[[nodiscard]] std::optional<std::uint64_t> TryNextU64() noexcept;
[[nodiscard]] std::optional<std::uint64_t> TryUniform(std::uint64_t exclusive_max) noexcept;
[[nodiscard]] std::optional<std::uint64_t> TryUniformRange(std::uint64_t minimum, std::uint64_t maximum_exclusive) noexcept;
[[nodiscard]] std::optional<double> TryUniform01() noexcept;
[[nodiscard]] std::optional<double> TryUniformReal(double minimum, double maximum) noexcept;
[[nodiscard]] std::optional<bool> TryRollMicro(std::uint32_t chance_micro) noexcept;
[[nodiscard]] std::uint64_t UniformUnchecked(std::uint64_t exclusive_max) noexcept;
[[nodiscard]] std::uint64_t UniformRangeUnchecked(std::uint64_t minimum, std::uint64_t maximum_exclusive) noexcept;
[[nodiscard]] double Uniform01Unchecked() noexcept;
[[nodiscard]] double UniformRealUnchecked(double minimum, double maximum) noexcept;
[[nodiscard]] bool RollMicroUnchecked(std::uint32_t chance_micro) noexcept;
[[nodiscard]] std::optional<std::size_t> WeightedIndex(std::span<const std::uint64_t> weights) noexcept;
template <typename T> void ShuffleUnchecked(std::span<T> values) noexcept(std::is_nothrow_swappable_v<T>);
template <typename TContainer> void ShuffleUnchecked(TContainer& values) noexcept(noexcept(ShuffleUnchecked(std::span;
[[nodiscard]] RandomSequenceSnapshot CaptureSnapshot() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t sequence_ = 0`.

#### `RandomSeed`

Объявлен в `EngineFramework/BaseInfrastructure/SupportRandom/include/Epidemic/GameFramework/SupportRandom/deterministic_random.h:18`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t value = 0`.

#### `RandomSequenceSnapshot`

Объявлен в `EngineFramework/BaseInfrastructure/SupportRandom/include/Epidemic/GameFramework/SupportRandom/deterministic_random.h:35`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t sequence = 0`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/BaseInfrastructure/SupportRandom/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/BaseInfrastructure/SupportRandom/src/deterministic_random.cpp` (246 строк): `RandomSequence::RandomSequence`, `RandomSequence::TryFromSnapshot`, `RandomSequence::TryNextU64`, `RandomSequence::TryUniform`, `RandomSequence::TryUniformRange`, `RandomSequence::TryUniform01`, `RandomSequence::TryUniformReal`, `RandomSequence::TryRollMicro`, `RandomSequence::NextU64Unchecked`, `RandomSequence::UniformUnchecked`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.

