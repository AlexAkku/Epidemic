---
aliases: []
type: "guide"
source_snapshot: "2026-10-10"
---

# Runtime: независимые majors и Support

`EngineRuntime` разделён на 16 engine majors плюс композиционный `Support`. `RuntimeFoundation` служит общей основой для majors. Прямые include/link между остальными majors запрещены (подробности задаёт architecture checker).

```mermaid
flowchart LR
 subgraph Majors[Runtime majors]
   Assets[Assets]
   Resources[Resources]
   Scene[Scene]
   World[World]
   Physics[Physics]
   Renderer[Renderer]
   Streaming[Streaming]
   Animation[Animation]
   Audio[Audio]
   Nav[Navigation]
   Env[Environment]
   Time[Time]
   Sim[Simulation]
   Persistence[Persistence]
 end
 Support[EngineRuntime Support: adapters & coordinator]
 Assets -.-> Support
 Resources -.-> Support
 Scene -.-> Support
 World -.-> Support
 Physics -.-> Support
 Renderer -.-> Support
 Streaming -.-> Support
 Animation -.-> Support
 Audio -.-> Support
 Nav -.-> Support
 Env -.-> Support
 Time -.-> Support
 Sim -.-> Support
 Persistence -.-> Support
```

Стандартные мосты, выраженные через ports/adapters: Scene→Renderer; Resources→Renderer; Scene↔Physics; Resources→Animation→Renderer; Resources→Audio; World+Resources+Persistence→Streaming; Environment→Navigation; Time→Simulation→commit target. Это **логические потоки**, а не разрешение на прямое связывание majors.

`PrepareEngineRuntime()` подготавливает bundle и adapters до регистрации в `Application`; `CommitPreparedRuntime()` публикует подготовленную композицию. `RegisterDefaultEngineRuntime()` предоставляет сокращённый путь. Runtime lifecycle координируется `IEngineRuntimeCoordinator`.

[[05 EngineRuntime/Модуль Support|Описание Support]] · [[07 Сценарии/Runtime кадр|Последовательность Tick]] · [[11 API/EngineRuntime/Support/runtime_support.h|Публичный контракт]].
