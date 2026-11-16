# Вход в F-II.4

Статус на 2026-11-16: **NOT STARTED**. Источник требований — `Temp/!Tasks/F-II_Implementation_Spec.md`, раздел 7 (F8). В этом батче изменялась только текущая оболочка F-II.1–3; runtime-функциональность F-II.4 не добавлялась.

## Перед входом

- F-II.0/0b и F-II.1 приняты; F-II.2-GATE PASS в заявленном offline scalar/unary профиле, Rider — принятое исключение.
- Ручные пункты 1–21 текущего прогона приняты владельцем. Пункты 17, 19 и 20 включать в последующие регрессии.
- UI-замечания устранены, адресная автоматическая проверка прошла. Для окончательного F-II.3-GATE остаются короткий повтор UI и результат пункта 22 (Save/полное закрытие/новый процесс/Play-Stop/завершение).
- Предыдущие предупреждения анализаторов сохраняются в backlog; проверки не ослаблены.

## Порядок следующей реализации

1. Сверить реальные API владельцев Play, Scheduler, Renderer/Input и Auditor. Зафиксировать одну native state machine Stopped/Starting/Running/Paused/Stopping/Failed, существующий fixed timestep и policy Stop/Restart. Не вводить второго владельца симуляции.
2. Подключить непрерывный Game View к существующему Renderer и одной Play-сессии, затем Pause/StepTick/Resume/Restart/Stop и маршрутизацию input по фокусу. Scene остаётся authoring.
3. Добавить ограниченный immutable RuntimeSnapshot с session/generation/tick; read-only Runtime Inspector и безопасный teardown HWND/GPU leases в нескольких окнах.
4. Подключить Graph execution overlay/trace, activation/Wait/ошибки, breakpoints/Continue и доказуемую гранулярность StepGraph отдельно от StepTick. Не сохранять execution state в GraphDefinition и не блокировать EDT.
5. Подключить Console/Problems и реальные метрики Auditor с единицами, интервалами и Unavailable. Зафиксировать измеренный baseline и budgets перед gate.
6. Выполнить регрессии раздела 7.8 и ручную проверку кадров, фокуса и общей session_id в независимых окнах. До этого не объявлять F-II.4 PASS.

Настоящий документ определяет порядок будущего батча и не подтверждает наличие перечисленных возможностей.

## Разрешение на начало

2026-11-16: владелец подтвердил UI и пункт22, **F-II.3-GATE PASS**. F-II.4 IN PROGRESS от `33e3beb`. Основной сквозной сценарий — runtime Health/Damage/Wait/Damage, Pause/Step/active node, Stop без изменения authoring. Два проверочных прогона: примерно посередине и финальный после проверки актуальности тестов. Java не владеет тиками, временем или игровым циклом.
