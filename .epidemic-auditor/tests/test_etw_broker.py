"""Fixed client contract and negative completeness checks; no fake service PASS."""
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import uuid

from ep_auditor.collectors.etw_broker import BrokerError, request_bytes, capture_directory, STORE
from ep_auditor.collectors.wpr import WprCollector
from ep_auditor.core.evidence import RawEvidence, digest
from ep_auditor.collectors.xperf import parse_trace_loss, trace_buffer_coverage, XperfPostProcessor
from ep_auditor.core.evidence import EvidenceError


class EtwBrokerTests(unittest.TestCase):
    def test_zero_loss_does_not_establish_truncated_buffer_coverage(self):
        header = {"buffer_size": 1048576, "buffers_written": 125, "buffers_lost": 0,
                  "events_lost": 0, "log_file_mode": 65537, "start_time": 100, "end_time": 200}
        trace_buffer_coverage(header, 131072000)
        for selected, size in ((header, 65536), (header, 131072001),
                               ({**header, "log_file_mode": 2}, 131072000),
                               ({**header, "log_file_mode": 0x04000001}, 131072000),
                               ({**header, "end_time": 0}, 131072000)):
            with self.subTest(header=selected, size=size), self.assertRaises(EvidenceError):
                trace_buffer_coverage(selected, size)

    def test_xperf_symbol_cache_and_lookup_are_owned_and_local(self):
        from subprocess import CompletedProcess
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            output = directory / "reports"
            decoder = XperfPostProcessor()
            decoder.exe = Path("xperf.exe")
            def invoke(command, **options):
                self.assertEqual(options["env"]["_NT_SYMBOL_PATH"], str(directory.resolve()))
                self.assertTrue(Path(options["env"]["_NT_SYMCACHE_PATH"]).is_relative_to(output.resolve()))
                report = Path(command[command.index("-o") + 1])
                self.assertEqual("-symbols" in command, report.stem == "profile_stacks")
                report.write_text("synthetic decoder boundary output")
                return CompletedProcess(command, 0, stdout="", stderr="")
            with patch("ep_auditor.collectors.xperf.subprocess.run", side_effect=invoke):
                result = decoder.process(directory / "trace.etl", 41, output, symbol_dirs=(directory,))
            self.assertFalse(result["errors"])
            self.assertEqual(len(result["reports"]), 3)

    def test_physical_deep_runner_registers_xperf_outputs_and_seals_raw(self):
        from test_core import AuditorCoreTests
        from ep_auditor.config import load_config
        from ep_auditor.core.process import ManagedProcessRunner
        from ep_auditor.core.storage import RunStorage
        root = Path(__file__).resolve().parents[2]
        fixture = root / "build/auditor-native/bin" / os.environ.get("EP_AUDITOR_NATIVE_CONFIG", "Release") / "EpidemicAuditorConcurrencyFixture.exe"
        self.assertTrue(fixture.is_file(), "mandatory native fixture missing")
        self.assertTrue(XperfPostProcessor().available, "mandatory independent ETL decoder missing")
        with tempfile.TemporaryDirectory() as temporary:
            _, project = AuditorCoreTests()._config(Path(temporary))
            config_path = project / ".epidemic-auditor/auditor.config.json"
            data = json.loads(config_path.read_text())
            data["capture"]["enable_wpr_in_deep"] = True
            evidence = Path(os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE", root / ".ai/auditor/test-results")) / ("deep-" + uuid.uuid4().hex)
            data["storage"] = {"runs_dir": str(evidence / "runs"), "database": str(evidence / "auditor.sqlite3")}
            config_path.write_text(json.dumps(data))
            config = load_config(config_path)
            storage = RunStorage(config)
            record = storage.new_run("process", "physical native Deep", "Deep")
            result = ManagedProcessRunner(config).run([str(fixture), "starvation", "good", "Local\\AuditorDeepTest"],
                project, record.run_dir, "Deep", timeout=30)
            self.assertEqual(result.exit_code, 0)
            raw = RawEvidence(record.run_dir)
            payload, _ = raw.read_capture("wpr.json")
            metadata = json.loads(payload)
            self.assertFalse(metadata["xperf"]["errors"], metadata["xperf"])
            self.assertEqual(set(metadata["xperf"]["reports"]), {"cpu_profile", "cpu_disk", "profile_stacks"})
            raw.seal({"run_id": record.run_id, "instrumentation_profile": "audit-deep",
                      "source_fingerprints": {}, "binary_fingerprints": {}, "analyzer_version": "current-source"})
            self.assertTrue(raw.verify()["capture_complete"])
            truncated = record.run_dir / "xperf/truncated.etl"
            with (raw.raw / "trace.etl").open("rb") as source:
                truncated.write_bytes(source.read(65536))
            negative = XperfPostProcessor().trace_statistics(truncated, record.run_dir / "xperf/truncated-statistics")
            self.assertIsNotNone(negative["native_header"], negative)
            self.assertIsNone(negative["trace_loss"], negative)
            self.assertIn("buffer coverage", negative["error"])

    def test_corrupt_physical_etl_cannot_establish_zero_loss(self):
        decoder = XperfPostProcessor()
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            trace = directory / "corrupt.etl"
            trace.write_bytes(b"deliberately corrupt physical ETL negative control")
            result = decoder.trace_statistics(trace, directory / "decoded")
            self.assertIsNone(result["trace_loss"])
            self.assertIsNotNone(result["error"])
            self.assertEqual(result["trace_sha256"], digest(trace))

    def test_empty_broker_log_is_complete_only_with_independent_zero_loss(self):
        for loss in ({"lost_events": 0, "lost_buffers": 0},
                     {"lost_events": 1, "lost_buffers": 0}, None):
            with self.subTest(loss=loss), tempfile.TemporaryDirectory() as temporary:
                directory = Path(temporary)
                capture = directory / "external-capture"
                capture.mkdir()
                trace = capture / "trace.etl"
                trace.write_bytes(b"synthetic external boundary fixture")
                (capture / "0.stderr.log").write_bytes(b"")
                def release(identity):
                    (capture / "release_receipt.json").write_text(json.dumps({"trace_sha256": digest(trace)}))
                    return {"state": "RELEASED"}
                collector = WprCollector()
                collector._run_dir, collector.active, collector.capture_id = directory / "run", True, str(uuid.uuid4())
                with patch("ep_auditor.collectors.wpr.BrokerClient") as client, \
                        patch("ep_auditor.collectors.wpr.capture_directory", return_value=capture), \
                        patch("ep_auditor.collectors.wpr.XperfPostProcessor") as decoder:
                    collector.client = client.return_value
                    collector.client.stop.return_value = {"state": "COMPLETE"}
                    collector.client.cancel.side_effect = release
                    decoder.return_value.trace_statistics.return_value = {
                        "trace_sha256": digest(trace), "trace_loss": loss, "report": None}
                    result = collector.stop()
                    self.assertIsNotNone(result, collector.note)
                raw = RawEvidence(collector._run_dir)
                raw.seal({"run_id": "RunId:" + str(uuid.uuid4()), "instrumentation_profile": "audit-deep",
                          "source_fingerprints": {}, "binary_fingerprints": {}, "analyzer_version": "synthetic-test"})
                self.assertEqual(raw.verify()["capture_complete"], loss == {"lost_events": 0, "lost_buffers": 0})

    def test_independent_loss_counters_reject_missing_duplicate_and_negative_values(self):
        report = "Total # Lost Buffers : 0\nTotal # Lost Events : 0\n"
        self.assertEqual(parse_trace_loss(report), {"lost_buffers": 0, "lost_events": 0})
        self.assertEqual(parse_trace_loss(report.replace("Events : 0", "Events : 7"))["lost_events"], 7)
        for invalid in ("", report + report, report.replace("Events : 0", "Events : -1"),
                        report.replace("Events : 0", "Events : unknown")):
            with self.subTest(invalid=invalid), self.assertRaises(EvidenceError):
                parse_trace_loss(invalid)

    def test_capture_location_is_fixed_and_rejects_old_workspace_or_escape(self):
        identity = "00112233-4455-6677-8899-aabbccddeeff"
        expected = STORE / identity
        self.assertEqual(capture_directory({"capture_id": identity, "directory": str(expected)}), expected)
        for directory in (Path(r"E:\Epidemic engine v1.0\_goal6_handoff\etw-broker\captures") / identity,
                          STORE / ".." / identity, Path(identity)):
            with self.subTest(directory=directory), self.assertRaises(BrokerError):
                capture_directory({"capture_id": identity, "directory": str(directory)})

    def test_fixed_operations_reject_command_path_profile_and_id_injection(self):
        identity = "00112233-4455-6677-8899-aabbccddeeff"
        self.assertEqual(request_bytes("START"), b"START GeneralProfile")
        self.assertEqual(request_bytes("STATUS"), b"STATUS")
        for operation in ("STOP", "CANCEL"):
            self.assertEqual(request_bytes(operation, capture_id=identity), (operation + " " + identity).encode())
        for operation, arguments in (("EXEC", {}), ("START", {"profile": "CPU"}),
                ("START", {"profile": "GeneralProfile -filemode"}), ("START", {"capture_id": identity}),
                ("STATUS", {"capture_id": identity}), ("STOP", {"capture_id": "../../trace.etl"}),
                ("CANCEL", {"capture_id": identity + "\n"}), ("STOP", {"capture_id": identity.upper()})):
            with self.subTest(operation=operation, arguments=arguments), self.assertRaises(BrokerError):
                request_bytes(operation, **arguments)

    def test_unavailable_broker_is_raw_incomplete_evidence_not_just_a_note(self):
        with tempfile.TemporaryDirectory() as temporary:
            run = Path(temporary)
            with patch("ep_auditor.collectors.wpr.BrokerClient", side_effect=BrokerError("not installed")):
                collector = WprCollector()
                collector.start(run)
            raw = RawEvidence(run)
            payload, _ = raw.read_capture("broker/admission_failure.json")
            self.assertFalse(json.loads(payload)["capture_started"])
            self.assertFalse(collector.active)
            raw.seal({"run_id": "RunId:" + str(uuid.uuid4()), "instrumentation_profile": "audit-deep",
                      "source_fingerprints": {}, "binary_fingerprints": {}, "analyzer_version": "test"})
            self.assertFalse(raw.verify()["capture_complete"])
            self.assertIsNone(collector.stop())

    def test_missing_completed_trace_remains_raw_incomplete(self):
        with tempfile.TemporaryDirectory() as temporary:
            run = Path(temporary)
            collector = WprCollector()
            collector._run_dir, collector.active, collector.capture_id = run, True, str(uuid.uuid4())
            with patch("ep_auditor.collectors.wpr.BrokerClient") as client, \
                    patch("ep_auditor.collectors.wpr.capture_directory", return_value=run / "missing"):
                collector.client = client.return_value
                collector.client.stop.return_value = {"state": "COMPLETE"}
                self.assertIsNone(collector.stop())
            payload, _ = RawEvidence(run).read_capture("broker/stop_failure.json")
            self.assertEqual(json.loads(payload)["qualification"], "INCOMPLETE")
            self.assertFalse(collector.active)
