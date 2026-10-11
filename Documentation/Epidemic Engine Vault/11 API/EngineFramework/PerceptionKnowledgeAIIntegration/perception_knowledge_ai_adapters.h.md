---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/include/Epidemic/GameFramework/PerceptionKnowledgeAIIntegration/perception_knowledge_ai_adapters.h"
---

# PerceptionKnowledgeAIIntegration: perception_knowledge_ai_adapters.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/include/Epidemic/GameFramework/PerceptionKnowledgeAIIntegration/perception_knowledge_ai_adapters.h`.

**Модуль:** [[06 EngineFramework/Модуль PerceptionKnowledgeAIIntegration|PerceptionKnowledgeAIIntegration]].
**Размер исходного файла:** 5294 байт.
**Подключение:** `#include <Epidemic/GameFramework/PerceptionKnowledgeAIIntegration/perception_knowledge_ai_adapters.h>`.

## Объявления типов (лексический индекс)
`PerceptionKnowledgeMapping`, `ObservationKnowledgeResult`, `PerceptionKnowledgeAdapter`, `AIExecutionAvailability`, `KnowledgeAIInputKeys`, `KnowledgeAIAdapter`, `AIIntentExecutionRecorder`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/GameFramework/AI/ai.h"
#include "Epidemic/GameFramework/Knowledge/knowledge.h"
#include "Epidemic/GameFramework/Perception/perception.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace epidemic::gameplay::integration
{
struct PerceptionKnowledgeMapping
{
    knowledge::BeliefTypeId observed_belief{knowledge::BeliefTypeId::FromString("framework.knowledge.observed")};
    knowledge::KnowledgeTopicId observed_topic{
        knowledge::KnowledgeTopicId::FromString("framework.knowledge.observed_subject")};
    knowledge::MemoryTypeId observation_memory{
        knowledge::MemoryTypeId::FromString("framework.memory.perception_observation")};
};

struct ObservationKnowledgeResult
{
    knowledge::KnowledgeRecordId knowledge{};
    std::optional<knowledge::MemoryRecordId> memory;
    bool memory_creation_failed = false;
    std::optional<foundation::Error> memory_error;
};

using ObservationLearningResult = ObservationKnowledgeResult;

class PerceptionKnowledgeAdapter
{
  public:
    explicit PerceptionKnowledgeAdapter(knowledge::KnowledgeService &knowledge_service,
                                        PerceptionKnowledgeMapping mapping = {});
    [[nodiscard]] foundation::Result<ObservationKnowledgeResult>
    LearnFromObservation(const perception::PerceptionObservation &observation);

  private:
    knowledge::KnowledgeService &knowledge_;
    PerceptionKnowledgeMapping mapping_;
};

struct AIExecutionAvailability
{
    bool materialized = false;
    bool runtime_projection_available = false;
};

struct KnowledgeAIInputKeys
{
    ai::AIInputKeyId knowledge_confidence{
        ai::AIInputKeyId::FromString("framework.ai.target.knowledge_confidence")};
    ai::AIInputKeyId current_perception{
        ai::AIInputKeyId::FromString("framework.ai.target.source.current_perception")};
    ai::AIInputKeyId knowledge_source{ai::AIInputKeyId::FromString("framework.ai.target.source.knowledge")};
    ai::AIInputKeyId knowledge_semantics{
        ai::AIInputKeyId::FromString("framework.ai.target.knowledge_semantics")};
    ai::AIInputKeyId last_known_source{ai::AIInputKeyId::FromString("framework.ai.target.source.last_known")};
    ai::AIInputKeyId perception_confidence{
        ai::AIInputKeyId::FromString("framework.ai.target.perception_confidence")};
    ai::AIInputKeyId identity_confidence{
        ai::AIInputKeyId::FromString("framework.ai.target.identity_confidence")};
    ai::AIInputKeyId position_uncertainty{
        ai::AIInputKeyId::FromString("framework.ai.target.position_uncertainty")};
    ai::AIInputKeyId perception_sense{ai::AIInputKeyId::FromString("framework.ai.target.perception_sense")};
    ai::AIInputKeyId perception_semantics{
        ai::AIInputKeyId::FromString("framework.ai.target.perception_semantics")};
};

class KnowledgeAIAdapter
{
  public:
    explicit KnowledgeAIAdapter(const knowledge::KnowledgeService &knowledge_service,
                                KnowledgeAIInputKeys input_keys = {});
    [[nodiscard]] ai::AIContextSnapshot BuildContext(
        GameplayObjectRef agent, AIExecutionAvailability availability,
        const perception::PerceptionService *perception_service = nullptr, GameplayTimePoint now = {}) const;

  private:
    const knowledge::KnowledgeService &knowledge_;
    KnowledgeAIInputKeys input_keys_;
};

class AIIntentExecutionRecorder
{
  public:
    explicit AIIntentExecutionRecorder(std::size_t retention_capacity = 1024) noexcept;

    [[nodiscard]] foundation::Result<void> Accept(const ai::AIIntent &intent, ai::AIService &ai_service);
    [[nodiscard]] foundation::Result<void> Succeed(ai::AIIntentId intent, ai::AIService &ai_service);
    [[nodiscard]] const std::vector<ai::AIIntent> &AcceptedIntents() const noexcept { return accepted_; }
    [[nodiscard]] std::size_t RetentionCapacity() const noexcept { return retention_capacity_; }
    void Clear() noexcept { accepted_.clear(); }

  private:
    void RecordAccepted(const ai::AIIntent &intent);

    std::size_t retention_capacity_ = 1024;
    std::vector<ai::AIIntent> accepted_;
};

// This object is diagnostic only; AIService remains the authoritative intent state owner.
using AIIntentExecutionLog = AIIntentExecutionRecorder;
} // namespace epidemic::gameplay::integration

// BEGIN GENERATED GOAL5 PARTICIPANTS
#include "Epidemic/GameFramework/SaveGame/save_game.h"
#include "Epidemic/GameFramework/AI/ai.h"
#include "Epidemic/GameFramework/Knowledge/knowledge.h"
#include "Epidemic/GameFramework/Perception/perception.h"
namespace epidemic::gameplay::savegame
{
[[nodiscard]] std::unique_ptr<ISaveParticipant> CreateAIServiceSaveParticipant(epidemic::gameplay::ai::AIService& owner, SaveRestoreWorkspace& workspace);
[[nodiscard]] std::unique_ptr<ISaveParticipant> CreateKnowledgeServiceSaveParticipant(epidemic::gameplay::knowledge::KnowledgeService& owner, SaveRestoreWorkspace& workspace);
[[nodiscard]] std::unique_ptr<ISaveParticipant> CreatePerceptionServiceSaveParticipant(epidemic::gameplay::perception::PerceptionService& owner, SaveRestoreWorkspace& workspace);
}
// END GENERATED GOAL5 PARTICIPANTS
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/GameFramework/AI/ai.h`, `Epidemic/GameFramework/Knowledge/knowledge.h`, `Epidemic/GameFramework/Perception/perception.h`, `Epidemic/GameFramework/SaveGame/save_game.h`, `Epidemic/GameFramework/AI/ai.h`, `Epidemic/GameFramework/Knowledge/knowledge.h`, `Epidemic/GameFramework/Perception/perception.h`.

### Состав типов и операции

#### `ObservationKnowledgeResult` (строка заголовка 23)

Публичные поля и значения по умолчанию:

- `std::optional<knowledge::MemoryRecordId> memory`
- `bool memory_creation_failed = false`
- `std::optional<foundation::Error> memory_error`

#### `PerceptionKnowledgeAdapter` (строка заголовка 33)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] foundation::Result<ObservationKnowledgeResult> LearnFromObservation(const perception::PerceptionObservation &observation);
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

#### `AIExecutionAvailability` (строка заголовка 46)

Публичные поля и значения по умолчанию:

- `bool materialized = false`
- `bool runtime_projection_available = false`

#### `KnowledgeAIAdapter` (строка заголовка 73)

#### `AIIntentExecutionRecorder` (строка заголовка 87)

Публичные сигнатуры операций:

```cpp
explicit AIIntentExecutionRecorder(std::size_t retention_capacity = 1024) noexcept;
[[nodiscard]] foundation::Result<void> Accept(const ai::AIIntent &intent, ai::AIService &ai_service);
[[nodiscard]] foundation::Result<void> Succeed(ai::AIIntentId intent, ai::AIService &ai_service);
[[nodiscard]] const std::vector<ai::AIIntent> &AcceptedIntents() const noexcept;
[[nodiscard]] std::size_t RetentionCapacity() const noexcept;
void Clear() noexcept;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `LearnFromObservation` | `PerceptionKnowledgeAdapter::LearnFromObservation` | `EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/src/perception_knowledge_ai_adapters.cpp:208` |
| `AIIntentExecutionRecorder` | `AIIntentExecutionRecorder::AIIntentExecutionRecorder` | `EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/src/perception_knowledge_ai_adapters.cpp:353` |
| `Accept` | `AIIntentExecutionRecorder::Accept` | `EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/src/perception_knowledge_ai_adapters.cpp:371` |
| `Succeed` | `AIIntentExecutionRecorder::Succeed` | `EngineFramework/IntegrationLayer/PerceptionKnowledgeAIIntegration/src/perception_knowledge_ai_adapters.cpp:380` |

