import json
import os
from pathlib import Path
import shutil
import sys
import tempfile
import time
import unittest
from unittest.mock import patch
import psutil

from ep_auditor.collectors.process_stats import ProcessStatsCollector, windows_clock_calibration
from ep_auditor.core.owned_process import OwnedProcess
from ep_auditor.core.process import ManagedProcessRunner
from ep_auditor.core.storage import RunStorage
from ep_auditor.core.evidence import RawEvidence, load_json, EvidenceError
from ep_auditor.analyzers.summary import summarize_process_samples
import test_core


class ProcessObservationTests(unittest.TestCase):
    def test_external_clock_calibration_bounds_precise_utc_and_qpc(self):
        before = time.time_ns()
        anchor = windows_clock_calibration()
        after = time.time_ns()
        self.assertLessEqual(before, anchor["utc_unix_ns"])
        self.assertLessEqual(anchor["utc_unix_ns"], after)
        self.assertLessEqual(anchor["perf_begin_ns"], anchor["perf_end_ns"])
        self.assertLessEqual(anchor["qpc_begin_ticks"], anchor["qpc_end_ticks"])
        self.assertGreater(anchor["qpc_frequency_hz"], 0)
        self.assertEqual(anchor["provenance"], "external")

    def test_summary_uses_verified_raw_and_rejects_corruption(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            raw = RawEvidence(directory)
            payload = (json.dumps({"cpu_percent_sum": 2, "processes": []}) + "\n").encode()
            raw.append("process/process_samples.jsonl", payload, collector="fixture_sampling", version="1",
                       start_ns=1, end_ns=2,
                       health={"capacity": 1024, "observed": len(payload), "dropped": 0, "incomplete_reason": None})
            (directory / "process_samples.jsonl").write_text(json.dumps({"cpu_percent_sum": 999}) + "\n")
            self.assertEqual(summarize_process_samples(directory)["cpu_avg"], 2)
            (raw.raw / "process/process_samples.jsonl").write_bytes(payload + b"tampered")
            with self.assertRaises(EvidenceError):
                summarize_process_samples(directory)

    def test_root_exit_retains_live_child_and_grandchild_samples(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            helper = Path(__file__).parent / "fixtures/observation_tree.py"
            process = OwnedProcess([sys.executable, str(helper), "root", str(directory)], str(directory), os.environ.copy())
            collector = ProcessStatsCollector(0.02)
            try:
                collector.start(process.pid, directory)
                deadline = time.monotonic() + 10
                expected = []
                while time.monotonic() < deadline:
                    if all((directory / (role + ".json")).exists() for role in ("child", "grandchild")):
                        expected = [json.loads((directory / (role + ".json")).read_text())["pid"]
                                    for role in ("child", "grandchild")]
                        samples = [json.loads(line) for line in (directory / "process_samples.jsonl").read_text().splitlines()]
                        if all(pid in [row["pid"] for row in samples[-1]["processes"]] for pid in expected):
                            break
                    time.sleep(0.02)
                self.assertEqual(len(expected), 2)
                members_before = process.observe_members()
                self.assertEqual(members_before["availability"], "observed")
                self.assertTrue(all(pid in [row["pid"] for row in members_before["members"]] for pid in expected))
                actor_root = json.loads((directory / "root.json").read_text())["pid"]
                self.assertIn(actor_root, [row["pid"] for row in members_before["members"]])
                self.assertEqual(psutil.Process(expected[0]).ppid(), actor_root)
                self.assertEqual(psutil.Process(expected[1]).ppid(), expected[0])
                (directory / "exit-root").touch()
                while process.poll() is None and time.monotonic() < deadline:
                    time.sleep(0.01)
                self.assertIsNotNone(process.poll())
                time.sleep(0.12)
                members_after = process.observe_members()
                self.assertEqual(members_after["availability"], "observed")
                self.assertTrue(all(pid in [row["pid"] for row in members_after["members"]] for pid in expected))
                collector.stop()
                samples = [json.loads(line) for line in (directory / "process_samples.jsonl").read_text().splitlines()]
                evidence = os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE")
                if evidence:
                    target = Path(evidence)
                    target.mkdir(parents=True, exist_ok=True)
                    shutil.copy2(directory / "process_samples.jsonl", target / "root_exit_samples.jsonl")
                    (target / "expected_children.json").write_text(json.dumps(expected))
                    (target / "kernel_membership.json").write_text(json.dumps({"before": members_before, "after": members_after}))
                self.assertTrue(all(pid in [row["pid"] for row in samples[-1]["processes"]] for pid in expected),
                                "sampler discarded live descendants after observed root exit")
            finally:
                collector.stop()
                self._close_owned(process)

    def test_real_sampling_distinct_clocks_context_and_unavailable_io(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            process = OwnedProcess([sys.executable, "-c", "import time; time.sleep(30)"], str(directory), os.environ.copy())
            collector = ProcessStatsCollector(0.01)
            try:
                with patch.object(psutil.Process, "io_counters", side_effect=psutil.AccessDenied()):
                    collector.start(process.pid, directory)
                    deadline = time.monotonic() + 5
                    while collector.summary["sample_count"] < 4 and time.monotonic() < deadline:
                        time.sleep(0.01)
                    collector.stop()
                rows = [json.loads(line) for line in (directory / "process_samples.jsonl").read_text().splitlines()]
                self.assertGreater(len(rows), 2)
                self.assertIsNone(rows[0]["cpu_percent_sum"])
                self.assertTrue(all(row["read_bytes_delta"] is None and row["write_bytes_delta"] is None for row in rows))
                self.assertTrue(all(row["game_time"] is None and row["simulation_time"] is None for row in rows))
                self.assertGreater(rows[-1]["trace_time_perf_ns"], rows[0]["trace_time_perf_ns"])
                self.assertTrue(all(row["root_cpu_time_seconds"] is not None for row in rows))
                self.assertTrue(all(row["processes"][0]["rss_bytes"] > 0 for row in rows))
                contexts = [json.loads(line) for line in (directory / "children.jsonl").read_text().splitlines()]
                context = next(row for row in contexts if row["pid"] == process.pid)
                self.assertEqual(context["pid"], process.pid)
                self.assertEqual(Path(context["cwd"]), directory)
                self.assertTrue(context["command"])
                self.assertTrue(context["environment_sha256"])
                payload = (directory / "process_samples.jsonl").read_bytes()
                RawEvidence(directory).append("process/process_samples.jsonl", payload,
                    collector="windows_resource_sampling", version="2", start_ns=1, end_ns=2,
                    health={"capacity": collector.capacity_bytes, "observed": len(payload), "dropped": 0, "incomplete_reason": None})
                summary = summarize_process_samples(directory)
                self.assertIsNotNone(rows[-1]["cpu_percent_sum"])
                self.assertEqual(summary["cpu_observed_samples"], sum(row["cpu_percent_sum"] is not None for row in rows))
            finally:
                collector.stop()
                self._close_owned(process)

    def _close_owned(self, process):
        try:
            process.cancel("fixture_cleanup")
            deadline = time.monotonic() + 5
            while process.active_process_count() and time.monotonic() < deadline:
                time.sleep(0.01)
            self.assertEqual(process.active_process_count(), 0, "fixture Job did not finish termination")
            process.poll()
        finally:
            process.close()

    def test_bounded_sampler_records_loss_and_missing_cpu_stays_unavailable(self):
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            collector = ProcessStatsCollector(0.01, capacity_bytes=1024)
            collector._run_dir = directory
            path = directory / "process_samples.jsonl"
            self.assertFalse(collector._record(path, {"large": "x" * 2048}))
            state = collector.capture_state[path.name]
            self.assertEqual(state["stored"], 0)
            self.assertEqual(state["observed"], state["dropped"])
            self.assertIsNone(summarize_process_samples(directory)["cpu_avg"])
            path.write_text(json.dumps({"cpu_percent_sum": None, "processes": []}) + "\n")
            payload = path.read_bytes()
            RawEvidence(directory).append("process/process_samples.jsonl", payload,
                collector="fixture_sampling", version="1", start_ns=1, end_ns=2,
                health={"capacity": 1024, "observed": len(payload), "dropped": 0, "incomplete_reason": None})
            self.assertIsNone(summarize_process_samples(directory)["cpu_peak"])

    def test_managed_runner_catalog_binds_original_sampling_and_environment(self):
        with tempfile.TemporaryDirectory() as temporary:
            config, project = test_core.AuditorCoreTests._config(self, Path(temporary))
            storage = RunStorage(config)
            record = storage.new_run("fixture", "sampling capture", "Fast")
            result = ManagedProcessRunner(config).run([sys.executable, "-c", "import time; time.sleep(.2)"],
                                                     project, record.run_dir, "Fast", env_extra={"MS63_FIXTURE": "external"})
            self.assertEqual(result.exit_code, 0)
            capture = RawEvidence(record.run_dir)
            catalog = load_json(record.run_dir / "capture_catalog.json")
            for name in ("process_samples.jsonl", "children.jsonl"):
                raw = capture.raw / "process" / name
                self.assertEqual(raw.read_bytes(), (record.run_dir / name).read_bytes())
                self.assertEqual(catalog["files"]["process/" + name]["collector"], "windows_resource_sampling")
            lifecycle = load_json(capture.raw / "process/lifecycle.json")
            self.assertEqual(lifecycle["environment"]["MS63_FIXTURE"], "external")
            if os.environ.get("EP_AUDITOR_FIXTURE_EVIDENCE"):
                target = Path(os.environ["EP_AUDITOR_FIXTURE_EVIDENCE"]) / "managed_capture"
                target.mkdir(parents=True, exist_ok=True)
                shutil.copy2(record.run_dir / "capture_catalog.json", target / "capture_catalog.json")
                shutil.copytree(capture.raw, target / "raw")
