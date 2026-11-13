# Матрица готовности F-II

| Функция | Состояние | Основание |
|---|---|---|
| F-II.0a, сверка и baseline | Implemented · PASS | Свежий SHA совпал с GitHub, Release, адресные CTest и managed baseline прошли. [Baseline](FII_BASELINE.md). |
| Graph authoring Core / VM | Implemented | GraphDefinition, compiler, VM, Add/DeleteGraphNode, Connect/DisconnectPorts, native history и сериализация. |
| Graph Editor и browser | Implemented F-II.1 · AUTO PASS / manual NOT RUN | Native catalogue, Rete projection/Selection, Inspector/defaults/metadata, atomic fragments/bulk edits, compile/native VM/scheduler, diagnostics, offline sandbox EXE. [Отчёт и границы](FII1_GRAPH_AUTHORING.md). |
| Code Space | Limited · PARTIAL | Есть текущая центральная вкладка и доказательство EditorFactory на реальном .cs. Продуктовый VFS/Save/conflict путь и LSP client ожидают интеграции. |
| Rider | Planned · NOT RUN | Проверены пути обнаружения и официальный CLI. Реальный launcher на машине не найден. |
| Managed bridge | Implemented / Limited | Opt-in C ABI и Graph adapter проходят контракт. Native hostfxr проверен на двух сборках Turret. Автоматическая регистрация schema пользователя и lifecycle из Play остаются F-II.2. |
| Native Schema, Play/Stop, Scene Renderer | Implemented | Runtime/authoring доступны по существующим контрактам, Scene использует native viewport. |
| Graph↔Scene, Live Game View, Gameplay authoring | Planned | F-II.3, F-II.4 и F-II.5 соответственно. |
| F-II.0b / вход в F-II.1 | PASS / READY | Настоящий CEF sandbox и native roundtrip квалифицированы. F-II.1 реализован; автоматические функциональные проверки PASS, ручной gate ожидает владельца. |

Статус и решения по обязательным пробам приведены в [техническом отчёте](FII_TECH_ADR.md).

Контракты F-II.2: [Code Space / managed](FII_CODESPACE_MANAGED_CONTRACT.md). F-II.1 реализован; ручная приёмка по [инструкции](FII1_MANUAL_QA.md) NOT RUN. F-II.2 не начат.
