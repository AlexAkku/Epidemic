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

---

## Практическая интерпретация provenance

Разделяйте source происхождение доказательств и scope утверждения. `external_process` отражает OS process sampling, `external_ctest` отражает CTest outcome, `external_wpr_raw` является сырым ETW артефактом; `engine_probe` означает собственное наблюдение audit-instrumented build. Последнее полезно для семантического causal chain, но не является независимым внешним доказательством, что сама инструментированная функция корректна.

### Пример проверки от Action до raw bytes

```powershell
# Выполняется в подготовленном python environment Auditor.
python -B -m ep_auditor.cli event-log <sealed-run> --offset 0 --limit 50
python -B -m ep_auditor.cli action <sealed-run> <ActionId>
```

Команды только читают уже запечатанный run. Они не создают новые telemetry и не могут восстановить незафиксированные вызовы. Если `ENTER` без соответствующего `EXIT`, длительность функции нельзя вычислять как полноценную. Если trace имел перезапись кольца или обрезку pre/post окна, `COMPLETE` недоказуем. Несовпадение recorded symbol map и binary identity запрещает доверять resolved function names.

`PASS` ограничен конкретной проверкой; `COMPLETE` означает полноту захвата для указанной операции, а не отсутствие memory errors. `UNKNOWN` означает отсутствие достаточных сведений, а `INCOMPLETE` указывает на конкретный недостаток наблюдения. В отчёте оставляйте оба поля отдельно. Нельзя считать нулём CPU time или количество failures, которые вообще не измерялись.
