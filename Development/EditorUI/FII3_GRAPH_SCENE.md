# F-II.3 — Graph ↔ Scene

Отправная точка: `abdc2ec` (F-II.2). Computer Use и push не применяются.

## Native контракт

GraphDefinition/NodeDefinition владеет optional GraphReference, а не editor settings или JS. Payload v1 содержит kind, document, placement, prefab, entity, component, part. Существующие архивы без payload читаются как прежде. Имена, HWND, source revision, drag nonce и runtime handles не сохраняются.

Scene Entity без Prefab: document=Scene, placement=SceneNodeId. Instance/Entity/Component placement: document=Scene, placement=SceneNodeId, prefab=AssetId, entity=materialized PrefabNodeId; Component дополнительно хранит canonical TypeId и EntityPartId. Prefab-owned Entity/Component: document=prefab, placement=0. Prefab Definition: document=prefab, остальная instance identity пуста. Два placements не alias. Смешанные scene/prefab scopes в одном Graph отклоняются; абсолютные межсценовые ссылки не поддерживаются.

Reference и Is Reference Valid — настоящие операции native compiler/IR/VM с отдельными типами портов, включая точный Component type. Ссылка не преобразуется в i64; только результат Is Valid равен 0/1. Runtime resolver проверяет существующий live PrefabInstance/ComponentStore в текущей Play-сессии и scope. После удаления runtime-цели используется Missing, не cached handle. Авторские ссылки переживают rename/reparent/delete/Undo; отсутствие цели не удаляет payload.

IPC envelope v4 сохранён, Graph DTO v5. Команда 40: probe/drop/jump/uses. Drop несёт bounded stable-ID payload в одном аргументе и остаётся в прежнем общем лимите IPC. Probe не изменяет revision. Drop повторно проверяет target/scope/type/occupied input и выполняет AddGraphNode + MoveGraphNode + optional ConnectPorts одним EditorWorkspace::Execute.

## UI

Alt+drag от reference input в Graph к Scene/Hierarchy создаёт ссылку и link. Alt+drag Entity в Hierarchy/native viewport к Graph создаёт reference node; drop на совместимый input добавляет link. Если вариантов несколько, предлагается явный выбор Entity/Instance/Definition/Component/part. Обычные жесты Rete, hierarchy move и gizmo сохранены. Нет implicit spawn.

Один project-scoped GraphSceneDragCoordinator передаёт только JVM-local UUID через Windows OLE DnD (AWT DragSource/DropTarget). HTML5 DnD не является межоконным transport. Нативный probe управляет допустимостью preview. Короткая сессия отзывается при Escape/failed OS drop/закрытии view/смене проекта/backend generation/revision/timeout. Поздний и повторный Drop не авторизуются nonce. AWT→CEF координаты нормализуются к фактическому CSS viewport каждого окна.

Rete показывает типизированные reference nodes и Missing Reference; Node Inspector содержит стабильную identity, Jump to Reference Target и Find Target References. Hover не меняет native Selection или проект.

## Проверки

PASS: native Graph VM typed reference/resolver regression; native Bridge roundtrip с двумя placements, Component/part/Definition, atomic Undo, rename/reparent/Delete/Restore, wrong type/scope, 100 transient session/cancel cycles, duplicate/stale Drop, Save/Open другим процессом.

Runtime Play regression PASS: удаление настоящего Entity/Component второй placement наблюдается resolver без изменения authoring snapshot. Адресный standalone JCEF smoke PASS: AWT/CSS hit, native probe/transaction, DTO/DOM Missing и Restore; прежние Source/Graph/lifecycle smoke также PASS. Полный Debug CTest **161/161 PASS**, public headers, IPC adversarial/catalog и managed bridge/contract/architecture PASS. Native Release build PASS. Static проверил восемь изменённых TU прежними clang-analyzer/bugprone checks; новая optional-access диагностика исправлена, повтор backend не содержит новых диагностик. Общий static FAIL сохраняется на прежних Scene signed-bitwise, Composition/Editor aggregate move/exception и ThumbnailCache parameters; подавлений/ослаблений нет. Финальный свежий standalone Release smoke **PASS**; SDK/Graph/GraphScene roundtrip на backend именно из этого образа **PASS**. Проверенный EXE: `Temp/U/verification-images/94a7464f6d3c4a4cb254cf759e194094/EpidemicEditor/EpidemicEditor.exe`. Во время smoke исправлены отмена drag от собственного Loading probe и от обновлений посторонних скрытых деревьев; регрессии сохранены. Тесты актуализированы по native-командам и исходному непустому графу; проверки не отключались.

**F-II.3-GATE: PENDING итогового подтверждения.** Владелец принял пункты1–21 адресного прогона 2026-11-16; 17/19/20 сохраняются для регрессий. Замечания компоновки устранены и проверены standalone smoke. Остались короткий визуальный повтор этих изменений и результат пункта22. Конфигурации DPI/мониторов отдельно владельцем не перечислены; повторный Computer Use прогон не выполнялся. F-II.4 не начат; [подготовлен порядок входа](FII4_RUNTIME_CONTRACT.md).

[Ручная проверка](FII3_MANUAL_QA.md).
