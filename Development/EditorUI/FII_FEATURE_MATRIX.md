# Матрица готовности F-II

| Функция | Состояние | Основание |
|---|---|---|
| F-II.0a, сверка и baseline | Implemented · PASS | Свежий SHA совпал с GitHub, Release, адресные CTest и managed baseline прошли. [Baseline](FII_BASELINE.md). |
| Graph authoring Core / VM | Implemented | GraphDefinition, compiler, VM, Add/DeleteGraphNode, Connect/DisconnectPorts, native history и сериализация. |
| Graph Editor и browser | Implemented F-II.1 · GATE PASS | Native catalogue, Rete projection/Selection, Inspector/defaults/metadata, atomic fragments/bulk edits, compile/native VM/scheduler, diagnostics, offline sandbox EXE. [Отчёт и границы](FII1_GRAPH_AUTHORING.md). |
| Code Space | Implemented · offline profile | Platform Editor/Document/VFS, highlighting/search/line, dirty/Save/close/conflicts, production SDK build and diagnostics. LSP не поставляется (§5.3 fallback). [F-II.2](FII2_SCHEMA_CODESPACE.md). |
| Rider / Open With | Implemented · принято с исключением | VS Code проверен владельцем; Rider принят им без установленного Rider (2026-11-16). Реальный запуск Rider не заявляется. |
| Managed bridge | Implemented / Limited | Opt-in C ABI и Graph adapter проходят контракт. Native hostfxr проверен на двух сборках Turret. F-II.2 добавляет native descriptors и один worker на поколение Play, native field roundtrip, Stop/old-handle checks. Scalar/unary профиль; [границы](FII2_SCHEMA_CODESPACE.md). |
| Native Schema, Play/Stop, Scene Renderer | Implemented | Runtime/authoring доступны по существующим контрактам, Scene использует native viewport. |
| Graph↔Scene | Implemented · manual pending | F-II.3 native/coordinator/VM regressions; физический DnD/DPI ещё не принят. |
| Live Game View, Gameplay authoring | Planned | F-II.4 и F-II.5 соответственно. |
| F-II.0b / вход в F-II.1 | PASS / READY | Настоящий CEF sandbox и native roundtrip квалифицированы. F-II.1 реализован; автоматические функциональные проверки PASS, ручной gate PASS (владелец, 2026-11-14). |

Статус и решения по обязательным пробам приведены в [техническом отчёте](FII_TECH_ADR.md).

Контракты F-II.2: [Code Space / managed](FII_CODESPACE_MANAGED_CONTRACT.md). F-II.1-GATE PASS. F-II.2 реализован от `451ba62`; gate ожидает адресную ручную приёмку.


F-II.1-GATE **PASS**: владелец подтвердил ручную приёмку 2026-11-14 («Проверил, всё работает»). Исходная реализация `451ba62`; ранее зафиксированные ограничения производительности и исторические static diagnostics сохраняются. F-II.2 начинается от этого коммита.

## F-II.3 — текущий implementation batch

Native GraphReference payload v1, typed compiler/IR/VM resolver, atomic command 40 и текущий Graph DTO v5 подключены к одному project-scoped Windows OLE DnD coordinator. DTO v5 добавляет optional i64 input literal; IPC envelope остаётся v4. [Контракт и фактические проверки](FII3_GRAPH_SCENE.md), [ручная квалификация](FII3_MANUAL_QA.md). F-II.3-GATE ожидает физической межоконной DnD/DPI проверки владельцем; NOT RUN не переименован в PASS.

Исправления по ручному прогону 2026-11-16: [отчёт](FII_REVIEW_FIXES.md), [адресная проверка](FII_REVIEW_MANUAL_QA.md). F-II.2 schema/defaults/Remove и новые Graph interactions требуют повтора владельцем.
