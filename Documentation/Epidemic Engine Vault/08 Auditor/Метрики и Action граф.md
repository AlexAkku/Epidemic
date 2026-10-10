---
aliases: []
type: "guide"
source_snapshot: "2026-10-10"
---

# События, Action и причинные графы

Auditor различает function ENTER/EXIT (compiler/native trace), семантические probe handoffs и внешние process/ETW facts. Action фиксирует bounded correlation chain вокруг заданного события (например, нажатие input в Audit profile); replay evidence можно открыть через Actions/Event Log или CLI.

```mermaid
flowchart LR
 Input[Input event] --> Action[ActionId and event scope]
 Action --> Job[Task or dispatcher handoff]
 Job --> Call[FunctionId/CallId]
 Call --> Effects[Owned state / IO / render probe]
 Effects --> Report[Bounded event log + DAG + profile]
```

Это модель корреляции при наличии probes; не каждая стрелка гарантированно наблюдаема, а callback entered не равен успешному commit или GPU completion. Поддерживается rolling capture с ограниченными pre/post окнами. Потерянные records, abort/crash и неполные стековые события помечаются неполными.

Отчёты подготавливаются из проверенных sealed Run; назначение события input через `capture.arm_next_input` и rolling capture через `capture.rolling` описаны в [[90 Первоисточники/.epidemic-auditor/README|актуальном README Auditor]].
