---
aliases: []
type: "guide"
source_snapshot: "2026-10-10"
---

# Архитектурный freeze и запретные зависимости

Проверка включена по умолчанию флагом `EPIDEMIC_ARCHITECTURE_FREEZE_CHECKS=ON` и определяется скриптами `cmake/ArchitectureFreeze.cmake`, `cmake/ArchitectureFreezeSelfTest.cmake`. При конфигурации контролируются зависимости targets, production includes и public headers. Отдельная цель `EpidemicPublicHeaderSelfContainment` собирает публичные заголовки в изоляции.

Главные контракты:

1. `EngineBase` не зависит от Runtime и Framework; Runtime не зависит от Framework.
2. Runtime majors не формируют горизонтальный include/link граф; техническая интеграция в `EngineRuntime/Support`.
3. Framework gameplay owners используют allowlisted foundation/ports, а горизонтальная gameplay-композиция принадлежит `IntegrationLayer`.
4. `RuntimeBridge` является разрешённой Framework↔Runtime границей с централизованным набором World/Physics/Environment/Navigation контрактов.
5. Прямой runtime include IntegrationLayer ограничен существующим исключением `CoreIntegration → RuntimeTime` и `core_adapters.h`; расширять исключение без решения по архитектуре нельзя.
6. `PUBLIC`/`INTERFACE` зависимости обязаны отражать реально включаемые public headers; наличие транзитивного include пути недостаточно.

После изменения правил проверяйте оба архитектурных файла CMake, комментарии об исключениях, self-test с отрицательными фикстурами и матрицу сборок. В точности разрешённые targets сверяйте по CMake, а не по упрощённой схеме.

```powershell
cmake -P cmake/ArchitectureFreezeSelfTest.cmake
cmake --build build/full-debug --config Debug --target EpidemicPublicHeaderSelfContainment --parallel
```

[[09 Качество и статус/Матрица проверок]] · [[90 Первоисточники/Documentation/architecture|Исходная архитектура]].

---

## Проверяемый порядок изменения зависимости

Проверьте исходную связь `#include`, CMake target_link_libraries, ownership порта и правила списка запрещённых связей. Если связь нарушает freeze, перенесите её в разрешённый adapter/port. После изменения проверьте `cmake -P cmake/ArchitectureFreezeSelfTest.cmake`, header self-containment и dependent tests. Добавлять исключение только ради прохода сборки недопустимо.
