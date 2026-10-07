import copy
import sys
import tempfile
import unittest
import uuid
from pathlib import Path

from ep_auditor.core.evidence import EvidenceError, RawEvidence, atomic_json, digest
from ep_auditor.core.lineage import EvidenceLineage, normalized_record


class EvidenceLineageTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.producer = Path(__file__)
        self.capture = RawEvidence(self.root)

    def tearDown(self):
        self.temp.cleanup()

    def seal(self, dropped=0):
        self.capture.append("events.jsonl", b'{"value":7}\n', collector="fixture", version="1",
                            start_ns=10, end_ns=20,
                            health={"capacity": 64, "observed": 1, "dropped": dropped, "incomplete_reason": None})
        self.capture.seal({"run_id": "RunId:" + str(uuid.uuid4()), "instrumentation_profile": "audit",
                           "source_fingerprints": {"fixture": digest(self.producer)},
                           "binary_fingerprints": {"interpreter": digest(Path(sys.executable))}, "analyzer_version": "1"})
        self.lineage = EvidenceLineage(self.root)
        self.ref = self.lineage.reference("raw", "events.jsonl")

    def row(self, **changes):
        return {"schemaVersion": 1, "eventType": "fixture.observation", "required": True,
                "provenance": "external", "confidence": "observed", "extension": {"future": 7}, **changes}

    def normalize(self):
        row = normalized_record(self.row(), {"fixture.observation"}, self.ref, 0)
        return self.lineage.write("normalized", "v1.json", producer_version="1", producer_source=self.producer,
                                  inputs=[self.ref], records=[row])

    def test_forward_fields_and_stable_event_identity(self):
        self.seal()
        envelope = self.normalize()
        rows = self.lineage.read_normalized("v1.json", {"fixture.observation"})
        self.assertEqual(rows[0]["extension"], {"future": 7})
        self.assertEqual(rows[0]["eventId"], envelope["records"][0]["eventId"])
        self.assertNotEqual(rows[0]["eventId"], normalized_record(self.row(), {"fixture.observation"}, self.ref, 1)["eventId"])

    def test_unknown_required_type_explicit_failure_optional_retained(self):
        self.seal()
        self.normalize()
        with self.assertRaisesRegex(EvidenceError, "unknown required record type"):
            self.lineage.read_normalized("v1.json", {"different.type"})
        optional = normalized_record(self.row(required=False), set(), self.ref, 0)
        self.assertEqual(optional["typeSupport"], "unknown_optional")
        self.assertEqual(optional["extension"], {"future": 7})

    def test_schema_policy_and_provenance_negatives(self):
        self.seal()
        for changes in ({"schemaVersion": 2}, {"schemaVersion": True}, {"eventType": ""}, {"required": None},
                        {"provenance": "engine_log"}, {"confidence": "PASS"}, {"eventId": "EventId:fake"}):
            with self.subTest(changes=changes), self.assertRaises(EvidenceError):
                normalized_record(self.row(**changes), {"fixture.observation"}, self.ref, 0)

    def test_graph_metric_finding_lineage_and_corruption(self):
        self.seal()
        self.normalize()
        lower = self.lineage.reference("normalized", "v1.json")
        for layer in ("graphs", "metrics", "findings"):
            result = self.lineage.write(layer, "v1.json", producer_version="1", producer_source=self.producer,
                                        inputs=[lower], records=[{"provenance": "external", "confidence": "derived"}])
            self.assertEqual(result["inputs"], [lower])
            self.assertEqual(result["producerSourceSha256"], digest(self.producer))
            lower = self.lineage.reference(layer, "v1.json")
        normalized = self.root / "normalized/v1.json"
        normalized.write_bytes(normalized.read_bytes() + b" ")
        with self.assertRaisesRegex(EvidenceError, "hash mismatch"):
            self.lineage.verify("findings", "v1.json")

    def test_wrong_direction_missing_inputs_and_missing_metadata(self):
        self.seal()
        envelope = self.normalize()
        normalized_ref = self.lineage.reference("normalized", "v1.json")
        with self.assertRaisesRegex(EvidenceError, "lower evidence layer"):
            self.lineage.write("normalized", "cycle.json", producer_version="1", producer_source=self.producer,
                               inputs=[normalized_ref], records=envelope["records"])
        for field in ("inputs", "producerVersion", "producerSourceSha256", "generatedAt", "completeness"):
            with self.subTest(field=field):
                broken = copy.deepcopy(envelope)
                broken.pop(field)
                atomic_json(self.root / "normalized/broken.json", broken)
                with self.assertRaises(EvidenceError):
                    self.lineage.verify("normalized", "broken.json")
        with self.assertRaises(EvidenceError):
            self.lineage.write("metrics", "empty.json", producer_version="1", producer_source=self.producer,
                               inputs=[], records=[{"provenance": "external", "confidence": "derived"}])

    def test_dropped_input_cannot_become_complete(self):
        self.seal(dropped=1)
        envelope = self.normalize()
        self.assertFalse(envelope["completeness"]["complete"])
        envelope["completeness"] = {"complete": True, "incompleteReason": None}
        atomic_json(self.root / "normalized/forged.json", envelope)
        with self.assertRaisesRegex(EvidenceError, "completeness disagrees"):
            self.lineage.verify("normalized", "forged.json")

    def test_existing_output_cannot_be_overwritten(self):
        self.seal()
        self.normalize()
        with self.assertRaisesRegex(EvidenceError, "already exists"):
            self.normalize()

    def test_output_writer_collision_and_invalid_time_rejected(self):
        self.seal()
        (self.root / "normalized").mkdir()
        claim = self.root / "normalized/v1.json.writer.lock"
        claim.write_bytes(b"active owner")
        with self.assertRaisesRegex(EvidenceError, "writer active or interrupted"):
            self.normalize()
        claim.unlink()
        value = self.normalize()
        for timestamp in ("yesterday", "2026-10-06T01:00:00"):
            value["generatedAt"] = timestamp
            atomic_json(self.root / "normalized/bad-time.json", value)
            with self.assertRaisesRegex(EvidenceError, "invalid derived generation time"):
                self.lineage.verify("normalized", "bad-time.json")
