# Ручная проверка F-II.3

Использовать `Temp/U/verification-images/94a7464f6d3c4a4cb254cf759e194094/EpidemicEditor/EpidemicEditor.exe`, сначала максимизированное окно. Сохранить проект перед тестом. F-II.2 также ожидает ручной приёмки по FII2_MANUAL_QA.md.

1. Создать Prefab с компонентами Health/Movement и два placements одного Prefab в Scene. Открыть Graph и Scene одновременно, затем вынести Scene в независимое Windows-окно.
2. В Palette добавить Is Entity Valid. Alt+drag его input reference к первому placement в Scene или Hierarchy. Допустимая цель подсвечивается после native probe. Отпустить: один reference node и link; Scene count не меняется. Undo убирает оба, Redo возвращает.
3. Alt+drag второго placement из Hierarchy или viewport в пустое место Graph. Выбрать Reference Entity, Instance или Definition. Повторить drop непосредственно на подходящий input. У двух Entity references разные placement IDs при одном prefab/entity ID.
4. Добавить Is Health Valid (имя соответствует зарегистрированному canonical type). Повторить оба направления; выбрать конкретный Component/part. Component другого типа, i64/control/event и занятый input не должны давать допустимую цель или изменять revision.
5. Выбрать reference node → Node Inspector. Проверить identity, Jump to Reference Target и Find Target References. Rename и Reparent цели сохраняют node ID и живую ссылку. Delete показывает красный Missing Reference, Undo оживляет тот же node ID.
6. В Graph с Entry/Output соединить Entry.next→Output.in, Is Entity Valid.valid→Output.value и reference→Is Entity Valid.reference. Compile/Run Preview даёт valid=1, для отсутствующей цели — 0. Ссылки не являются i64 input.
7. Повторить оба направления из Prefab Hierarchy. Явно различать Prefab Definition и конкретный placement. Попытка смешать разные Scene scopes отклоняется с причиной.
8. Начать drag и нажать Escape; отпустить вне окна; закрыть source/target вкладку или окно; сменить проект; перезапустить backend. Каждый раз ноль частичных edits, временная подсветка исчезает, следующий обычный drag работает.
9. Save, полностью закрыть приложение, запустить новый процесс, открыть проект. Проверить IDs/links/координаты, обе placements и Component/part, Missing/Restore. Несколько Play/Stop не должны менять authoring references.
10. Повторить в отдельных Windows-окнах, на втором мониторе и при DPI 100/150%, включая разные масштабирования мониторов. Проверить попадание в точный socket/entity и фокус клавиатуры после Drop. Эти физические сценарии остаются NOT RUN до подтверждения владельца.
