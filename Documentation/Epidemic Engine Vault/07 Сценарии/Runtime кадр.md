---
aliases: []
type: "guide"
source_snapshot: "2026-10-10"
---

# Последовательность Runtime кадра

Стандартный coordinator, описанный в `Documentation/EngineRuntime/modules/support.md`, исполняет следующий порядок:

```mermaid
flowchart TD
  T[Time] --> C[MainThreadCommits] --> R[Resources] --> St[Streaming] --> S[Simulation] --> N[Navigation] --> A[Animation] --> P[Physics] --> Pr[SceneProjectionCommit] --> Au[Audio] --> Re[Renderer] --> D[DiagnosticsEvents]
```

Кадр получает `RuntimeFrameInput` с `real_delta`, бюджетами загрузки, стриминга, навигации и анимации. Game delta берётся из `Time`, и не вычисляется произвольно в каждом major. `Tick()` возвращает `RuntimeTickResult`; `executed_steps` содержит реально принятые фазы, `failures` локальные recoverable failures.

При позднем прерывании Tick coordinator сохраняет pending-frame state. Повтор с тем же input может продолжить незавершённый suffix, другой input до reconciliation отклоняется. Не вызывайте Tick с новым временем, если предыдущий кадр требует retry.

Результат domain sim commits ограничивается frame budget. Обновление объектов на главном потоке выполняется на designated phase; не модифицируйте authoritative state из произвольного worker callback.

[[05 EngineRuntime/Модуль Support]] · [[11 API/EngineRuntime/Support/runtime_support.h]].
