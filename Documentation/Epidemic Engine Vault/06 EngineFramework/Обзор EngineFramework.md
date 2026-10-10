---
aliases: []
type: "index"
source_snapshot: "2026-10-10"
tags: ["engineframework", "index"]
---

# Обзор EngineFramework

EngineFramework содержит gameplay-owned data и integration adapters; каждый owner должен иметь явные authoritative state, identity и save/query/mutation contracts. Документы Goal 4 фиксируют LOCAL_READY по модулям, не системное FROZEN.

Исходный индекс: [[90 Первоисточники/Documentation/EngineFramework/README|EngineFramework contract documentation]].

[[02 Архитектура/Граница игрового кода]] · [[07 Сценарии/Игровое состояние и идентификаторы]]

## BaseInfrastructure (5)

| Модуль | Назначение | Public headers |
| --- | --- | ---: |
| [[06 EngineFramework/Модуль Facts|Facts]] | Gameplay facts | 1 |
| [[06 EngineFramework/Модуль Foundation|Foundation]] | Базовые типы, Result, errors и ID | 8 |
| [[06 EngineFramework/Модуль Queries|Queries]] | Read-model queries | 1 |
| [[06 EngineFramework/Модуль SupportRandom|SupportRandom]] | Детерминированная псевдослучайность | 1 |
| [[06 EngineFramework/Модуль Time|Time]] | Runtime game delta и clock | 1 |

## RuntimeBoundary (1)

| Модуль | Назначение | Public headers |
| --- | --- | ---: |
| [[06 EngineFramework/Модуль RuntimeBridge|RuntimeBridge]] | Мост Runtime contracts ↔ Gameplay identity | 1 |

## GameplayWorldStateOwners (33)

| Модуль | Назначение | Public headers |
| --- | --- | ---: |
| [[06 EngineFramework/Модуль AI|AI]] | Gameplay AI contracts | 1 |
| [[06 EngineFramework/Модуль Abilities|Abilities]] | Способности и activation | 1 |
| [[06 EngineFramework/Модуль Combat|Combat]] | Боевые операции и damage | 1 |
| [[06 EngineFramework/Модуль Conditions|Conditions]] | Состояния и условия | 1 |
| [[06 EngineFramework/Модуль Construction|Construction]] | Gameplay construction | 1 |
| [[06 EngineFramework/Модуль Crime|Crime]] | Gameplay crime/legal systems | 1 |
| [[06 EngineFramework/Модуль Dialogue|Dialogue]] | Dialogue state и контракты разговора | 1 |
| [[06 EngineFramework/Модуль Economy|Economy]] | Gameplay economy transactions | 1 |
| [[06 EngineFramework/Модуль Effects|Effects]] | Эффекты игровых действий | 1 |
| [[06 EngineFramework/Модуль Encounters|Encounters]] | Встречи и встречи персонажей | 1 |
| [[06 EngineFramework/Модуль Entities|Entities]] | Gameplay entities и lifecycles | 1 |
| [[06 EngineFramework/Модуль Environment|Environment]] | Нейтральное состояние окружения | 1 |
| [[06 EngineFramework/Модуль Equipment|Equipment]] | Экипировка и equip rules | 1 |
| [[06 EngineFramework/Модуль Interaction|Interaction]] | Игровые взаимодействия | 1 |
| [[06 EngineFramework/Модуль ItemsInventory|ItemsInventory]] | Предметы и инвентари | 1 |
| [[06 EngineFramework/Модуль Knowledge|Knowledge]] | Знания сущностей и источники | 1 |
| [[06 EngineFramework/Модуль Loot|Loot]] | Drops и rewards | 1 |
| [[06 EngineFramework/Модуль Materials|Materials]] | Игровые материалы | 1 |
| [[06 EngineFramework/Модуль Narrative|Narrative]] | Повествовательные структуры и состояния | 1 |
| [[06 EngineFramework/Модуль NavigationSemantics|NavigationSemantics]] | Игровые правила навигации | 1 |
| [[06 EngineFramework/Модуль NeedsLife|NeedsLife]] | Нужды и жизненные потребности | 1 |
| [[06 EngineFramework/Модуль Ownership|Ownership]] | Gameplay ownership | 1 |
| [[06 EngineFramework/Модуль Perception|Perception]] | Gameplay perception | 1 |
| [[06 EngineFramework/Модуль Population|Population]] | Популяция и её развитие | 1 |
| [[06 EngineFramework/Модуль Processes|Processes]] | Игровые процессы | 1 |
| [[06 EngineFramework/Модуль Progression|Progression]] | Gameplay progression | 1 |
| [[06 EngineFramework/Модуль ResourcesProduction|ResourcesProduction]] | Геймплейное производство | 1 |
| [[06 EngineFramework/Модуль RolesJobs|RolesJobs]] | Роли и работа NPC | 1 |
| [[06 EngineFramework/Модуль SaveGame|SaveGame]] | Gameplay save/load, participants и wire | 1 |
| [[06 EngineFramework/Модуль Simulation|Simulation]] | Budgeted proposals и commits | 1 |
| [[06 EngineFramework/Модуль Society|Society]] | Группы и социальные отношения | 1 |
| [[06 EngineFramework/Модуль Traversal|Traversal]] | Прохождение и перемещение персонажей | 1 |
| [[06 EngineFramework/Модуль World|World]] | World ownership, placement, residency | 1 |

## IntegrationLayer (13)

| Модуль | Назначение | Public headers |
| --- | --- | ---: |
| [[06 EngineFramework/Модуль ExtendedGameplayIntegration|ExtendedGameplayIntegration]] | Расширенная композиция gameplay | 1 |
| [[06 EngineFramework/Модуль GameplayIntegration|GameplayIntegration]] | Сопряжение gameplay owners | 1 |
| [[06 EngineFramework/Модуль Integration|Integration]] | Core gameplay adapters | 1 |
| [[06 EngineFramework/Модуль InteractionEffectsIntegration|InteractionEffectsIntegration]] | Связь взаимодействия и эффектов | 1 |
| [[06 EngineFramework/Модуль InteractionTimeIntegration|InteractionTimeIntegration]] | Связь взаимодействий и времени | 1 |
| [[06 EngineFramework/Модуль NarrativeIntegration|NarrativeIntegration]] | Связь narrative services | 1 |
| [[06 EngineFramework/Модуль PerceptionKnowledgeAIIntegration|PerceptionKnowledgeAIIntegration]] | Связь восприятия, знаний и AI | 1 |
| [[06 EngineFramework/Модуль PopulationSimulationIntegration|PopulationSimulationIntegration]] | Связь популяции с симуляцией | 1 |
| [[06 EngineFramework/Модуль ProcessResourceSimulationIntegration|ProcessResourceSimulationIntegration]] | Связь процессов и ресурсов с симуляцией | 1 |
| [[06 EngineFramework/Модуль SocialLegalIntegration|SocialLegalIntegration]] | Связь социальных и юридических систем | 1 |
| [[06 EngineFramework/Модуль StateIntegration|StateIntegration]] | Согласование gameplay state | 1 |
| [[06 EngineFramework/Модуль TraversalNavigationConstructionIntegration|TraversalNavigationConstructionIntegration]] | Связь перемещения, навигации и строительства | 1 |
| [[06 EngineFramework/Модуль WorldIntegration|WorldIntegration]] | Связь gameplay world | 1 |
