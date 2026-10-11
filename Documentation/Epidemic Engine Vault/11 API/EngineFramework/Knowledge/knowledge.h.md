---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/GameplayWorldStateOwners/Knowledge/include/Epidemic/GameFramework/Knowledge/knowledge.h"
---

# Knowledge: knowledge.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/GameplayWorldStateOwners/Knowledge/include/Epidemic/GameFramework/Knowledge/knowledge.h`.

**Модуль:** [[06 EngineFramework/Модуль Knowledge|Knowledge]].
**Размер исходного файла:** 15677 байт.
**Подключение:** `#include <Epidemic/GameFramework/Knowledge/knowledge.h>`.

## Объявления типов (лексический индекс)
`KnowledgeRecordId`, `MemoryRecordId`, `BeliefTypeId`, `KnowledgeSourceId`, `KnowledgeTopicId`, `MemoryTypeId`, `MemoryDecayRuleId`, `IdHash`, `RefHash`, `KnowledgeAssertionValue`, `KnowledgeEpistemicState`, `KnowledgeConfidence`, `MemoryImportance`, `MemoryPersistencePolicy`, `KnowledgeChangeKind`, `KnowledgeShareMode`, `KnowledgeTopic`, `MemoryDecayRule`, `KnowledgeProfile`, `KnowledgeRecord`, `MemoryRecord`, `LearnKnowledgeRequest`, `CreateMemoryRequest`, `ShareKnowledgeRequest`, `ContradictKnowledgeRequest`, `KnowledgeChange`, `KnowledgeChangeBatch`, `KnowledgeSnapshot`, `KnowledgeDiagnostics`, `KnowledgeService`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/GameFramework/Foundation/gameplay_foundation.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace epidemic::gameplay::knowledge
{
struct KnowledgeRecordId { GameplayObjectId value{}; static constexpr KnowledgeRecordId FromString(std::string_view s) noexcept{return {GameplayObjectId::FromString(s)};} static constexpr KnowledgeRecordId FromRaw(std::uint64_t h,std::uint64_t l) noexcept{return {GameplayObjectId::FromRaw(h,l)};} [[nodiscard]] constexpr bool IsValid()const noexcept{return value.IsValid();} [[nodiscard]] constexpr bool operator==(const KnowledgeRecordId&)const noexcept=default; [[nodiscard]] constexpr auto operator<=>(const KnowledgeRecordId&)const noexcept=default;};
struct MemoryRecordId { GameplayObjectId value{}; static constexpr MemoryRecordId FromString(std::string_view s) noexcept{return {GameplayObjectId::FromString(s)};} static constexpr MemoryRecordId FromRaw(std::uint64_t h,std::uint64_t l) noexcept{return {GameplayObjectId::FromRaw(h,l)};} [[nodiscard]] constexpr bool IsValid()const noexcept{return value.IsValid();} [[nodiscard]] constexpr bool operator==(const MemoryRecordId&)const noexcept=default; [[nodiscard]] constexpr auto operator<=>(const MemoryRecordId&)const noexcept=default;};
struct BeliefTypeId { TypeId value{}; static constexpr BeliefTypeId FromString(std::string_view s) noexcept{return {TypeId::FromString(s)};} [[nodiscard]] constexpr bool IsValid()const noexcept{return value.IsValid();} [[nodiscard]] constexpr bool operator==(const BeliefTypeId&)const noexcept=default; [[nodiscard]] constexpr auto operator<=>(const BeliefTypeId&)const noexcept=default;};
struct KnowledgeSourceId { TypeId value{}; static constexpr KnowledgeSourceId FromString(std::string_view s) noexcept{return {TypeId::FromString(s)};} [[nodiscard]] constexpr bool IsValid()const noexcept{return value.IsValid();} [[nodiscard]] constexpr bool operator==(const KnowledgeSourceId&)const noexcept=default; [[nodiscard]] constexpr auto operator<=>(const KnowledgeSourceId&)const noexcept=default;};
struct KnowledgeTopicId { TypeId value{}; static constexpr KnowledgeTopicId FromString(std::string_view s) noexcept{return {TypeId::FromString(s)};} [[nodiscard]] constexpr bool IsValid()const noexcept{return value.IsValid();} [[nodiscard]] constexpr bool operator==(const KnowledgeTopicId&)const noexcept=default; [[nodiscard]] constexpr auto operator<=>(const KnowledgeTopicId&)const noexcept=default;};
struct MemoryTypeId { TypeId value{}; static constexpr MemoryTypeId FromString(std::string_view s) noexcept{return {TypeId::FromString(s)};} [[nodiscard]] constexpr bool IsValid()const noexcept{return value.IsValid();} [[nodiscard]] constexpr bool operator==(const MemoryTypeId&)const noexcept=default; [[nodiscard]] constexpr auto operator<=>(const MemoryTypeId&)const noexcept=default;};
struct MemoryDecayRuleId { TypeId value{}; static constexpr MemoryDecayRuleId FromString(std::string_view s) noexcept{return {TypeId::FromString(s)};} [[nodiscard]] constexpr bool IsValid()const noexcept{return value.IsValid();} [[nodiscard]] constexpr bool operator==(const MemoryDecayRuleId&)const noexcept=default; [[nodiscard]] constexpr auto operator<=>(const MemoryDecayRuleId&)const noexcept=default;};
struct IdHash { template<class T> [[nodiscard]] std::size_t operator()(const T& id)const noexcept{return std::hash<decltype(id.value)>{}(id.value);} };
struct RefHash { [[nodiscard]] std::size_t operator()(GameplayObjectRef r)const noexcept{return std::hash<GameplayObjectRef>{}(r);} };

enum class KnowledgeAssertionValue { Unknown, Affirmed, Denied };
enum class KnowledgeEpistemicState { Known, Suspected, Rumor, Contradicted, Outdated };
enum class KnowledgeConfidence { None, Low, Medium, High, Certain };
enum class MemoryImportance { Low, Normal, High, Critical };
enum class MemoryPersistencePolicy { Transient, Session, Persistent, Timed };
enum class KnowledgeChangeKind { Learned, Updated, Forgotten, Shared, Contradicted, MemoryCreated, KnowledgeDecayed, MemoryDecayed, MemoryCompacted, MemoryForgotten, ProfileCreated, ProfileRemoved };
enum class KnowledgeShareMode { Tell, Report, Rumor, Broadcast, GroupSync };

struct KnowledgeTopic
{
    KnowledgeTopicId id{}; GameplayTagSet tags; GameplayObjectRef primary_subject{}; GameplayObjectRef scope{}; std::vector<std::byte> payload;
    [[nodiscard]] bool operator==(const KnowledgeTopic& o)const noexcept{return id==o.id&&primary_subject==o.primary_subject&&scope==o.scope;}
};
struct MemoryDecayRule
{
    MemoryDecayRuleId id{}; std::string canonical_name; GameplayDuration interval{}; std::uint8_t confidence_steps=1;
    std::optional<KnowledgeConfidence> outdated_at_or_below; std::optional<KnowledgeConfidence> forget_at_or_below;
};
struct KnowledgeProfile
{
    GameplayObjectRef subject{}; GameplayTagSet tags; MemoryPersistencePolicy retention=MemoryPersistencePolicy::Persistent;
    GameplayTagSet sharing_tags; MemoryDecayRuleId default_decay_rule{}; Revision revision{};
};
struct KnowledgeRecord
{
    KnowledgeRecordId id{}; GameplayObjectRef owner{}; BeliefTypeId type{}; KnowledgeTopic topic{};
    KnowledgeAssertionValue assertion=KnowledgeAssertionValue::Unknown; KnowledgeEpistemicState epistemic_state=KnowledgeEpistemicState::Suspected;
    KnowledgeConfidence confidence=KnowledgeConfidence::None; GameplayObjectRef subject{}; KnowledgeSourceId source_kind{}; GameplayObjectRef source{};
    KnowledgeRecordId derived_from{}; KnowledgeSourceId original_source_kind{}; GameplayObjectRef original_source{}; std::uint32_t transmission_depth=0;
    GameplayTimePoint learned_at{}; GameplayTimePoint last_confirmed_at{}; GameplayTimePoint last_decay_at{}; MemoryDecayRuleId decay_rule{};
    MemoryPersistencePolicy persistence=MemoryPersistencePolicy::Persistent; std::vector<std::byte> payload; Revision revision{};
};
struct MemoryRecord
{
    MemoryRecordId id{}; GameplayObjectRef owner{}; MemoryTypeId type{}; GameplayTimePoint time{}; GameplayObjectRef subject{}; GameplayObjectRef area{};
    MemoryImportance importance=MemoryImportance::Normal; TagId emotion{}; MemoryPersistencePolicy persistence=MemoryPersistencePolicy::Session;
    GameplayDuration decay_after{}; std::vector<std::byte> payload; Revision revision{};
};
struct LearnKnowledgeRequest
{
    GameplayObjectRef learner{}; BeliefTypeId type{}; KnowledgeTopic topic{}; KnowledgeSourceId source_kind{}; GameplayObjectRef source_object{};
    KnowledgeAssertionValue assertion=KnowledgeAssertionValue::Affirmed; KnowledgeEpistemicState epistemic_state=KnowledgeEpistemicState::Suspected;
    KnowledgeConfidence confidence=KnowledgeConfidence::Low; std::optional<MemoryPersistencePolicy> persistence; std::optional<MemoryDecayRuleId> decay_rule;
    std::vector<std::byte> payload; GameplayContext context{};
};
struct CreateMemoryRequest
{
    GameplayObjectRef owner{}; MemoryTypeId type{}; GameplayObjectRef subject{}; GameplayObjectRef area{}; MemoryImportance importance=MemoryImportance::Normal; TagId emotion{};
    std::optional<MemoryPersistencePolicy> persistence; GameplayDuration decay_after{}; std::vector<std::byte> payload; GameplayContext context{};
};
struct ShareKnowledgeRequest { GameplayObjectRef speaker{}; GameplayObjectRef listener{}; KnowledgeRecordId record{}; KnowledgeShareMode mode=KnowledgeShareMode::Tell; GameplayContext context{}; };
struct ContradictKnowledgeRequest
{
    KnowledgeRecordId old_record{}; KnowledgeAssertionValue new_assertion=KnowledgeAssertionValue::Denied;
    KnowledgeEpistemicState epistemic_state=KnowledgeEpistemicState::Known; KnowledgeConfidence confidence=KnowledgeConfidence::Medium;
    KnowledgeSourceId source_kind{}; GameplayObjectRef source_object{}; std::vector<std::byte> payload; GameplayContext context{};
};
struct KnowledgeChange
{
    std::uint64_t sequence=0; KnowledgeChangeKind kind=KnowledgeChangeKind::Learned; GameplayObjectRef owner{}; GameplayObjectRef other{};
    KnowledgeRecordId knowledge{}; MemoryRecordId memory{}; KnowledgeTopicId topic{}; GameplayContext context{}; Revision revision{};
};
struct KnowledgeChangeBatch
{
    std::vector<KnowledgeChange> changes; bool snapshot_required=false; std::uint64_t oldest_available_sequence=0; std::uint64_t latest_sequence=0;

    ChangeCursor oldest_available_cursor{};
    ChangeCursor latest_cursor{};
};
struct KnowledgeSnapshot
{
    std::vector<KnowledgeProfile> profiles; std::vector<KnowledgeRecord> knowledge; std::vector<MemoryRecord> memories;
    MonotonicIdGenerator<GameplayObjectId>::Snapshot knowledge_ids{}; MonotonicIdGenerator<GameplayObjectId>::Snapshot memory_ids{}; std::uint64_t next_change_sequence=1; Revision revision{};

    std::uint64_t change_epoch = 1;
};
struct KnowledgeDiagnostics
{
    std::uint64_t profiles=0,knowledge_records=0,memory_records=0,learn_ops=0,shared_records=0,forgotten_records=0,compacted_memories=0,contradictions=0;
};

class KnowledgeService
{
    friend struct epidemic::gameplay::SnapshotRestoreAccess<KnowledgeService>;

public:
    explicit KnowledgeService(std::size_t change_capacity=4096);
    [[nodiscard]] static constexpr GameplayDomainId Domain()noexcept{return GameplayDomainId::FromString("framework.knowledge");}
    [[nodiscard]] foundation::Result<void> RegisterDecayRule(MemoryDecayRule rule);
    [[nodiscard]] foundation::Result<void> FreezeDefinitions();
    [[nodiscard]] bool DefinitionsFrozen()const noexcept{return definitions_frozen_;}
    [[nodiscard]] foundation::Result<void> CreateProfile(KnowledgeProfile profile);
    [[nodiscard]] foundation::Result<void> RemoveProfile(GameplayObjectRef subject,GameplayContext context={});
    [[nodiscard]] foundation::Result<KnowledgeRecordId> Learn(LearnKnowledgeRequest request);
    [[nodiscard]] foundation::Result<MemoryRecordId> CreateMemory(CreateMemoryRequest request);
    [[nodiscard]] foundation::Result<MemoryRecordId> CompactMemories(GameplayObjectRef owner,std::span<const MemoryRecordId> source_ids,CreateMemoryRequest summary,GameplayContext context={});
    [[nodiscard]] foundation::Result<KnowledgeRecordId> Share(ShareKnowledgeRequest request);
    [[nodiscard]] foundation::Result<void> Forget(KnowledgeRecordId id,GameplayContext context={});
    [[nodiscard]] foundation::Result<KnowledgeRecordId> Contradict(ContradictKnowledgeRequest request);
    [[nodiscard]] foundation::Result<void> Decay(GameplayTimePoint now);
    [[nodiscard]] const KnowledgeProfile* FindProfile(GameplayObjectRef subject)const noexcept;
    [[nodiscard]] const KnowledgeRecord* FindKnowledge(KnowledgeRecordId id)const noexcept;
    [[nodiscard]] const MemoryRecord* FindMemory(MemoryRecordId id)const noexcept;
    [[nodiscard]] const MemoryDecayRule* FindDecayRule(MemoryDecayRuleId id)const noexcept;
    [[nodiscard]] std::vector<KnowledgeRecord> FindKnowledgeByOwner(GameplayObjectRef owner)const;
    [[nodiscard]] std::vector<KnowledgeRecord> FindKnowledgeByTopic(GameplayObjectRef owner,KnowledgeTopicId topic)const;
    [[nodiscard]] std::vector<KnowledgeRecord> FindKnowledgeAboutSubject(GameplayObjectRef owner,GameplayObjectRef subject)const;
    [[nodiscard]] std::optional<KnowledgeRecord> GetLastKnownPosition(GameplayObjectRef owner,GameplayObjectRef subject)const;
    [[nodiscard]] std::vector<MemoryRecord> FindMemoriesByOwner(GameplayObjectRef owner)const;
    [[nodiscard]] std::vector<MemoryRecord> FindImportantMemories(GameplayObjectRef owner,MemoryImportance min_importance)const;
    private:
        [[nodiscard]] KnowledgeChangeBatch ReadChangesSinceSequence(std::uint64_t sequence) const;
    public:
    [[nodiscard]] KnowledgeChangeBatch ReadChangesSince(ChangeCursor cursor) const
    {
        auto batch = ReadChangesSinceSequence(cursor.sequence);
        batch.oldest_available_cursor = {journal_epoch_, batch.oldest_available_sequence};
        batch.latest_cursor = {journal_epoch_, next_change_sequence_ == 0 ? std::numeric_limits<std::uint64_t>::max()
                                                                    : next_change_sequence_ - 1};
        if ((!cursor.IsValid() && cursor.sequence != 0) || (cursor.IsValid() && cursor.epoch != journal_epoch_))
        {
            batch.changes.clear();
            batch.snapshot_required = true;
        }
        return batch;
    }
    [[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept
    {
        return {journal_epoch_, next_change_sequence_ == 0 ? std::numeric_limits<std::uint64_t>::max()
                                                          : next_change_sequence_ - 1};
    }
  private:
    [[nodiscard]] std::vector<KnowledgeChange> ChangesSinceSequence(std::uint64_t sequence) const
    {
        return ReadChangesSinceSequence(sequence).changes;
    }

  public:
    [[nodiscard]] KnowledgeSnapshot CaptureSnapshot()const;
    [[nodiscard]] foundation::Result<void> RestoreSnapshot(KnowledgeSnapshot snapshot);
    [[nodiscard]] KnowledgeDiagnostics GetDiagnostics()const noexcept;
    [[nodiscard]] Revision CurrentRevision()const noexcept{return revision_;}
private:
    [[nodiscard]] foundation::Result<Revision> PrepareRevision() const;
    [[nodiscard]] bool CanAdvanceRevisionBy(std::size_t count) const noexcept;
    void Record(KnowledgeChange change) noexcept;
    [[nodiscard]] KnowledgeConfidence Degrade(KnowledgeConfidence c,KnowledgeShareMode mode)const noexcept;
    [[nodiscard]] foundation::Result<KnowledgeRecordId> LearnInternal(LearnKnowledgeRequest request,KnowledgeRecordId derived_from,KnowledgeSourceId original_source_kind,GameplayObjectRef original_source,std::uint32_t transmission_depth);
    [[nodiscard]] KnowledgeRecord* FindMutableKnowledge(KnowledgeRecordId id)noexcept;
    void IndexKnowledge(const KnowledgeRecord& record); void UnindexKnowledge(const KnowledgeRecord& record);
    void IndexMemory(const MemoryRecord& record); void UnindexMemory(const MemoryRecord& record);
    void RebuildIndexes();
    [[nodiscard]] bool CanShareTopic(const KnowledgeProfile& profile,const KnowledgeTopic& topic)const noexcept;

    Revision revision_{};
    MonotonicIdGenerator<GameplayObjectId> knowledge_ids_{0x2100}; MonotonicIdGenerator<GameplayObjectId> memory_ids_{0x2101};
    std::unordered_map<MemoryDecayRuleId,MemoryDecayRule,IdHash> decay_rules_; bool definitions_frozen_=false;
    std::unordered_map<GameplayObjectRef,KnowledgeProfile,RefHash> profiles_;
    std::unordered_map<KnowledgeRecordId,KnowledgeRecord,IdHash> knowledge_; std::unordered_map<MemoryRecordId,MemoryRecord,IdHash> memories_;
    std::unordered_map<GameplayObjectRef,std::vector<KnowledgeRecordId>,RefHash> knowledge_by_owner_;
    std::unordered_map<GameplayObjectRef,std::unordered_map<KnowledgeTopicId,std::vector<KnowledgeRecordId>,IdHash>,RefHash> knowledge_by_topic_;
    std::unordered_map<GameplayObjectRef,std::unordered_map<GameplayObjectRef,std::vector<KnowledgeRecordId>,RefHash>,RefHash> knowledge_by_subject_;
    std::unordered_map<GameplayObjectRef,std::vector<MemoryRecordId>,RefHash> memories_by_owner_;
    std::deque<KnowledgeChange> changes_; std::size_t change_capacity_=4096; std::uint64_t next_change_sequence_=1;
    std::uint64_t journal_epoch_ = 1; KnowledgeDiagnostics diagnostics_{};
};
} // namespace epidemic::gameplay::knowledge
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/GameFramework/Foundation/gameplay_foundation.h`.

### Перечисления и допустимые категории

- `KnowledgeAssertionValue`: `Unknown`, `Affirmed`, `Denied`
- `KnowledgeEpistemicState`: `Known`, `Suspected`, `Rumor`, `Contradicted`, `Outdated`
- `KnowledgeConfidence`: `None`, `Low`, `Medium`, `High`, `Certain`
- `MemoryImportance`: `Low`, `Normal`, `High`, `Critical`
- `MemoryPersistencePolicy`: `Transient`, `Session`, `Persistent`, `Timed`
- `KnowledgeChangeKind`: `Learned`, `Updated`, `Forgotten`, `Shared`, `Contradicted`, `MemoryCreated`, `KnowledgeDecayed`, `MemoryDecayed`, `MemoryCompacted`, `MemoryForgotten`, `ProfileCreated`, `ProfileRemoved`
- `KnowledgeShareMode`: `Tell`, `Report`, `Rumor`, `Broadcast`, `GroupSync`

### Состав типов и операции

#### `KnowledgeRecordId` (строка заголовка 19)

Публичные сигнатуры операций:

```cpp
static constexpr KnowledgeRecordId FromString(std::string_view s) noexcept;
static constexpr KnowledgeRecordId FromRaw(std::uint64_t h,std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid()const noexcept;
[[nodiscard]] constexpr bool operator==(const KnowledgeRecordId&)const noexcept=default;
[[nodiscard]] constexpr auto operator<=>(const KnowledgeRecordId&)const noexcept=default;
```

#### `MemoryRecordId` (строка заголовка 20)

Публичные сигнатуры операций:

```cpp
static constexpr MemoryRecordId FromString(std::string_view s) noexcept;
static constexpr MemoryRecordId FromRaw(std::uint64_t h,std::uint64_t l) noexcept;
[[nodiscard]] constexpr bool IsValid()const noexcept;
[[nodiscard]] constexpr bool operator==(const MemoryRecordId&)const noexcept=default;
[[nodiscard]] constexpr auto operator<=>(const MemoryRecordId&)const noexcept=default;
```

#### `BeliefTypeId` (строка заголовка 21)

Публичные сигнатуры операций:

```cpp
static constexpr BeliefTypeId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid()const noexcept;
[[nodiscard]] constexpr bool operator==(const BeliefTypeId&)const noexcept=default;
[[nodiscard]] constexpr auto operator<=>(const BeliefTypeId&)const noexcept=default;
```

#### `KnowledgeSourceId` (строка заголовка 22)

Публичные сигнатуры операций:

```cpp
static constexpr KnowledgeSourceId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid()const noexcept;
[[nodiscard]] constexpr bool operator==(const KnowledgeSourceId&)const noexcept=default;
[[nodiscard]] constexpr auto operator<=>(const KnowledgeSourceId&)const noexcept=default;
```

#### `KnowledgeTopicId` (строка заголовка 23)

Публичные сигнатуры операций:

```cpp
static constexpr KnowledgeTopicId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid()const noexcept;
[[nodiscard]] constexpr bool operator==(const KnowledgeTopicId&)const noexcept=default;
[[nodiscard]] constexpr auto operator<=>(const KnowledgeTopicId&)const noexcept=default;
```

#### `MemoryTypeId` (строка заголовка 24)

Публичные сигнатуры операций:

```cpp
static constexpr MemoryTypeId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid()const noexcept;
[[nodiscard]] constexpr bool operator==(const MemoryTypeId&)const noexcept=default;
[[nodiscard]] constexpr auto operator<=>(const MemoryTypeId&)const noexcept=default;
```

#### `MemoryDecayRuleId` (строка заголовка 25)

Публичные сигнатуры операций:

```cpp
static constexpr MemoryDecayRuleId FromString(std::string_view s) noexcept;
[[nodiscard]] constexpr bool IsValid()const noexcept;
[[nodiscard]] constexpr bool operator==(const MemoryDecayRuleId&)const noexcept=default;
[[nodiscard]] constexpr auto operator<=>(const MemoryDecayRuleId&)const noexcept=default;
```

#### `IdHash` (строка заголовка 26)

Публичные сигнатуры операций:

```cpp
template<class T> [[nodiscard]] std::size_t operator()(const T& id)const noexcept;
```

#### `T` (строка заголовка 26)

#### `RefHash` (строка заголовка 27)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] std::size_t operator()(GameplayObjectRef r)const noexcept;
```

#### `KnowledgeTopic` (строка заголовка 37)

Публичные поля и значения по умолчанию:

- `GameplayTagSet tags`
- `std::vector<std::byte> payload`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] bool operator==(const KnowledgeTopic& o)const noexcept;
```

#### `MemoryDecayRule` (строка заголовка 42)

Публичные поля и значения по умолчанию:

- `std::string canonical_name`
- `std::uint8_t confidence_steps=1`
- `std::optional<KnowledgeConfidence> outdated_at_or_below`
- `std::optional<KnowledgeConfidence> forget_at_or_below`

#### `KnowledgeProfile` (строка заголовка 47)

Публичные поля и значения по умолчанию:

- `GameplayTagSet tags`
- `MemoryPersistencePolicy retention=MemoryPersistencePolicy::Persistent`
- `GameplayTagSet sharing_tags`

#### `KnowledgeRecord` (строка заголовка 52)

Публичные поля и значения по умолчанию:

- `KnowledgeAssertionValue assertion=KnowledgeAssertionValue::Unknown`
- `KnowledgeEpistemicState epistemic_state=KnowledgeEpistemicState::Suspected`
- `KnowledgeConfidence confidence=KnowledgeConfidence::None`
- `std::uint32_t transmission_depth=0`
- `MemoryPersistencePolicy persistence=MemoryPersistencePolicy::Persistent`
- `std::vector<std::byte> payload`

#### `MemoryRecord` (строка заголовка 61)

Публичные поля и значения по умолчанию:

- `MemoryImportance importance=MemoryImportance::Normal`
- `MemoryPersistencePolicy persistence=MemoryPersistencePolicy::Session`
- `std::vector<std::byte> payload`

#### `LearnKnowledgeRequest` (строка заголовка 67)

Публичные поля и значения по умолчанию:

- `KnowledgeAssertionValue assertion=KnowledgeAssertionValue::Affirmed`
- `KnowledgeEpistemicState epistemic_state=KnowledgeEpistemicState::Suspected`
- `KnowledgeConfidence confidence=KnowledgeConfidence::Low`
- `std::optional<MemoryPersistencePolicy> persistence`
- `std::optional<MemoryDecayRuleId> decay_rule`
- `std::vector<std::byte> payload`

#### `CreateMemoryRequest` (строка заголовка 74)

Публичные поля и значения по умолчанию:

- `MemoryImportance importance=MemoryImportance::Normal`
- `std::optional<MemoryPersistencePolicy> persistence`
- `std::vector<std::byte> payload`

#### `ShareKnowledgeRequest` (строка заголовка 79)

Публичные поля и значения по умолчанию:

- `KnowledgeShareMode mode=KnowledgeShareMode::Tell`

#### `ContradictKnowledgeRequest` (строка заголовка 80)

Публичные поля и значения по умолчанию:

- `KnowledgeAssertionValue new_assertion=KnowledgeAssertionValue::Denied`
- `KnowledgeEpistemicState epistemic_state=KnowledgeEpistemicState::Known`
- `KnowledgeConfidence confidence=KnowledgeConfidence::Medium`
- `std::vector<std::byte> payload`

#### `KnowledgeChange` (строка заголовка 86)

Публичные поля и значения по умолчанию:

- `std::uint64_t sequence=0`
- `KnowledgeChangeKind kind=KnowledgeChangeKind::Learned`

#### `KnowledgeChangeBatch` (строка заголовка 91)

Публичные поля и значения по умолчанию:

- `std::vector<KnowledgeChange> changes`
- `bool snapshot_required=false`
- `std::uint64_t oldest_available_sequence=0`
- `std::uint64_t latest_sequence=0`

#### `KnowledgeSnapshot` (строка заголовка 98)

Публичные поля и значения по умолчанию:

- `std::vector<KnowledgeProfile> profiles`
- `std::vector<KnowledgeRecord> knowledge`
- `std::vector<MemoryRecord> memories`
- `std::uint64_t next_change_sequence=1`
- `std::uint64_t change_epoch = 1`

#### `KnowledgeDiagnostics` (строка заголовка 105)

Публичные поля и значения по умолчанию:

- `std::uint64_t profiles=0,knowledge_records=0,memory_records=0,learn_ops=0,shared_records=0,forgotten_records=0,compacted_memories=0,contradictions=0`

#### `KnowledgeService` (строка заголовка 110)

Внутреннее состояние (не часть публичного API): `std::unordered_map<MemoryDecayRuleId,MemoryDecayRule,IdHash> decay_rules_`, `bool definitions_frozen_=false`, `std::unordered_map<GameplayObjectRef,KnowledgeProfile,RefHash> profiles_`, `std::unordered_map<KnowledgeRecordId,KnowledgeRecord,IdHash> knowledge_`, `std::unordered_map<MemoryRecordId,MemoryRecord,IdHash> memories_`, `std::unordered_map<GameplayObjectRef,std::vector<KnowledgeRecordId>,RefHash> knowledge_by_owner`, `std::unordered_map<GameplayObjectRef,std::unordered_map<KnowledgeTopicId,std::vector<KnowledgeR`, `std::unordered_map<GameplayObjectRef,std::unordered_map<GameplayObjectRef,std::vector<Knowledge`, `std::unordered_map<GameplayObjectRef,std::vector<MemoryRecordId>,RefHash> memories_by_owner_`.

Публичные сигнатуры операций:

```cpp
explicit KnowledgeService(std::size_t change_capacity=4096);
[[nodiscard]] static constexpr GameplayDomainId Domain()noexcept;
[[nodiscard]] foundation::Result<void> RegisterDecayRule(MemoryDecayRule rule);
[[nodiscard]] foundation::Result<void> FreezeDefinitions();
[[nodiscard]] bool DefinitionsFrozen()const noexcept;
[[nodiscard]] foundation::Result<void> CreateProfile(KnowledgeProfile profile);
[[nodiscard]] foundation::Result<KnowledgeRecordId> Learn(LearnKnowledgeRequest request);
[[nodiscard]] foundation::Result<MemoryRecordId> CreateMemory(CreateMemoryRequest request);
[[nodiscard]] foundation::Result<KnowledgeRecordId> Share(ShareKnowledgeRequest request);
[[nodiscard]] foundation::Result<KnowledgeRecordId> Contradict(ContradictKnowledgeRequest request);
[[nodiscard]] foundation::Result<void> Decay(GameplayTimePoint now);
[[nodiscard]] const KnowledgeProfile* FindProfile(GameplayObjectRef subject)const noexcept;
[[nodiscard]] const KnowledgeRecord* FindKnowledge(KnowledgeRecordId id)const noexcept;
[[nodiscard]] const MemoryRecord* FindMemory(MemoryRecordId id)const noexcept;
[[nodiscard]] const MemoryDecayRule* FindDecayRule(MemoryDecayRuleId id)const noexcept;
[[nodiscard]] std::vector<KnowledgeRecord> FindKnowledgeByOwner(GameplayObjectRef owner)const;
[[nodiscard]] std::vector<KnowledgeRecord> FindKnowledgeByTopic(GameplayObjectRef owner,KnowledgeTopicId topic)const;
[[nodiscard]] std::vector<KnowledgeRecord> FindKnowledgeAboutSubject(GameplayObjectRef owner,GameplayObjectRef subject)const;
[[nodiscard]] std::optional<KnowledgeRecord> GetLastKnownPosition(GameplayObjectRef owner,GameplayObjectRef subject)const;
[[nodiscard]] std::vector<MemoryRecord> FindMemoriesByOwner(GameplayObjectRef owner)const;
[[nodiscard]] std::vector<MemoryRecord> FindImportantMemories(GameplayObjectRef owner,MemoryImportance min_importance)const;
[[nodiscard]] KnowledgeChangeBatch ReadChangesSince(ChangeCursor cursor) const;
[[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept;
[[nodiscard]] KnowledgeSnapshot CaptureSnapshot()const;
[[nodiscard]] foundation::Result<void> RestoreSnapshot(KnowledgeSnapshot snapshot);
[[nodiscard]] KnowledgeDiagnostics GetDiagnostics()const noexcept;
[[nodiscard]] Revision CurrentRevision()const noexcept;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.
- `std::span` является невладеющим представлением; вызывающий отвечает за срок жизни исходного буфера.
- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `KnowledgeService` | `KnowledgeService::KnowledgeService` | `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:54` |
| `RegisterDecayRule` | `KnowledgeService::RegisterDecayRule` | `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:69` |
| `FreezeDefinitions` | `KnowledgeService::FreezeDefinitions` | `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:80` |
| `CreateProfile` | `KnowledgeService::CreateProfile` | `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:82` |
| `Learn` | `KnowledgeService::Learn` | `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:129` |
| `CreateMemory` | `KnowledgeService::CreateMemory` | `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:184` |
| `Share` | `KnowledgeService::Share` | `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:224` |
| `Contradict` | `KnowledgeService::Contradict` | `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:246` |
| `Decay` | `KnowledgeService::Decay` | `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:276` |
| `FindProfile` | `KnowledgeService::FindProfile` | `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:299` |
| `FindKnowledge` | `KnowledgeService::FindKnowledge` | `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:300` |
| `FindMemory` | `KnowledgeService::FindMemory` | `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:301` |
| `FindDecayRule` | `KnowledgeService::FindDecayRule` | `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:302` |
| `FindKnowledgeByOwner` | `KnowledgeService::FindKnowledgeByOwner` | `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:304` |
| `FindKnowledgeByTopic` | `KnowledgeService::FindKnowledgeByTopic` | `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:308` |
| `FindKnowledgeAboutSubject` | `KnowledgeService::FindKnowledgeAboutSubject` | `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:312` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `RegisterDecayRule`: `EngineFramework/DevelopmentInfrastructure/Tests/knowledge_memory_tests.cpp:64`
- `CreateProfile`: `EngineFramework/DevelopmentInfrastructure/Tests/knowledge_memory_tests.cpp:77`
- `Learn`: `EngineFramework/DevelopmentInfrastructure/Tests/knowledge_memory_tests.cpp:81`
- `FindKnowledge`: `EngineFramework/DevelopmentInfrastructure/Tests/knowledge_memory_tests.cpp:86`
- `FindKnowledgeByTopic`: `EngineFramework/DevelopmentInfrastructure/Tests/knowledge_memory_tests.cpp:104`
- `Share`: `EngineFramework/DevelopmentInfrastructure/Tests/knowledge_memory_tests.cpp:107`
- `Contradict`: `EngineFramework/DevelopmentInfrastructure/Tests/knowledge_memory_tests.cpp:140`
- `CreateMemory`: `EngineFramework/DevelopmentInfrastructure/Tests/knowledge_memory_tests.cpp:154`
- `FindMemory`: `EngineFramework/DevelopmentInfrastructure/Tests/knowledge_memory_tests.cpp:158`
- `FreezeDefinitions`: `EngineFramework/DevelopmentInfrastructure/Tests/knowledge_memory_tests.cpp:178`
- `Decay`: `EngineFramework/DevelopmentInfrastructure/Tests/knowledge_memory_tests.cpp:193`
- `CaptureSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/knowledge_memory_tests.cpp:212`

