"""Fixed client contract and negative completeness checks; no fake service PASS."""
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import uuid

from ep_auditor.collectors.etw_broker import BrokerError, request_bytes
from ep_auditor.collectors.wpr import WprCollector
from ep_auditor.core.evidence import RawEvidence


class EtwBrokerTests(unittest.TestCase):
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
