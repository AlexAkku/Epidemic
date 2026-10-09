import csv
import json
from pathlib import Path
import tempfile
import unittest
import uuid

from ep_auditor.collectors.etw_import import import_capture
from ep_auditor.core.evidence import EvidenceError, RawEvidence, digest
from ep_auditor.core.lineage import EvidenceLineage


class EtwImportTests(unittest.TestCase):
    def test_external_control_and_wct_require_correlated_lifetimes_and_bounded_clocks(self):
        from ep_auditor.collectors.etw_import import external_concurrency_observations
        ref = {"layer": "raw", "path": "control/fixture.json", "sha256": "a" * 64}
        origin = 100000000000
        creation = origin + 1000
        filetime = creation // 100 + 116444736000000000
        anchors = [{"perf_begin_ns": 1000, "perf_end_ns": 1100, "utc_unix_ns": origin + 1050,
                    "qpc_begin_ticks": 10, "qpc_end_ticks": 11, "qpc_frequency_hz": 10000000,
                    "provenance": "external", "confidence": "observed"},
                   {"perf_begin_ns": 10000, "perf_end_ns": 10100, "utc_unix_ns": origin + 10050,
                    "qpc_begin_ticks": 100, "qpc_end_ticks": 101, "qpc_frequency_hz": 10000000,
                    "provenance": "external", "confidence": "observed"}]
        graph = {"identity_namespace": "trace-A", "processes": [{"pid": 41, "processId": "p",
            "creation_ns": 1000, "end_ns": 9000, "command_line": "fixture cancel bad Local\\Control", "rawReferences": [ref]}], "threads": [
            {"os_tid": tid, "threadId": name, "processId": "p", "first_observation_ns": 1200,
             "end_ns": 8900} for tid, name in ((9, "waiter"), (10, "owner"))]}
        payload = {"pid": 41, "creation_filetime_100ns": filetime, "api": "SetEvent", "api_succeeded": True,
            "observation_kind": "external_control_request", "request_begin_ns": 2000, "request_end_ns": 2100,
            "observed_end_ns": 9500, "clock_calibrations": anchors, "fixture_declarations": [], "object_name": "Local\\Control"}
        observed = {"payload": payload, "rawReference": ref, "case": "cancel_bad", "provenance": "external"}
        actions, waits = external_concurrency_observations(graph, [observed], origin)
        self.assertEqual(actions[0]["processId"], "p")
        self.assertEqual(actions[0]["kind"], "cancel")
        self.assertEqual(actions[0]["request_ns"], actions[0]["request_bounds"]["upper_ns"])
        self.assertEqual(waits, [])
        chain = {"available": True, "cycle": False, "qpc_frequency": 10000000, "query_begin": 30,
            "query_end": 31, "nodes": [{"kind": "thread", "pid": 41, "os_tid": 9},
                {"kind": "wait_resource", "name": "\\Sessions\\1\\Mutex"},
                {"kind": "thread", "pid": 41, "os_tid": 10}]}
        wct = {"pid": 41, "creation_filetime_100ns": filetime, "provenance": "external", "wait_chain": chain}
        wait_payload = {"pid": 41, "wct_snapshots": [wct], "clock_calibrations": anchors}
        _, waits = external_concurrency_observations(graph, [{**observed, "payload": wait_payload,
            "case": "mutex_bad"}], origin)
        self.assertEqual(waits[0]["ownerThreadId"], "owner")
        self.assertEqual(waits[0]["waiterThreadIds"], ["waiter"])
        self.assertIsNone(waits[0]["continuous_hold_ns"])
        self.assertIsNone(waits[0]["acquisition_stack"])
        for corrupted in ({**observed, "provenance": "engine_declared"},
                          {**observed, "payload": {**payload, "object_name": "Local\\Unrelated"}},
                          {**observed, "payload": {**payload, "creation_filetime_100ns": filetime + 100000}},
                          {**observed, "payload": {**payload, "request_end_ns": 11000}},
                          {**observed, "case": "unrecognized"}):
            with self.subTest(corrupted=corrupted), self.assertRaises(EvidenceError):
                external_concurrency_observations(graph, [corrupted], origin)

    def fixture(self, directory, loss):
        # Explicit synthetic decoder fixture, never evidence of a real ETL decoder.
        trace, events, metadata = (directory / name for name in ("trace.etl", "events.csv", "metadata.json"))
        trace.write_bytes(b"synthetic test fixture only")
        with events.open("w", newline="", encoding="utf-8") as stream:
            csv.writer(stream).writerows([
                ["BeginHeader"], ["T-Start", "TimeStamp", "ThreadID", "Process Name ( PID)"],
                ["SampledProfile", "TimeStamp", "ThreadID", "Process Name ( PID)", "PrgrmCtr", "Count"],
                ["T-End", "TimeStamp", "ThreadID", "Process Name ( PID)"], ["EndHeader"],
                ["T-Start", "1", "9", "fixture (41)"], ["SampledProfile", "2", "9", "fixture (41)", "0x123", "1"],
                ["T-End", "3", "9", "fixture (41)"]])
        scope = {"selection": "synthetic PID41 window", "clock_domain": "etl_relative_trace_ns", "full_system_coverage": False}
        metadata.write_text(json.dumps({"trace_sha256": digest(trace), "csv_sha256": digest(events),
            "decoder_command": ["synthetic fixture"], "trace_loss": {"lost_events": loss, "lost_buffers": 0}, "scope": scope}))
        files = {path.name: {"path": path, "sha256": digest(path)} for path in (trace, events, metadata)}
        context = {"run_id": "RunId:" + str(uuid.uuid4()), "instrumentation_profile": "audit-deep",
                   "source_fingerprints": {}, "binary_fingerprints": {}, "analyzer_version": "synthetic-test"}
        return files, scope, context

    def test_raw_normalized_graph_lineage_unknown_loss_stays_incomplete(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            files, scope, context = self.fixture(directory, None)
            run = directory / "run"
            graph = import_capture(run, files, context, pids={41}, encoding="utf-8", scope=scope)
            self.assertEqual(len(graph["samples"]), 1)
            self.assertEqual(graph["folded_stacks"], {"unresolved@0x123": 1})
            lineage = EvidenceLineage(run)
            envelope = lineage.verify("graphs", "windows_scheduler_sampling.json")
            self.assertFalse(envelope["completeness"]["complete"])
            records = lineage.read_normalized("windows_events.json", {"windows.etw.observation"})
            self.assertEqual(len(records), 3)
            self.assertEqual(records[1]["rawReference"]["sha256"], files["events.csv"]["sha256"])
            self.assertFalse(RawEvidence(run).verify()["capture_complete"])
            with self.assertRaises(EvidenceError):
                import_capture(run, files, context, pids={41}, encoding="utf-8", scope=scope)

    def test_disk_completion_lineage_and_missing_init(self):
        for missing in (False, True):
            with self.subTest(missing_init=missing), tempfile.TemporaryDirectory() as temporary:
                directory = Path(temporary)
                files, scope, context = self.fixture(directory, 0)
                events = directory / 'events.csv'
                rows = [['BeginHeader'],
                    ['P-Start','TimeStamp','Process Name ( PID)'],
                    ['DiskWriteInit','TimeStamp','Process Name ( PID)','ThreadID','IrpPtr'],
                    ['DiskWrite','TimeStamp','Process Name ( PID)','ThreadID','IrpPtr','IOSize'],
                    ['EndHeader'], ['P-Start','1','fixture (41)'],
                    ['DiskWrite','30','fixture (41)','9','0x123','0x200']]
                if not missing:
                    rows.append(['DiskWriteInit','10','fixture (41)','9','0x123'])
                with events.open('w', newline='', encoding='utf-8') as stream:
                    csv.writer(stream).writerows(rows)
                metadata = directory / 'metadata.json'
                data = json.loads(metadata.read_text())
                data['csv_sha256'] = digest(events)
                metadata.write_text(json.dumps(data))
                files['events.csv']['sha256'] = digest(events)
                files['metadata.json']['sha256'] = digest(metadata)
                run = directory / 'run'
                import_capture(run, files, context, pids={41}, encoding='utf-8', scope=scope)
                envelope = EvidenceLineage(run).verify('metrics', 'windows_disk_io.json')
                report = envelope['records'][0]
                self.assertEqual(report['complete'], not missing)
                if not missing:
                    self.assertEqual(report['disk_requests'][0]['latency_ns'], 20000)
                    self.assertEqual(report['disk_requests'][0]['bytes'], 512)
                self.assertEqual(report['rawReference']['sha256'], digest(events))

    def test_capacity_fail_retains_raw_without_derived_acceptance(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            files, scope, context = self.fixture(directory, 0)
            run = directory / "run"
            with self.assertRaises(EvidenceError):
                import_capture(run, files, context, pids={41}, encoding="utf-8", scope=scope, event_capacity=1)
            self.assertTrue(RawEvidence(run).manifest_path.is_file())
            self.assertFalse((run / "normalized/windows_events.json").exists())
            self.assertFalse((run / "graphs/windows_scheduler_sampling.json").exists())
