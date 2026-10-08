"""Commit batch observations from independently verified child capture artifacts."""
import json
from pathlib import Path
import time

from .evidence import EvidenceError, RawEvidence, digest, load_json
from .lifecycle import authoritative_run, seal_run_capture
from .lineage import EvidenceLineage, normalized_record
from .test_observation import junit_result, observed_cancellation


def capture_batch(record, selected, results, cancel_reason):
    if cancel_reason not in {None, "user_stop", "application_shutdown"}:
        raise EvidenceError("batch cancellation reason is not an observed user action")
    cancelled = cancel_reason is not None
    raw = RawEvidence(record.run_dir)
    capacity = 16 * 1024 * 1024

    def retain(name, data):
        now = time.perf_counter_ns()
        raw.append(name, data[:capacity], collector="external_batch_child_snapshot", version="1",
                   start_ns=now, end_ns=now,
                   health={"capacity": capacity, "observed": len(data), "dropped": max(0, len(data) - capacity),
                           "incomplete_reason": "batch_snapshot_capacity_exceeded" if len(data) > capacity else None})
        if len(data) > capacity:
            raise EvidenceError("batch child snapshot exceeds capture capacity")

    retain("batch/selection.json", json.dumps([test.to_dict() for test in selected], sort_keys=True).encode())
    observations, coverage_profiles, coverage_binaries = [], [], {}
    for index, result in enumerate(results):
        directory = Path(result["capture_dir"])
        before = digest(directory / "run.json")
        child = authoritative_run(directory)
        child_raw = RawEvidence(directory)
        child_manifest = child_raw.verify()
        if child["metadata"].get("parent_run_id") != record.run_id or child["run_id"] != result["test_run_id"]:
            raise EvidenceError("batch child identity/parent mismatch")
        prefix = "batch/children/" + str(index) + "/"
        for relative in ("run.json", "manifest.json"):
            path = directory / relative
            if path.stat().st_size > capacity:
                raise EvidenceError("batch child authority exceeds snapshot capacity")
            retain(prefix + relative, path.read_bytes())
        for relative in ("ctest/junit.xml", "ctest/compatibility.json", "process/lifecycle.json"):
            retain(prefix + relative, child_raw.read_bytes(relative))
        # Copy verified capture bytes into the existing independently replayable parent.
        for artifact in child_manifest['raw_files']:
            relative = artifact['path'].removeprefix('raw/')
            if relative.startswith('exact/') and (relative.endswith('.bin') or relative.endswith('-symbols.json') or relative.endswith('-health.json')):
                raw.import_file(relative, directory / artifact['path'], artifact['sha256'],
                    collector=artifact['collector'], version=artifact['collector_version'],
                    start_ns=artifact['start_ns'], end_ns=artifact['end_ns'], health=artifact['health'],
                    provenance=artifact['provenance'], confidence=artifact['confidence'])
            elif relative.startswith('coverage/') and relative.endswith('.profraw'):
                coverage_profiles.append(directory / artifact['path'])
        if (directory / 'raw/coverage/report.json').is_file():
            report = json.loads(child_raw.read_bytes('coverage/report.json'))
            for binary, fingerprint in report['producer_inputs'].items():
                if not binary.endswith('.profraw'):
                    if binary in coverage_binaries and coverage_binaries[binary] != fingerprint:
                        raise EvidenceError('batch mixes incompatible coverage binaries')
                    coverage_binaries[binary] = fingerprint
        if digest(directory / "run.json") != before:
            raise EvidenceError("batch child authority changed during observation")
        lifecycle = load_json(directory / "raw/process/lifecycle.json")
        try:
            status, duration = junit_result(child_raw.read_bytes("ctest/junit.xml"), result["test_name"],
                                           lifecycle["exit_code"], observed_cancellation(lifecycle))
        except EvidenceError:
            status, duration = "INCOMPLETE", None
        if child["status"] == "INCOMPLETE":
            status = "INCOMPLETE"
        if child["status"] != status or result["status"] != status:
            raise EvidenceError("batch result differs from child raw observation")
        observations.append({"child_run_id": child["run_id"], "test_id": result["test_id"],
                             "test_name": result["test_name"], "status": status, "test_seconds": duration})
    incomplete = any(row["status"] == "INCOMPLETE" for row in observations)
    status = ("INCOMPLETE" if incomplete else "CANCELLED" if cancelled or any(row["status"] == "CANCELLED" for row in observations)
              else "FAIL" if any(row["status"] == "FAIL" for row in observations)
              else "PASS" if len(observations) == len(selected) else "INCOMPLETE")
    summary = {"requested_count": len(selected), "observed_count": len(observations),
               "unstarted_count": len(selected) - len(observations), "cancel_requested": cancelled,
               "cancel_reason": cancel_reason, "status": status, "children": observations}
    retain("batch/observation.json", json.dumps(summary, sort_keys=True).encode())
    if coverage_profiles:
        from ep_auditor.collectors.coverage import collect_coverage
        if any(not Path(binary).is_file() or digest(Path(binary)) != fingerprint for binary, fingerprint in coverage_binaries.items()):
            raise EvidenceError('batch coverage binary changed after child capture')
        collect_coverage(record.run_dir, coverage_profiles, list(map(Path, coverage_binaries)),
            scope={'kind': 'selected_test_batch', 'requested_count': len(selected), 'observed_count': len(observations),
                   'tests': [test.name for test in selected], 'selection_complete': len(observations) == len(selected)})
    seal_run_capture(record)
    lineage = EvidenceLineage(record.run_dir)
    references = [lineage.reference("raw", path.relative_to(record.run_dir / "raw").as_posix())
                  for path in sorted((record.run_dir / "raw").rglob("*")) if path.is_file()]
    ref = lineage.reference("raw", "batch/observation.json")
    row = normalized_record({**summary, "schemaVersion": 1, "eventType": "ctest.batch.summary", "required": True,
                             "provenance": "external", "confidence": "derived"}, {"ctest.batch.summary"}, ref, 0)
    lineage.write("normalized", "batch_summary.json", producer_version="ctest-batch-1", producer_source=Path(__file__),
                  inputs=references, records=[row])
    return status, summary
