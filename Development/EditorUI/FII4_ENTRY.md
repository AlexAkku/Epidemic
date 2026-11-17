# F-II.4 — реализация и приёмка

F-II.3-GATE PASS подтверждён владельцем 2026-11-16 и зафиксирован в `688be74` (предыдущая реализация `33e3beb`). Первый проверенный native Runtime батч F-II.4 — `695fc4e`.

Цель F-II.4: один C++ Runtime, непрерывный Game View, native input, Pause/Step/Restart/Stop, read-only Runtime Inspector, Graph Debugger, Console/Problems и bounded diagnostics. Сквозной сценарий: authoring Health100 → runtime Damage7 → native Wait → Damage7 → Stop → authoring100.

Код завершён; функциональная автоматическая квалификация прошла в двух логических батчах: промежуточном и финальном. Исправления выявленных ошибок и повторы затронутых проверок входят в соответствующий прогон; Computer Use не используется. Final automatic PASS не равен ручному F-II.4-GATE PASS.

[Контракты](FII4_RUNTIME_CONTRACT.md) · [Ручная проверка с 10 ранними регрессиями](FII4_MANUAL_QA.md) · [Результат и измерения](FII4_REPORT.md).

F-II.5 не начат. Для перехода требуется ручное подтверждение кадров/фокуса/нескольких окон и принятие измеренных performance budgets F-II.4.
