# Матрица готовности F-II

| Функция | Состояние | Основание |
|---|---|---|
| F-II.0a, сверка и baseline | Implemented · PASS | Свежий SHA совпал с GitHub, Release, адресные CTest и managed baseline прошли. [Baseline](FII_BASELINE.md). |
| Graph authoring Core / VM | Implemented | GraphDefinition, compiler, VM, Add/DeleteGraphNode, Connect/DisconnectPorts, native history и сериализация. |
| Graph Editor и browser | Limited · PARTIAL | Документная Graph tab и выбор native nodes работают. Vanilla JBR даёт false. Публичный JBR с JCEF показал 3 DOM-узла и 1 ребро в Graph tab. Требуется сквозной native DTO/edit path. |
| Code Space | Limited · PARTIAL | Есть текущая центральная вкладка и доказательство EditorFactory на реальном .cs. Продуктовый VFS/Save/conflict путь и LSP client ожидают интеграции. |
| Rider | Planned · NOT RUN | Проверены пути обнаружения и официальный CLI. Реальный launcher на машине не найден. |
| Managed bridge | Implemented / Limited | Opt-in C ABI и Graph adapter проходят контракт. Native hostfxr проверен на двух сборках Turret. Автоматическая регистрация schema пользователя и lifecycle из Play остаются F-II.2. |
| Native Schema, Play/Stop, Scene Renderer | Implemented | Runtime/authoring доступны по существующим контрактам, Scene использует native viewport. |
| Graph↔Scene, Live Game View, Gameplay authoring | Planned | F-II.3, F-II.4 и F-II.5 соответственно. |
| Блокирующий режим текущего продукта | Broken для JCEF на vanilla JBR | Измеренный isSupported=false. Задача C++ authoring baseline при этом прошла. |

Статус и решения по обязательным пробам приведены в [техническом отчёте](FII_TECH_ADR.md).
