"""ETL/native decoder import into the existing immutable evidence lineage."""
from pathlib import Path
import json
import time
import hashlib

from ep_auditor.analyzers.windows_threads import thread_timeline, project_monotonic_time
from ep_auditor.collectors.xperf import iter_windows_events
from ep_auditor.core.evidence import EvidenceError, RawEvidence, digest
from ep_auditor.core.lineage import EvidenceLineage, normalized_record


def external_concurrency_observations(graph, observations, trace_origin_utc_ns, *, deadline_ns=500000000):
    """Bind verified external control/WCT records to trace lifetimes and clocks.

    Fixture declarations never establish ownership or correlation. WCT records
    establish only snapshot ownership, not continuous hold or acquisition stacks.
    Clock bounds are preserved; a control deadline uses the upper request bound.
    """
    if type(deadline_ns) is not int or deadline_ns <= 0:
        raise EvidenceError("external response deadline must be positive")
    actions, waits = [], []
    for observed in observations:
        if observed.get("provenance") != "external" or not observed.get("rawReference"):
            raise EvidenceError("control/WCT normalization requires external raw evidence")
        payload, reference = observed["payload"], observed["rawReference"]
        case = observed["case"]
        if case not in {mode + "_" + variant for mode in ("cancel", "completion", "mutex", "contention", "orphan", "starvation")
                        for variant in ("good", "bad")}:
            raise EvidenceError("unknown physical observation case")
        anchors = payload["clock_calibrations"]

        def project(value):
            return project_monotonic_time(value, anchors, trace_origin_utc_ns)

        def process(pid, creation_filetime=None):
            matches = [row for row in graph["processes"] if row["pid"] == pid]
            if creation_filetime is not None:
                created = (creation_filetime - 116444736000000000) * 100 - trace_origin_utc_ns
                matches = [row for row in matches if row["creation_ns"] is not None
                           and abs(row["creation_ns"] - created) <= 2000000]
            if len(matches) != 1:
                raise EvidenceError("external PID/creation does not identify one ETW lifetime")
            return matches[0]

        if case.startswith(("cancel_", "completion_")):
            if (payload.get("observation_kind") != "external_control_request" or payload.get("api") != "SetEvent"
                    or payload.get("api_succeeded") is not True):
                raise EvidenceError("control observation lacks successful external SetEvent")
            target = process(payload["pid"], payload["creation_filetime_100ns"])
            name = payload.get("object_name")
            if not isinstance(name, str) or not name or name not in (target.get("command_line") or "").split():
                raise EvidenceError("external control object not bound to observed target command")
            begin, end, observed_end = (project(payload[name]) for name in
                                       ("request_begin_ns", "request_end_ns", "observed_end_ns"))
            if (payload["request_begin_ns"] > payload["request_end_ns"] or target["end_ns"] is None
                    or target["creation_ns"] > begin["lower_ns"]
                    or target["end_ns"] < end["upper_ns"]
                    or target["end_ns"] > observed_end["upper_ns"]):
                raise EvidenceError("external control outside corroborated process lifetime")
            actions.append({"processId": target["processId"], "observation_kind": "external_control_request",
                "kind": "cancel" if case.startswith("cancel_") else "complete", "request_ns": end["upper_ns"],
                "deadline_ns": end["upper_ns"] + deadline_ns, "request_bounds": end,
                "request_begin_bounds": begin, "observed_end_bounds": observed_end,
                "rawReferences": [reference], "correlationEvidence": target["rawReferences"],
                "case": case, "provenance": "external", "confidence": "derived"})
        for snapshot in payload.get("wct_snapshots", []):
            if snapshot.get("provenance") != "external" or snapshot["pid"] != payload["pid"]:
                raise EvidenceError("WCT snapshot identity/provenance mismatch")
            target = process(snapshot["pid"], snapshot["creation_filetime_100ns"])
            chain = snapshot["wait_chain"]
            if not chain.get("available"):
                continue
            frequency = chain["qpc_frequency"]
            if type(frequency) is not int or frequency <= 0 or any(anchor["qpc_frequency_hz"] != frequency for anchor in anchors):
                raise EvidenceError("WCT QPC frequency differs from calibration")
            offsets = [(anchor["perf_begin_ns"] - anchor["qpc_end_ticks"] * 1000000000 // frequency,
                        anchor["perf_end_ns"] - anchor["qpc_begin_ticks"] * 1000000000 // frequency) for anchor in anchors]
            qpc_begin, qpc_end = chain["query_begin"], chain["query_end"]
            if qpc_begin > qpc_end:
                raise EvidenceError("WCT query clock reversed")
            lower = project(qpc_begin * 1000000000 // frequency + min(pair[0] for pair in offsets))["lower_ns"]
            upper = project(qpc_end * 1000000000 // frequency + max(pair[1] for pair in offsets))["upper_ns"]
            if target["creation_ns"] > lower or (target["end_ns"] is not None and target["end_ns"] < upper):
                raise EvidenceError("WCT query outside observed process lifetime")

            def thread(node):
                owners = [row for row in graph["threads"] if row["os_tid"] == node["os_tid"]
                          and row["processId"] == target["processId"] and node["pid"] == target["pid"]
                          and row["first_observation_ns"] <= lower
                          and (row["end_ns"] is None or row["end_ns"] >= upper)]
                if len(owners) != 1:
                    raise EvidenceError("WCT thread does not identify one observed lifetime")
                return owners[0]["threadId"]

            nodes = chain["nodes"]
            for index in range(1, len(nodes) - 1):
                resource, waiter, owner = nodes[index], nodes[index - 1], nodes[index + 1]
                if (resource.get("kind") != "wait_resource" or not resource.get("name")
                        or waiter.get("kind") != "thread" or owner.get("kind") != "thread"):
                    continue
                resource_id = "ResourceId:" + hashlib.sha256((graph["identity_namespace"] + ":" + resource["name"]).encode()).hexdigest()
                waits.append({"available": True, "provenance": "external", "confidence": "derived",
                    "resourceId": resource_id, "resource_name": resource["name"], "ownerThreadId": thread(owner),
                    "waiterThreadIds": [thread(waiter)], "cycle": chain.get("cycle"), "timestamp_ns": lower,
                    "timestamp_bounds": {"lower_ns": lower, "upper_ns": upper}, "rawReferences": [reference],
                    "continuous_hold_ns": None, "acquisition_stack": None, "case": case})
    return actions, sorted(waits, key=lambda row: row["timestamp_ns"])


def import_capture(run_dir: Path, files: dict, context: dict, *, pids: set[int], encoding: str,
                   scope: dict, event_capacity: int = 500000):
    """files bind original ETL, decoder CSV and capture/decoder metadata to SHA-256.

    Byte completeness is distinct from scope: a window/PID selection never
    establishes full system coverage. Unknown raw records remain in the CSV/ETL.
    """
    if set(files) != {"trace.etl", "events.csv", "metadata.json"}:
        raise EvidenceError("ETW import requires original trace, decoder output and metadata")
    if type(event_capacity) is not int or not 0 < event_capacity <= 2000000:
        raise EvidenceError("invalid bounded normalized event capacity")
    if not scope.get("selection") or not scope.get("clock_domain") or scope.get("full_system_coverage") is not False:
        raise EvidenceError("ETW selection/clock scope must be explicit")
    raw = RawEvidence(run_dir)
    metadata = json.loads(Path(files["metadata.json"]["path"]).read_text(encoding="utf-8"))
    if (metadata.get("trace_sha256") != files["trace.etl"]["sha256"]
            or metadata.get("csv_sha256") != files["events.csv"]["sha256"]
            or metadata.get("scope") != scope or not metadata.get("decoder_command")):
        raise EvidenceError("ETW metadata disagrees with trace/decoder/selection inputs")
    loss = metadata.get("trace_loss", {})
    counters = [loss.get("lost_events"), loss.get("lost_buffers")]
    if any(value is not None and (type(value) is not int or value < 0) for value in counters):
        raise EvidenceError("invalid trace loss counters")
    loss_reason = None if counters == [0, 0] else "etw_event_loss_or_unknown"
    start = time.perf_counter_ns()
    for name, item in files.items():
        path = Path(item["path"])
        size = path.stat().st_size
        raw.import_file("etw/" + name, path, item["sha256"], collector="external_etw_import", version="1",
                        start_ns=start, end_ns=time.perf_counter_ns(), health={"capacity": size,
                            "observed": size, "dropped": 0, "incomplete_reason": loss_reason if name != "metadata.json" else None})
    raw.seal(context)
    return _publish_events(run_dir, pids=pids, encoding=encoding, scope=scope,
        event_capacity=event_capacity, csv_name='etw/events.csv', metadata_name='etw/metadata.json',
        trace_name='etw/trace.etl', complete=counters == [0, 0])


def _publish_events(run_dir, *, pids, encoding, scope, event_capacity, csv_name, metadata_name,
                    trace_name, complete, verify_only=False):
    raw = RawEvidence(run_dir)
    context = raw.verify()
    lineage = EvidenceLineage(run_dir)
    csv_ref = lineage.reference("raw", csv_name)
    metadata_ref = lineage.reference("raw", metadata_name)
    trace_ref = lineage.reference("raw", trace_name)
    selection = {'pids': sorted(pids), 'encoding': encoding}
    def publish(layer, path, **fields):
        destination = run_dir / layer / path
        if destination.is_file():
            envelope = lineage.verify(layer, path)
            if envelope['records'] != fields['records'] or envelope['inputs'] != fields['inputs']:
                raise EvidenceError('ETW projection differs from sealed decoder bytes: ' + path)
        elif verify_only:
            raise EvidenceError('required ETW projection absent: ' + path)
        else:
            lineage.write(layer, path, **fields)
    records, events = [], []
    for event in iter_windows_events(raw.raw / csv_name, pids, encoding=encoding):
        if len(events) >= event_capacity:
            raise EvidenceError("ETW normalization exceeded event capacity; no derived PASS artifact")
        events.append(event)
        records.append(normalized_record({"schemaVersion": 1, "eventType": "windows.etw.observation",
            "required": True, "payload": event, "observationScope": scope, 'decoderSelection': selection,
            "provenance": "external", "confidence": "observed"}, {"windows.etw.observation"}, csv_ref, event["rawOrdinal"]))
    if not records:
        raise EvidenceError("selected ETW observations absent")
    publish("normalized", "windows_events.json", producer_version="1", producer_source=Path(__file__),
                  inputs=[csv_ref, metadata_ref, trace_ref], records=records)
    normalized_ref = lineage.reference("normalized", "windows_events.json")
    graph = thread_timeline(events, identity_namespace=trace_ref["sha256"] + ":" + context["run_id"])
    for process in graph["processes"]:
        process["rawReferences"] = [{**csv_ref, "rawOrdinal": ordinal} for ordinal in process["rawOrdinals"]]
    graph["rawReference"] = csv_ref
    graph["observationScope"] = scope
    graph["normalizationProducerSha256"] = lineage.verify("normalized", "windows_events.json")["producerSourceSha256"]
    graph['decoderSelection'] = selection
    publish("graphs", "windows_scheduler_sampling.json", producer_version="1",
                  producer_source=Path(__file__).parents[1] / "analyzers/windows_threads.py",
                  inputs=[normalized_ref], records=[graph])
    if any(event['kind'].startswith('Disk') for event in events):
        from ep_auditor.analyzers.io import disk_requests
        disk = disk_requests(events, complete=complete, main_thread_id=scope.get('main_thread_id'))
        disk['rawReference'] = csv_ref
        disk['observationScope'] = scope
        publish('metrics', 'windows_disk_io.json', producer_version='ms65-disk-1',
            producer_source=Path(__file__).parents[1] / 'analyzers/io.py', inputs=[normalized_ref],
            records=[{'eventType': 'windows.disk.requests', 'required': True, **disk}])
    return graph


def publish_wpr_capture(run_dir: Path, *, verify_only=False):
    """Reuse the ETW importer for a run's real broker/xperf capture."""
    raw = RawEvidence(run_dir)
    manifest = raw.verify()
    if not any(entry['path'] == 'raw/wpr.json' for entry in manifest['raw_files']):
        return
    metadata = json.loads(raw.read_bytes('wpr.json'))
    decoder = metadata['xperf']
    scope = metadata.get('scope')
    if (decoder.get('errors') or not scope or scope.get('full_system_coverage') is not False
            or not decoder.get('decoder_commands', {}).get('events') or not decoder.get('decoder_sha256')):
        raise EvidenceError('WPR capture has no verified bounded event decoder')
    lifecycle = json.loads(raw.read_bytes('process/lifecycle.json'))
    if scope.get('pids') != [lifecycle['pid']]:
        raise EvidenceError('WPR selected process differs from owned lifecycle')
    loss = metadata.get('trace_loss')
    if not isinstance(loss, dict):
        raise EvidenceError('WPR event loss is unknown')
    from ep_auditor.collectors.xperf import parse_trace_loss
    observed_loss = parse_trace_loss(raw.read_bytes('xperf/trace_statistics.txt').decode('mbcs', errors='strict'))
    if loss != observed_loss:
        raise EvidenceError('WPR loss metadata differs from sealed SDK statistics')
    return _publish_events(run_dir, pids=set(scope['pids']), encoding=decoder['event_encoding'], scope=scope,
        event_capacity=500000, csv_name='xperf/events.csv', metadata_name='wpr.json', trace_name='trace.etl',
        complete=loss == {'lost_events': 0, 'lost_buffers': 0}, verify_only=verify_only)


def verify_etw_capture(run_dir: Path):
    """Replay existing projections from the immutable decoder input and selection."""
    raw = RawEvidence(run_dir)
    manifest = raw.verify()
    names = {entry['path'] for entry in manifest['raw_files']}
    if 'raw/wpr.json' in names:
        return publish_wpr_capture(run_dir, verify_only=True)
    if 'raw/etw/events.csv' not in names:
        return None
    metadata = json.loads(raw.read_bytes('etw/metadata.json'))
    envelope = EvidenceLineage(run_dir).verify('normalized', 'windows_events.json')
    selection = envelope['records'][0].get('decoderSelection')
    if not selection or metadata.get('scope') != envelope['records'][0].get('observationScope'):
        raise EvidenceError('ETW decoder selection absent or inconsistent')
    if (metadata.get('trace_sha256') != digest(raw.raw / 'etw/trace.etl')
            or metadata.get('csv_sha256') != digest(raw.raw / 'etw/events.csv')):
        raise EvidenceError('ETW decoder input binding differs from sealed inputs')
    return _publish_events(run_dir, pids=set(selection['pids']), encoding=selection['encoding'],
        scope=metadata['scope'], event_capacity=500000, csv_name='etw/events.csv',
        metadata_name='etw/metadata.json', trace_name='etw/trace.etl',
        complete=metadata.get('trace_loss') == {'lost_events': 0, 'lost_buffers': 0}, verify_only=True)
