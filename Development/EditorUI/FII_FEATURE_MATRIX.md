# Матрица готовности F-II

**F-II-GATE PASS · FROZEN, 21 ноября 2026.** Реализация `55ab4bf` полностью принята владельцем. Следующая стадия не начата.


| Функция | Состояние | Основание |
|---|---|---|
| F-II.0a, сверка и baseline | Implemented · PASS | Свежий SHA совпал с GitHub, Release, адресные CTest и managed baseline прошли. [Baseline](FII_BASELINE.md). |
| Graph authoring Core / VM | Implemented | GraphDefinition, compiler, VM, Add/DeleteGraphNode, Connect/DisconnectPorts, native history и сериализация. |
| Graph math | Implemented · GATE PASS | Native i64/f64 overloads, explicit conversions, Subtract/Divide/Integer Divide/Round; inline/history/new-process roundtrip PASS. [Контракт](FII_NUMERIC_GRAPH.md). Native/managed call ABI остаётся i64→i64. |
| Graph Editor и browser | Implemented F-II.1 · GATE PASS | Native catalogue, Rete projection/Selection, Inspector/defaults/metadata, atomic fragments/bulk edits, compile/native VM/scheduler, diagnostics, offline sandbox EXE. [Отчёт и границы](FII1_GRAPH_AUTHORING.md). |
| Code Space | Implemented · offline profile | Platform Editor/Document/VFS, highlighting/search/line, dirty/Save/close/conflicts, production SDK build and diagnostics. LSP не поставляется (§5.3 fallback). [F-II.2](FII2_SCHEMA_CODESPACE.md). |
| Rider / Open With | Implemented · принято с исключением | VS Code проверен владельцем; Rider принят им без установленного Rider (2026-11-16). Реальный запуск Rider не заявляется. |
| Managed bridge | Implemented / Limited | Opt-in C ABI и Graph adapter проходят контракт. Native hostfxr проверен на двух сборках Turret. F-II.2 добавляет native descriptors и один worker на поколение Play, native field roundtrip, Stop/old-handle checks. Scalar/unary профиль; [границы](FII2_SCHEMA_CODESPACE.md). |
| Native Schema, Play/Stop, Scene Renderer | Implemented | Runtime/authoring доступны по существующим контрактам, Scene использует native viewport. |
| Graph↔Scene | Implemented · GATE PASS | F-II.3 принят владельцем после исправлений UI и подтверждения пункта22. |
| Live Game View / native Play | Implemented · GATE PASS | F-II.4: один C++ Runtime, input/Pause/Step/Restart/Stop, настоящие HWND/Renderer. |
| Runtime Inspector / Graph Debugger | Implemented, bounded profile | Реальные поля/ports/trace, native breakpoints/Step to Node, readonly overlay. |
| Console / Problems / Profiler | Implemented / Limited providers | История/фильтры/source links; реальные Diagnostics/process/renderer counters, отсутствующие providers → Unavailable. |
| Gameplay authoring | Implemented · GATE PASS | F-II.5: единый native owner, Schema/Graph/C#, движение, collision/projectile/damage/waves, Arena sample. [Контракт](FII5_GAMEPLAY_CONTRACT.md), [отчёт](FII5_REPORT.md). |
| F-II.0b / вход в F-II.1 | PASS / FROZEN | Настоящий CEF sandbox и native roundtrip квалифицированы. F-II.1 реализован; автоматические функциональные проверки PASS, ручной gate PASS (владелец, 2026-11-14). |


Дополнительно приняты: Scene copy/cut/paste/delete через native commands; Explorer clipboard, Shift/Ctrl selection, inline Tree rename, внутренний DnD и создание по выбранному пути; компактный Scene toolbar. C++ остаётся единственным игровым clock/execution owner.

Границы не расширены формальным PASS: Code Space использует принятый offline fallback; managed API scalar/unary; unavailable profiler providers показывают Unavailable; Rider принят с исключением без фактического запуска. Native ready activations получают отдельный bounded VM quantum. Исторический static FAIL остаётся backlog.

[Acceptance](FII_ACCEPTANCE.md) · [QA](FII_QA_MATRIX.md) · [Итоговый отчёт](FII_FREEZE.md) · [ADR](FII_TECH_ADR.md) · [Регрессии](FII45_MANUAL_QA.md).
