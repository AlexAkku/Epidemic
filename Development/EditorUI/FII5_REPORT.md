# F-II.5 — Gameplay Framework

Отправная точка: `763dbc3`. Работа выполнена одним агентом, без Computer Use и push. F-II.4 и F-II.5 требуют отдельного ручного подтверждения владельца; общий F-II-GATE автоматически не закрывается.

## Реализация тремя частями

1. Общий C++ gameplay owner в Game/Composition, Schema компонентов и единый каталог операций для Graph/C#. Health, validation, authoring/runtime isolation — адресный native чекап PASS.
2. Движение, input/bindings, AABB/Circle overlap, masks/triggers, projectile/lifetime/hit, targeting/cooldown, waves/state и именованные события в существующем fixed tick. Native Arena Graph/Wait/Pause/Step/Restart/Stop — второй чекап PASS.
3. ScriptApi.Gameplay → worker RPC v3 → тот же native owner; production Arena action с обычными документами, PNG import/assignment и C# исходником; сквозные регрессии, упаковка и общая инструкция. Итоговые результаты ниже.

Ни Java, ни Rete, ни C# facade не исполняют собственный игровой цикл. GameClock/PlaySession/Runtime Scheduler/VM, EntityService, ComponentStore, PrefabComposer и Runtime Scene остаются действующими владельцами. Private RPC v3 не меняет public managed C ABI v1 или Editor IPC v4. [Контракт](FII5_GAMEPLAY_CONTRACT.md).

## Проверки и найденные ошибки

- Чистая full-engine Debug сборка PASS (2429.21s), затем CTest162/162 PASS (346.68s). Профиль `EPIDEMIC_BUILD_MANAGED_BRIDGE=OFF`: native Arena работает без managed game features.
- Ревизия добавила реально падавшую регрессию: Died Graph отменялся при слишком раннем Destroy. Исправлена последовательность: обработчику даётся один scheduler pass до удаления, при debugger Pause scope сохраняется; Stop отменяет работу сразу. Сохранены проверки однократной смерти, cancel, generation и отсутствия dangling callbacks.
- Исправлены доступы к optional в новом тесте, defaults и параметры нового API; shutdown и журналы Entity ограничены, записи/Scene nodes освобождаются после despawn. Native1000 cycles: handlesΔ0, private bytes около2.7–2.9MiB на первом прогоне с bounded journal/allocator; Entity records, Scene nodes и jobs возвращаются к baseline. Это не заявление о бесконечной нагрузке.
- Исправлен повторный startup workspace в Arena action; context не должен переинициализироваться при добавлении sample. Time scale читается из C++ runtime metrics всеми Game-окнами.
- Финальная полная пересборка PASS (284.38s); последний CTest **162/162 PASS**,0FAIL по актуальному LastTest.log. Адресный native батч с Died breakpoint и1000 cycles — PASS.
- Deep:6headers/28 реальных production/test TU/56checks. Generated one-line header consumers проверены полной C++ сборкой и не дублировались сотни раз в temporary analysis DB. После исправлений адресный повтор подтверждает **0 новых диагностических позиций**; остаются33 inherited и1 toolchain. Общий static результат **FAIL/backlog**, не PASS; настройки/предикаты не ослаблены.

## Standalone / packaged

Standalone Release build и полный JCEF smoke **PASS**, включая production Arena action, PNG import/assignment, реальные native projectile pixels, crash/reconnect и штатное завершение без owned launcher/CEF процессов за15s. Packaged Runtime, Graph↔Scene, Graph и C# **PASS**. Финальный оконный повтор с актуальным host lifetime и последующие IPC v4, catalog и managed architecture проверки **PASS** (164.02s весь адресный батч). Arena работает в двух native HWND с одной session/input clock; после закрытия surfaces возвращаются к0.

Первый запуск свежего image завершился readiness timeout первого JCEF Canvas. Повтор на том же EXE в чистом qualification profile без увеличения таймаутов прошёл F-II.4 и production Arena, но обнаружил лишнюю асинхронную инициализацию workspace при добавлении sample. Причина исправлена в production action; полный fresh-image smoke после исправления прошёл. Также исправлены неверный Java accessor project identity, локальное затенение переменной в тесте и устаревшее ожидание ровно11 операций после расширения native каталога. Новая проверка требует каждую из11 базовых операций ровно один раз и отдельно gameplay registry; прежние проверки не удалены.

Оконный тест после полного закрытия всех своих JFrame позволял AWT заменить EDT перед следующим Arena-блоком. Он теперь сохраняет скрытый host peer на всё время квалификации, как основное окно настоящего приложения. Native thread-affinity guard сохранён; все игровые окна по-прежнему реально создаются и освобождаются.

Измерения последнего packaged Release прогона на этой машине: native tick p50/p95 при10 Entity —53.1/71.0µs,100 —672.7/1189.8µs,1000 —38.57/45.72ms. Для1000 entities это **не60Hz budget**; фиксируется как предел текущего диагностического профиля. Два Game HWND: UI refresh p50/p95=1.07/1.57ms; фактическая presentation≈32.4/63.0FPS при запросе60/120, не обещание гарантированной частоты.1000 Entity refresh/draw p50/p95=63.62/73.37ms.200 открытий/закрытий: surfaces=0, steady handlesΔ−3, private bytesΔ≈−10.6MiB (порог плато64MiB). Физические DPI/мониторы агентом не проверялись.

Квалифицированное приложение: `Temp/U/verification-images/419e8cc7300843cc804647a91fd5296b/EpidemicEditor/EpidemicEditor.exe`. Старый `Temp/U/distribution` не следует считать обновлённым этим side-by-side прогоном.

## Приёмка

[Совместный пошаговый прогон](FII45_MANUAL_QA.md): полный F-II.4, десять ранних регрессий и16 Arena/C# сценариев F-II.5. Закройте старый экземпляр и запускайте новый EXE из итоговой ссылки. Проверяйте максимизированное окно, реальный фокус, DPI/мониторы и обе независимые панели.

Профиль ограничен 2D overlap/trigger без Rigidbody/CCD, owner-scoped unary i64 API с явными единицами, session-only прогрессом и существующими bounded managed scopes. Editor Save не является savegame. F-II.4-GATE и F-II.5-GATE **PENDING MANUAL**; F-III не начат.
