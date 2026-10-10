import json
import os
import uuid
import tempfile
import unittest
from pathlib import Path

from ep_auditor.core.build_receipts import FILE_CAPACITY, _file_api, _retain
from ep_auditor.core.evidence import EvidenceError, RawEvidence
from ep_auditor.core.storage import RunStorage
import test_core


class BuildReceiptMetadataTests(unittest.TestCase):
    def fixture(self, base, *, index_major=1, body_major=1, descriptor="toolchains.json"):
        if os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE"):
            base = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / ("metadata_fixture_" + uuid.uuid4().hex)
            base.mkdir(parents=True)
        config, _ = test_core.AuditorCoreTests._config(self, base)
        storage = RunStorage(config)
        record = storage.new_run("fixture", "File API metadata", "Fast")
        reply = config.build_dir / ".cmake/api/v1/reply"
        reply.mkdir(parents=True)
        objects = []
        for kind, major in (("codemodel", 2), ("toolchains", 1), ("cache", 2)):
            value = {"kind": kind, "version": {"major": body_major if kind == "toolchains" else major}}
            (reply / (kind + ".json")).write_text(json.dumps(value))
            objects.append({"kind": kind, "version": {"major": index_major if kind == "toolchains" else major},
                            "jsonFile": descriptor if kind == "toolchains" else kind + ".json"})
        (reply / "index-fixture.json").write_text(json.dumps({"objects": objects}))
        return config, storage, record

    def test_unknown_and_boolean_protocol_major_versions_are_rejected(self):
        for index_major, body_major in ((True, 1), (1, True), (1, 99)):
            with self.subTest(index=index_major, body=body_major), tempfile.TemporaryDirectory() as temporary:
                config, _, record = self.fixture(Path(temporary), index_major=index_major, body_major=body_major)
                with self.assertRaises(EvidenceError) as rejected:
                    _file_api(RawEvidence(record.run_dir), config.build_dir)
                (record.run_dir / "outcome.json").write_text(json.dumps({"case": "unsupported_schema", "error": str(rejected.exception),
                    "index_major": index_major, "body_major": body_major, "status": "REJECTED"}))

    def test_descriptor_path_escape_is_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            config, _, record = self.fixture(Path(temporary), descriptor="../outside.json")
            with self.assertRaises(EvidenceError) as rejected:
                _file_api(RawEvidence(record.run_dir), config.build_dir)
            (record.run_dir / "outcome.json").write_text(json.dumps({"case": "descriptor_escape", "error": str(rejected.exception), "status": "REJECTED"}))

    def test_known_protocol_retains_original_bytes(self):
        with tempfile.TemporaryDirectory() as temporary:
            config, storage, record = self.fixture(Path(temporary))
            _file_api(RawEvidence(record.run_dir), config.build_dir)
            storage.finish_run(record, "PASS", {})
            self.assertEqual(RawEvidence(record.run_dir).read_bytes("build/fileapi/toolchains.json"),
                             (config.build_dir / ".cmake/api/v1/reply/toolchains.json").read_bytes())
            (record.run_dir / "outcome.json").write_text(json.dumps({"case": "known_protocol", "status": "PASS"}))

    def test_over_capacity_metadata_is_known_loss_and_incomplete(self):
        with tempfile.TemporaryDirectory() as temporary:
            config, storage, record = self.fixture(Path(temporary))
            with self.assertRaises(EvidenceError):
                _retain(RawEvidence(record.run_dir), "build/large.bin", b"x" * (FILE_CAPACITY + 1))
            storage.finish_run(record, "PASS", {})
            self.assertEqual(record.status, "INCOMPLETE")
            row = RawEvidence(record.run_dir).verify()["raw_files"][0]
            self.assertEqual(row["health"]["observed"], FILE_CAPACITY + 1)
            self.assertEqual(row["health"]["dropped"], 1)
            (record.run_dir / "outcome.json").write_text(json.dumps({"case": "capacity", "status": record.status,
                                                                   "observed": row["health"]["observed"], "dropped": 1}))
