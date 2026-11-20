# Матрица готовности F-II

| Функция | Состояние | Основание |
|---|---|---|
| F-II.0a, сверка и baseline | Implemented · PASS | Свежий SHA совпал с GitHub, Release, адресные CTest и managed baseline прошли. [Baseline](FII_BASELINE.md). |
| Graph authoring Core / VM | Implemented | GraphDefinition, compiler, VM, Add/DeleteGraphNode, Connect/DisconnectPorts, native history и сериализация. |
| Graph math | Implemented · manual pending | Native i64/f64 overloads, explicit conversions, Subtract/Divide/Integer Divide/Round; inline/history/new-process roundtrip PASS. [Контракт](FII_NUMERIC_GRAPH.md). Native/managed call ABI остаётся i64→i64. |
| Graph Editor и browser | Implemented F-II.1 · GATE PASS | Native catalogue, Rete projection/Selection, Inspector/defaults/metadata, atomic fragments/bulk edits, compile/native VM/scheduler, diagnostics, offline sandbox EXE. [Отчёт и границы](FII1_GRAPH_AUTHORING.md). |
| Code Space | Implemented · offline profile | Platform Editor/Document/VFS, highlighting/search/line, dirty/Save/close/conflicts, production SDK build and diagnostics. LSP не поставляется (§5.3 fallback). [F-II.2](FII2_SCHEMA_CODESPACE.md). |
| Rider / Open With | Implemented · принято с исключением | VS Code проверен владельцем; Rider принят им без установленного Rider (2026-11-16). Реальный запуск Rider не заявляется. |
| Managed bridge | Implemented / Limited | Opt-in C ABI и Graph adapter проходят контракт. Native hostfxr проверен на двух сборках Turret. F-II.2 добавляет native descriptors и один worker на поколение Play, native field roundtrip, Stop/old-handle checks. Scalar/unary профиль; [границы](FII2_SCHEMA_CODESPACE.md). |
| Native Schema, Play/Stop, Scene Renderer | Implemented | Runtime/authoring доступны по существующим контрактам, Scene использует native viewport. |
| Graph↔Scene | Implemented · GATE PASS | F-II.3 принят владельцем после исправлений UI и подтверждения пункта22. |
| Live Game View / native Play | Implemented, manual gate pending | F-II.4: один C++ Runtime, input/Pause/Step/Restart/Stop, настоящие HWND/Renderer. |
| Runtime Inspector / Graph Debugger | Implemented, bounded profile | Реальные поля/ports/trace, native breakpoints/Step to Node, readonly overlay. |
| Console / Problems / Profiler | Implemented / Limited providers | История/фильтры/source links; реальные Diagnostics/process/renderer counters, отсутствующие providers → Unavailable. |
| Gameplay authoring | Implemented · manual pending | F-II.5: единый native owner, Schema/Graph/C#, движение, collision/projectile/damage/waves, Arena sample. [Контракт](FII5_GAMEPLAY_CONTRACT.md), [отчёт](FII5_REPORT.md). |
| F-II.0b / вход в F-II.1 | PASS / READY | Настоящий CEF sandbox и native roundtrip квалифицированы. F-II.1 реализован; автоматические функциональные проверки PASS, ручной gate PASS (владелец, 2026-11-14). |

Статус и решения по обязательным пробам приведены в [техническом отчёте](FII_TECH_ADR.md).

Контракты F-II.2: [Code Space / managed](FII_CODESPACE_MANAGED_CONTRACT.md). F-II.1-GATE PASS. F-II.2 реализован от `451ba62`; gate ожидает адресную ручную приёмку.


F-II.1-GATE **PASS**: владелец подтвердил ручную приёмку 2026-11-14 («Проверил, всё работает»). Исходная реализация `451ba62`; ранее зафиксированные ограничения производительности и исторические static diagnostics сохраняются. F-II.2 начинается от этого коммита.

## F-II.3 — текущий implementation batch

Native GraphReference payload v1, typed compiler/IR/VM resolver, atomic command 40 и текущий Graph DTO v5 подключены к одному project-scoped Windows OLE DnD coordinator. DTO v5 добавляет optional i64 input literal; IPC envelope остаётся v4. [Контракт и фактические проверки](FII3_GRAPH_SCENE.md), [ручная квалификация](FII3_MANUAL_QA.md). F-II.3-GATE ожидает физической межоконной DnD/DPI проверки владельцем; NOT RUN не переименован в PASS.

Исправления по ручному прогону 2026-11-16: [отчёт](FII_REVIEW_FIXES.md), [адресная проверка](FII_REVIEW_MANUAL_QA.md). Владелец подтвердил пункты 1–21, включая schema/defaults/Remove и новые Graph interactions. F-II.2-GATE PASS в заявленном профиле. Для F-II.3 остаются короткий повтор исправлений компоновки и результат пункта 22. F-II.4 NOT STARTED; [подготовлен порядок входа](FII4_ENTRY.md).

Актуальное подтверждение владельца 2026-11-16: **F-II.3-GATE PASS**, включая новые UI-исправления и пункт22. **F-II.4 IN PROGRESS**, основной сценарий Health → Damage → Wait(ticks) → Damage, C++ Runtime владеет временем и исполнением.


Актуальный статус: F-II.4 и F-II.5 code complete, оба GATE PENDING MANUAL. F-II.5: full native CTest162/162 и standalone Release/JCEF smoke PASS. Deep сохраняет inherited FAIL отдельно от функционального результата; packaged результаты и бюджеты в [FII5_REPORT](FII5_REPORT.md). [Совместный ручной прогон с10 ранними регрессиями](FII45_MANUAL_QA.md). F-III не начат.

Повтор20 ноября2026: единые Data/Math элементы палетки, native Auto/i64/f64 lock, атомарное согласование типов с Undo/Redo и graph_authoring v2; исправлены Wait/debug boundaries и независимое закрытие Problems. Арифметика переиспользует C++ VM, время остаётся в C++ Runtime. Принятые владельцем пункты и оставшийся manual scope — [FII_ACCEPTANCE](FII_ACCEPTANCE.md), результаты текущего запуска — [FII45_REVIEW_FIXES](FII45_REVIEW_FIXES.md). F-II.4/F-II.5-GATE PENDING MANUAL; F-II.5 manual NOT STARTED.
