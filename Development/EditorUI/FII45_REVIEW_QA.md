# Проверка исправлений регрессии F-II.4 / F-II.5

Закройте старый экземпляр и запустите EXE ниже. Используйте копию проекта; разверните окно. Для текущей приёмки перейдите к разделу «Финальный ручной прогон перед freeze F-II»: более ранние пункты ниже сохранены как справочник. Уже принятый полный прогон повторять не требуется. Оба gate ожидают подтверждения оставшихся проверок.

Новая сборка, прошедшая standalone smoke и packaged-регрессии:

```powershell
& 'D:\MyProjects\Epidemic engine v2.0\Temp\U\verification-images\8b28b81e5f534452b8274e111f01d4a1\EpidemicEditor\EpidemicEditor.exe'
```

1. **Папки и Documents Move.** В Explorer нажмите ПКМ по Documents → New Folder, создайте `Сцены`. Отдельно создайте папку через ПКМ по Scripts и по самому корню проекта. Создайте Scene/Graph, выберите документ → Document Actions → Move Document; укажите `Сцены` без префикса Documents. Документ появляется внутри папки, открытая вкладка продолжает работать. Undo/Redo перемещения, Save All, перезапуск: структура сохранена. Для возврата в Documents укажите пустую строку. Documents — организация документов по пути имени; физическое версионированное хранилище и стабильные ID не перемещаются.
2. **Выбор Prefab.** Один раз выберите Prefab в Project Explorer. В Prefab Editor сразу должны появиться поля корневой сущности, Add Component и Attach Graph. Измените Health, Undo/Redo. Для редактирования дочернего узла по-прежнему можно выбрать его через Hierarchy.
3. **Sprite.** Назначьте Prefab прямоугольную текстуру размером, например,256×128. Fit Sprite to Prefab сохраняет пропорции и уменьшает её до границ1×1 локальной единицы; небольшие текстуры не увеличивает. Transform самого Prefab не меняется. Reset Sprite Size возвращает исходные64 pixels/unit. Remove Sprite снимает привязку, но не удаляет ассет. Назначьте текстуру снова; Save/Open сохраняет выбранное состояние.
4. **Кнопки.** Уменьшите ширину Prefab Editor примерно до220px. Choose/Remove/Fit/Reset Sprite, Add/Remove Component, Attach Graph, Create/Delete Prefab Child должны переноситься на следующие строки и оставаться доступными. При расширении строки снова уплотняются; соседние dock-панели сами размер не меняют.
5. **Auto и замок типов.** В Palette должны быть один Input и Output в Data, по одному Add/Multiply/Subtract/Divide и режиму округления в Math. ПКМ → Add показывает те же пункты. Добавьте Event, Input, Multiply, Output. Input.value→Multiply.left; Multiply.result→Output.value; Event.next→Output.in. Введите Input=1.25, right=2.5: Auto показывает f64, Preview=3.125. Поставьте оба значения целыми: Auto возвращается к i64, если нет других дробных литералов/фиксированных f64. На Multiply через замок выберите Lock i64; попытка ввести2.5 должна отвергаться. Переведите Multiply в Auto, Output в Lock f64: соседи Auto согласуются в f64. Попытка соединить с этой цепью отдельный Input, запертый в i64, отклоняется без изменения графа. Замок доступен и в Graph Inspector. Undo/Redo отменяет типы вместе с изменением; Save/Open сохраняет режимы и значения. Старые f64-графы продолжают открываться.
6. **Остальные Math.** Для f64 проверьте Subtract(1.25,2.5)=−1.25, Divide(7.5,2)=3.75, Integer Divide(−7.5,2)=−3. Round Up(−1.2)=−1, Round Down(−1.2)=−2. Round (0.6 up) в режиме Auto/f64 использует именно указанный вами порог:1.5→1,1.6→2,−1.6→−2. Деление на0, NaN/Inf и переполнение должны давать диагностику, а не неверное число или crash. Целочисленные и дробные варианты выбираются Auto/замком; дублирующих пунктов палетки нет.
7. **Главный блокер — Damage/Wait.** Откройте Damage Wait Damage из основного прогона. Нажмите Compile: успешная проверка не должна требовать Play. Run Preview объясняет, что native-операциям нужен живой Entity; он не наносит урон авторскому Prefab. Attach Graph к корню Target, сохраните. В Game Controls выберите нужную Scene и нажмите **Play**, а не Run Preview. В Runtime Inspector Health100→93→86, между вызовами проходит заданное число ticks. Pause/Step/Restart работают, Stop возвращает авторское100.

Дополнительно: в Code Space проверьте Ctrl+Z/Ctrl+Y (Undo/Redo) и Ctrl+Delete (удаление строки). Закройте и сразу переоткройте сохранённый Graph, в том числе после Save All: запрос Open не должен теряться во время завершения native-команды.

Пункт8 ранних регрессий теперь подробно расписан в [основной инструкции](FII45_MANUAL_QA.md): Entry/Output/Is Entity Valid, Alt+drag на B, Missing/Undo и component-вариант. Повторите его, затем продолжайте основной прогон с пунктов14–25 и Arena1–16; пункт13 уже покрыт адресным пунктом7. Если конкретный пункт не запускали, отметьте NOT RUN. Старые сообщения об ошибке могут оставаться в истории Problems; оценивайте новые действия и текущий отчёт Graph.

## Повторная приёмка20 ноября2026

Владелец подтвердил адресные2–4,6–7 и F-II.4:13–15,17–20,22–23,25. Пункт15 требует только пояснения, повторной визуальной проверки не запрашиваем. Исправления для повторения: пустые папки имеют значок папки; адресный5 Auto/Lock; основной16 Wait/Go to Node и независимое закрытие Problems. Основной21 отложен;24 требует уточнённой проверки ниже. F-II.5 ещё не проверялся. Оба gate PENDING MANUAL.

### Пункт15: действия Debugger

- Pause останавливает единый native clock всех активаций; удерживаемый ввод освобождается. Continue запускает ту же сессию, продолжая остановленную активацию; уже подтверждённая граница breakpoint не должна немедленно останавливать тот же вызов повторно.
- Step Tick доступен в Paused: прибавляет ровно один игровой tick всему Runtime. За него могут выполниться несколько узлов и несколько активаций. Это не «одна нода».
- Step to Node работает с выбранной activation: выполняет ограниченный переход к следующей границе узла без продвижения GameClock. Один шаг не обязан менять Health: сначала могут выполняться Event/Input/граница перед Native Call. Только исполненный Damage меняет100→93. Waiting до wake tick не завершается от Step to Node; используйте Step Tick или Continue.
- Авторский Breakpoint+Apply сохраняется в Graph и копируется в новую Play-сессию. Toggle Breakpoint в Debugger изменяет только текущую сессию: приоритет имеет выделенная авторская Graph-нода, иначе выбранная runtime activation. На Native Call остановка происходит до побочного эффекта. После Stop снимите авторский флаг отдельно, если он больше не нужен.
- Go to Node открывает текущий узел выбранной runtime activation, независимо от прежнего авторского выделения. Open C# Source открывает исходник только у текущего managed-узла с source mapping; для Wait/native-only узла кнопка недоступна.
- Activations/Values показывает независимые ID, текущее состояние и native значения портов. Trace/Filters показывает уже произошедшие события; выбранная историческая строка не перематывает игру.

### Пункт16: короткий повтор исправления

Поставьте breakpoint на первом Damage, Play, затем Step Tick до Waiting. В активации/подсветке текущим узлом должен быть Wait, а не первый Native Call. Выделите в авторском графе первый Native Call, выберите ожидающую activation в Debugger и нажмите Go to Node: открывается Wait. Step to Node не меняет tick/wake; Step Tick приближает wake на1. Откройте Problems рядом с Debugger и закройте именно вкладку Problems её крестиком: Debugger/Output остаются. Откройте Problems снова через список панелей. Отдельные вкладки нижней группы могут закрываться независимо.

### Пункт23: две сущности

У двух placement должны быть разные entity/activation IDs, независимые Health и одинаковая для каждого длительность собственного Wait. Описанный владельцем сдвиг первых вызовов на3 ticks сам по себе не ошибка: начала активаций могут различаться. Сравнивайте разность tick второго и первого Damage внутри одной activation, а не одновременность двух placement. WASD является общим входом native-сессии: оба объекта с соответствующим Movement получают его, сохраняя собственную скорость.

### Пункт24: Profiler пошагово

1. Откройте Game, Runtime Inspector и Profiler, запустите сцену с двумя Target. Дайте ей2–3 секунды. В таблице у доступных метрик должны быть число, единица и источник. `Unavailable` допускается только для действительно отсутствующего provider.
2. Найдите Fixed ticks: при Play число растёт. Pause на секунду: ticks стоят. Step Tick: ровно+1; Step to Node: без прибавки. После Restart начинается новая сессия. Это тот же clock, который показывает Debugger.
3. Найдите Tick p50/p95: это длительность измеренного native tick в указанной единице, а не FPS. p95 характеризует более медленный хвост; при достаточной выборке он не ниже p50. У короткой сессии/пустой выборки возможен Unavailable. CPU total — накопленные миллисекунды CPU, не процент загрузки.
4. Найдите HWND1 FPS и Frame p50/p95. Это реальная частота подачи кадров и интервал между ними за ограниченное окно last256; не GPU execution time. Сравните Present60/120: меняется целевая частота показа, фактическая зависит от нагрузки. GameClock/длительность Wait не должны ускоряться. На Pause кадры могут продолжать рисоваться при неподвижной игре.
5. Вынесите второй Game в отдельное окно и держите оба видимыми. Появляются строки второго HWND, его FPS/размер измеряются отдельно; Runtime session/tick остаются общими. При закрытии второго окна его строки исчезают. Resize меняет physical px, не authoring Transform.
6. Посмотрите Entities/Activations, память backend/UI, preview residency и очереди импорта/build. Сопоставьте счётчики с текущей сценой и выполняемой работой. После Stop live entities/activations очищаются; освобождение working set ОС не обязано быть мгновенным. GPU duration, allocations и JCEF renderer RAM без provider остаются Unavailable, а не выдуманными нулями. Dropped catch-up ticks может увеличиваться при перегрузке; это диагностический счётчик, его ноль не гарантируется на любой машине.

Реальный FPS уже реализован в Profiler; новый FPS HUD поверх Game View на этом этапе не вводится. Продолжение после этих пунктов: основной21 и весь F-II.5/Arena из FII45_MANUAL_QA.md.

## Финальный оставшийся ручной прогон перед freeze F-II

Предыдущие адресные1–7, F-II.4:15/16/24 приняты владельцем. F-II.5 приняты1–4,6–7,9–13,16;5/8 исправлены и ожидают повторной приёмки,14/15 ещё NOT RUN. F-II.4:21 остаётся NOT RUN. Gate не закрывается до подтверждения перечисленного.

1. **Новые действия папок.** В Documents создайте `Папка/Внутри`, поместите туда Graph; создайте соседнюю папку назначения. ПКМ → Rename Folder, затем Move Folder (вводится существующая папка-родитель относительно проекта, например `Documents/Назначение`). Вкладка Graph/ID/ссылки сохраняются. Undo/Redo возвращает папку и имена документов одним шагом. Save/Open сохраняет результат. Удаление непустой папки с неиспользуемым Graph должно закрыть вкладку; Undo возвращает документ. Папка с entry Scene или внешними сильными ссылками удалению не подлежит: отказ сохраняет данные.
2. **Content.** Повторите Rename/Move для папки с PNG, уже присвоенным Prefab: AssetId и Sprite остаются. Delete убирает папку в приватную корзину; Undo восстанавливает bytes/ID/привязки. Save/Open после Move сохраняет paths. Корни Content/Documents/Scripts и сервисные ProjectData/Cache не переименовываются и не удаляются.
3. **Scripts и обычная папка в корне.** Создайте подпапку с C# файлом, откройте его в Code Space, выполните Rename/Move. Файл остаётся редактируемым по новому пути; Build & Register видит его. Delete закрывает редактор файла, исключает его из активных sources и сохраняет bytes в `.epidemic/FolderTrash`; зарегистрированные component data сами не удаляются. Для Scripts/root действуют VFS/file semantics; единый native Undo шага выше заявлен для Content/Documents. Затем пересоберите C#, если перемещали source.
4. **F-II.5:5, InputBindings.** Stop. Откройте существующий Arena Player Turret Prefab, выберите root/component gameplay.inputbindings. Значения left65/right68/up87/down83/attack32 должны быть видны. Поменяйте right на76(L), left на74(J), Save All, Play: J/L двигают, A/D больше не действуют на Player. Stop, Save/Open новым процессом:74/76 сохранены. Верните65/68. `Value unavailable` у этих полей больше не допускается; блокировка во время Play ожидаема.
5. **F-II.5:8, Collider.** Stop. Arena Enemy Prefab → gameplay.collider2d: shape/radius/half_x/half_y/layer/mask/trigger видны и редактируются. shape0=AABB,1=Circle. Проверьте оба режима, затем mask0: снаряды больше не наносят Enemy collision damage. Верните shape1/radius0.3/layer2/mask5. Projectile: layer4/mask2/trigger=true. Trigger создаёт контакт/попадание без физического выталкивания; Rigidbody/CCD не заявлены.
6. **F-II.4:23, чистый повтор.** Stop; снимите авторские Breakpoint через Graph Inspector+Apply. Начните новую сессию без Pause/Step и runtime breakpoints. На сцене два placement одного Native Damage/Wait/Damage prefab с одинаковыми входами. Сравните Damaged tick по Entity/activation и повторите запуск. В базовом профиле без перегрузки оба одинаково запущенных Graph должны давать один и тот же результат по тикам. Если повтор с теми же настройками даёт разный порядок/tick — это непрошедшая проверка; freeze пока не подтверждать. Независимые события разных стартовых ticks не означают одну AoE-операцию.
7. **F-II.4:21, Console/Problems.** Две Play-сессии → Console/Trace Filters: severity, Current session only, Entity/Graph/Activation/Tick, поиск. Copy копирует строки. Двойной клик/Go to Source ведёт к доступной Graph-нode/C# file:line; Go to Entity выбирает правильный placement. В Problems нет Info/успешных сообщений. Закрытие Problems оставляет Debugger.

### F-II.5:14 — C# параметры пошагово

- Stop. Откройте `Scripts/ArenaManagedTurret.cs`. В компоненте `arena.ManagedTurret` поля Damage/Range/FireRate должны иметь source Schema default для проверки именно defaults. Уже переопределённое поле сохраняет свой override; для чистого теста снимите override или удалите/добавьте только managed component и снова прикрепите `Managed Arena` к нему. Исходный Graph root Entity сохраняйте.
- Измените `Damage = 7` на14, Build & Register C#, дождитесь успешного результата. Убедитесь, что используется актуальная generation. Save All, Play: C# DamageSelf один раз меняет native Health100→86; `gameplay.turret.damage` в Runtime Inspector становится14. Новые projectile получают14. Смотрите native Health, не одноимённое поле другого C# компонента.
- Stop. Поставьте Range2 и FireRate2 в defaults, снова Build & Register/Save/Play. В Runtime Inspector native `gameplay.turret.range=2`, `cooldown=30` ticks, так как cooldown=60/FireRate. Удерживайте Space/ЛКМ при наличии цели: поражаются цели в новом range, интервалы стрельбы соответствуют cooldown; UI polling/FPS не являются игровыми ticks.
- Stop, Damage100, Build & Register/Play: native Health0, Died один раз для Player. Если Entity уже удалена, ориентируйтесь на Console/Trace события этой Entity. На отдельном запуске Damage−1 либо FireRate0 вызывает диагностируемый отказ/Failed, не crash; Stop работает. Ошибка не должна считаться успешной игровой логикой.
- Верните Damage7/Range6/FireRate4, Build & Register и уберите тестовые overrides. Новый Play снова даёт Health93.

### F-II.5:15 — граница authoring/runtime

- Stop: в авторском Player Prefab Health/points100 и gameplay.state.currency0. Затем Play с восстановленным Damage7: Runtime Inspector показывает Health93 и currency≥1 после Award/убийств.
- Runtime Inspector read-only. Save All во время игры, если команда доступна, сохраняет авторские документы; это не savegame.
- Stop: авторские Health100/currency0 и исходный Transform. Следующий Play начинает новую сессию, а не продолжает накопленные HP/currency прошлого запуска. Значения runtime не должны проникать в Prefab после Save/Stop.
- Save/Undo/Redo относятся к авторским правкам. Для дополнительного контроля после Stop измените speed, Undo/Redo, Save; игровой прогресс при этом не возвращается.

После PASS всех пунктов выше можно фиксировать F-II.4/F-II.5 и общий F-II-GATE; до этого они PENDING MANUAL. Полный старый прогон повторять не требуется.
