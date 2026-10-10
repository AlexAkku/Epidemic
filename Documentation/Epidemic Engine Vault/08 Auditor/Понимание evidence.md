---
aliases: []
type: "guide"
source_snapshot: "2026-10-10"
---

# Как интерпретировать evidence и статусы

Наблюдение может быть проверено по raw files, run metadata, schema/provenance и identity. Не преобразуйте отсутствие данных в нулевую метрику или в успех.

| Значение | Что можно утверждать |
| --- | --- |
| `PASS` | Только доказанное по конкретному assertion и scope |
| `COMPLETE` | Захват достаточен для определённой аналитической операции, не verdict о безопасности |
| `INCOMPLETE` | Потеря событий, разрыв call tree или нехватка pre/post контекста |
| `UNKNOWN` | Недостаточно данных или observation не выполнялось |
| `BLOCKED` | Не выполнены предварительные условия/адаптер недоступен |
| `INCOMPATIBLE` | Нельзя корректно сравнить два Runs |
| `FAIL` | Прямой провал проверенного условия, без подмены метриками |

В `RawEvidence` должны сохраняться ограничения размера и provenance. `external_process` (OS), `external_console` (stdout), `external_ctest`, `external_static_scan`, `external_wpr_raw` описывают внешние источники; `engine_probe` описывает только явно внедрённые в Audit build семантические probes. Непарные `ENTER/EXIT` не допускают корректного восстановления полного call tree и duration.

Source paths, symbols и binary identities должны совпадать с анализируемым Run. Перекомпилированный binary не означает, что старые symbol maps автоматически применимы к новому trace.

[[08 Auditor/Профили захвата]] · [[09 Качество и статус/Состояние MS6]].
