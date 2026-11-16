# Ручная проверка F-II.2

Проверять новый standalone EXE из итогового отчёта, в максимизированном окне. Сохраните и закройте прежнюю версию перед переключением на новый образ. Нужен .NET SDK 10 (автоматически проверен 10.0.402); Rider необязателен. Проверки U/F-I целиком повторять не требуется.

1. Создайте тестовый проект в папке с пробелами/кириллицей. File → New C# Class → `Turret`. Должен открыться реальный `Scripts/Turret.cs` с нумерацией строк и подсветкой. В шаблоне есть Damage, Range, FireRate, Health, Enabled, Label и ApplyDamage.
2. Измените комментарий, проверьте текстовые Undo/Redo, Ctrl+F, переход к строке и Ctrl+S. Undo в Code Space должен менять текст, а не сцену. Автосохранение принято владельцем 2026-11-16: после закрытия/открытия проверьте текст и Undo. При внешнем конфликте данные всё равно должны сохраняться до явного разрешения.
3. В Project Explorer раскройте Scripts: двойной щелчок открывает `.cs`, ПКМ → Open With, на Scripts → New C# Class. Asset Delete/Duplicate не должны применяться к выбранному `.cs`.
4. Build → Build & Register C#. Подтвердите доверие собственному коду. UI остаётся отзывчивым; статус сборки виден и в source-вкладке. После успеха Type Registry содержит `user.Turret`, Palette — `Turret.ApplyDamage`. Перезапуск/rebuild EXE не требуется.
5. Откройте Prefab, выберите его root и добавьте компонент `user.Turret` через Inspector. Измените Damage (целое), Range/FireRate (дробное), Enabled и Label (кириллица/пробелы). Проверьте Enter/Escape, Undo/Redo, Save и повторное открытие проекта. Значения и type ID должны сохраниться.
6. Создайте Graph с Entry, Input, Turret.ApplyDamage, Output. У Input задайте Preview default `7`. Соедините:

| Откуда | Куда |
|---|---|
| Entry.next | Turret.ApplyDamage.in |
| Input.value | Turret.ApplyDamage.argument |
| Turret.ApplyDamage.next | Output.in |
| Turret.ApplyDamage.result | Output.value |

Compile и Run Preview: при исходном шаблоне Health=100 результат `93`. Это изолированный native VM preview, не визуальный Game View и не изменение authoring-полей Prefab.

7. Выберите C# node → Node Inspector → Open C# Source. Должен открыться настоящий файл на методе. В Code Space → Find Graph Uses должен быть этот node/Graph. Сохраните и откройте Graph; ID/links остаются.
8. Измените только тело ApplyDamage: вычитайте `amount * 2`, сохранив атрибуты/IDs, сигнатуру и поля. Build & Register, снова Run Preview: `86`. Для отдельной проверки измените Range default и добавьте новое scalar поле: совместимая пересборка должна пройти, ранее заданные overrides сохраняются. Смена типа/удаление прежнего поля требуют отдельной миграции.
9. Внесите синтаксическую ошибку и нажмите Build. В Problems/списке diagnostics должны быть file:line:column; двойной щелчок открывает ошибку. Последняя рабочая schema не исчезает. После исправления и успешной сборки сценарий снова работает. Запуск старого поколения при Rebuild Required должен быть явно обозначен, а не выдан за новый код.
10. Во время сборки проверьте Cancel C# Build; после правки source требуется новая сборка. Во время Play source можно редактировать; новое поколение не публикуется поверх активного Play. Stop → Build → Play работает. Live Game View/Runtime Inspector — следующий F-II.4; runtime-вызов и native mutation здесь проверены headless-тестом.
11. Откройте тот же `.cs` внешним редактором. При чистом Code Space проверьте обновление. Затем измените обе копии: Save не должен молча затереть внешний файл. Resolve External Change → Compare/Reload/Save As; обе версии должны быть доступны до явного выбора.
12. Open With → Rider: при установленном Rider проверьте правильный файл/строку; без Rider должен работать fallback. Проверьте неверный EXE-путь, Remember my choice и Tools → Code Space Preferences, включая project override/Use Global. Машинный путь не должен появиться в файлах проекта.
13. Разделите source-документ/вынесите в другое окно. В обеих views должен быть один Document; текст, Undo и dirty синхронизированы. Проверьте Ctrl+S/Ctrl+Shift+S в отдельном окне, закрытие peer view и повторное открытие. Close Project должен закрыть source-вкладки во всех окнах.
14. Сделайте несколько Play/Stop и Build циклов, затем закройте Editor. Собственный ScriptWorker не должен оставаться. Повторный запуск/Save/Open сохраняет native Graph и user component данные. Если меняется source вне приложения, перед запуском сохранённой сборки должен быть понятен её статус.

Сообщите номера PASS/FAIL и короткие шаги для FAIL. Реальный Rider, фокус/shortcuts и вид двух окон пока **MANUAL NOT RUN**, без Computer Use. F-II.2-GATE закроем после этой приёмки.
