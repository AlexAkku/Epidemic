from __future__ import annotations

import json
import re
import subprocess
import time
from pathlib import Path
from typing import Any
from types import SimpleNamespace

from ep_auditor.config import AuditorConfig
from ep_auditor.core.process import ManagedProcessRunner, LogCallback
from ep_auditor.core.storage import RunStorage
from ep_auditor.core.evidence import EvidenceError, RawEvidence, load_json
from ep_auditor.core.environment import compiler_toolchain
from ep_auditor.core.identity import semantic_command, test_identity
from ep_auditor.core.lifecycle import seal_run_capture
from ep_auditor.core.lineage import EvidenceLineage, normalized_record
from ep_auditor.core.test_observation import compatibility, junit_result
from ep_auditor.models import TestCase, TestResult


class TestCatalog:
    def __init__(self, config: AuditorConfig, runner: ManagedProcessRunner | None = None) -> None:
        self.config = config
        self.runner = runner or ManagedProcessRunner(config)
        self.tests: list[TestCase] = []
        self.discovery_run = None

    def _collect(self, cmd, record, mode):
        result = self.runner.run(cmd, self.config.project_root, record.run_dir, mode, timeout=60)
        return SimpleNamespace(returncode=result.exit_code, cancelled=result.cancelled,
                               stdout=(record.run_dir / "raw/process/stdout.bin").read_bytes(),
                               stderr=(record.run_dir / "raw/process/stderr.bin").read_bytes())

    def discover(self) -> list[TestCase]:
        cmd = [
            "ctest", "--test-dir", str(self.config.build_dir),
            "-C", self.config.build_config,
            "--show-only=json-v1",
            *self.config.ctest_args,
        ]
        storage = RunStorage(self.config)
        mode = self.config.data.get("capture", {}).get("selected_mode", "Fast")
        record = storage.new_run("discovery", "CTest catalog discovery", mode)
        self.discovery_run = record
        start_ns = time.perf_counter_ns()
        try:
            proc = self._collect(cmd, record, mode)
        except BaseException:
            storage.finish_run(record, "INCOMPLETE", {"operation": "ctest.discovery", "command": cmd,
                                                       "reason": "discovery transport interrupted"})
            raise
        end_ns = time.perf_counter_ns()
        raw = RawEvidence(record.run_dir)
        limit = 16 * 1024 * 1024
        dropped = 0
        for channel, data in (("stdout", proc.stdout), ("stderr", proc.stderr)):
            # Test doubles may provide text; the actual collector always retains original bytes.
            payload = data.encode("utf-8") if isinstance(data, str) else data
            loss = max(0, len(payload) - limit)
            dropped += loss
            raw.append("ctest/" + channel + ".bin", payload[:limit], collector="ctest.discovery." + channel,
                       version="1", start_ns=start_ns, end_ns=end_ns,
                       health={"capacity": limit, "observed": len(payload), "dropped": loss,
                               "incomplete_reason": "raw_capture_budget_exceeded" if loss else None})
        summary = {"operation": "ctest.discovery", "command": cmd, "exit_code": proc.returncode,
                   "transport_capacity_kind": "retained_raw_bytes_per_channel"}
        manifest = seal_run_capture(record)
        if getattr(proc, "cancelled", False):
            storage.finish_run(record, "CANCELLED", summary)
            raise RuntimeError("CTest discovery cancelled")
        if dropped or not manifest["capture_complete"]:
            storage.finish_run(record, "INCOMPLETE", summary)
            raise EvidenceError("CTest catalog capture exceeded raw storage capacity")
        if proc.returncode != 0:
            storage.finish_run(record, "FAIL", summary)
            diagnostic = proc.stderr or proc.stdout
            if isinstance(diagnostic, bytes):
                diagnostic = diagnostic.decode("utf-8", errors="replace")
            raise RuntimeError(f"CTest discovery failed ({proc.returncode}):\n{diagnostic}")
        try:
            payload = json.loads(raw.read_bytes("ctest/stdout.bin"))
            tests = self._parse_catalog(payload)
            lineage = EvidenceLineage(record.run_dir)
            reference = lineage.reference("raw", "ctest/stdout.bin")
            entries = [test.to_dict() for test in tests] or [{"count": 0, "observation": "empty_ctest_catalog"}]
            records = [normalized_record({**entry, "schemaVersion": 1, "eventType": "ctest.catalog.entry",
                                          "required": True, "provenance": "external", "confidence": "observed"},
                                         {"ctest.catalog.entry"}, reference, index) for index, entry in enumerate(entries)]
            lineage.write("normalized", "ctest_catalog.json", producer_version="ctest-catalog-1", producer_source=Path(__file__),
                          inputs=[reference], records=records)
        except Exception as error:
            storage.finish_run(record, "FAIL", {**summary, "catalog_error": str(error)})
            raise
        storage.finish_run(record, "PASS", {**summary, "test_count": len(tests)})
        self.tests = tests
        return tests

    def _parse_catalog(self, payload: dict) -> list[TestCase]:
        if not isinstance(payload.get("tests"), list):
            raise EvidenceError("CTest catalog tests array absent")
        profile = {"configuration": self.config.build_config, "build_dir": str(self.config.build_dir),
                   "toolchain": compiler_toolchain(self.config.build_dir)}
        tests: list[TestCase] = []
        for idx, raw in enumerate(payload.get("tests", []), start=1):
            props = self._properties(raw.get("properties", []))
            labels = props.get("LABELS", [])
            if isinstance(labels, str):
                labels = [labels]
            timeout = props.get("TIMEOUT")
            try:
                timeout_value = float(timeout) if timeout is not None else None
            except (TypeError, ValueError):
                timeout_value = None
            name = raw.get("name")
            command = raw.get("command")
            if (not isinstance(name, str) or not name or not isinstance(command, list) or not command
                    or any(not isinstance(value, str) for value in command)):
                raise EvidenceError("CTest name/executable/arguments missing")
            executable_identity, stable_arguments = semantic_command(command, self.config.project_root,
                                                                      self.config.build_dir, self.config.build_config)
            group = self._group_for(name, [str(x) for x in labels])
            tests.append(TestCase(
                name=name,
                labels=[str(x) for x in labels],
                command=command,
                working_directory=props.get("WORKING_DIRECTORY"),
                timeout=timeout_value,
                group=group,
                index=idx,
                test_id=test_identity(name, executable_identity, stable_arguments),
                executable=command[0],
                arguments=command[1:],
                source_location=self._source_location(payload.get("backtraceGraph", {}), raw.get("backtrace")),
                build_profile=profile,
            ))
        return tests

    def _source_location(self, graph: dict, frame_id: int | None) -> dict | None:
        nodes = graph.get("nodes", [])
        files = graph.get("files", [])
        seen = set()
        while frame_id is not None:
            if type(frame_id) is not int or frame_id in seen or not 0 <= frame_id < len(nodes):
                raise EvidenceError("invalid or cyclic CTest source backtrace")
            seen.add(frame_id)
            node = nodes[frame_id]
            file_id = node.get("file")
            if file_id is not None and "line" in node:
                if type(file_id) is not int or not 0 <= file_id < len(files):
                    raise EvidenceError("invalid CTest source file reference")
                source = Path(files[file_id])
                if not source.is_absolute():
                    source = self.config.project_root / source
                return {"file": str(source.resolve()), "line": node["line"], "provenance": "external", "confidence": "observed"}
            frame_id = node.get("parent")
        return None

    @staticmethod
    def _properties(raw: Any) -> dict[str, Any]:
        if isinstance(raw, dict):
            return raw
        result: dict[str, Any] = {}
        if isinstance(raw, list):
            for item in raw:
                if isinstance(item, dict) and "name" in item:
                    result[str(item["name"])] = item.get("value")
        return result

    @staticmethod
    def _group_for(name: str, labels: list[str]) -> str:
        if labels:
            return labels[0]
        for sep in (".", "/", "::", "_"):
            if sep in name:
                return name.split(sep, 1)[0]
        return "Ungrouped"


class TestExecutor:
    def __init__(self, config: AuditorConfig, runner: ManagedProcessRunner, storage: RunStorage) -> None:
        self.config = config
        self.runner = runner
        self.storage = storage

    def run_one(self, test: TestCase, parent_run_dir: Path, capture_mode: str, log: LogCallback | None = None) -> TestResult:
        config = self.config.for_mode(capture_mode)
        compatibility_id, compatibility_bytes = compatibility(config, test)
        parent = load_json(parent_run_dir / "run.json")
        parent_id = parent["run_id"]
        scope = parent.get("metadata", {}).get("mode_selection", {}).get("target_scope", [])
        child = self.storage.new_run("test", test.name, capture_mode, target_scope=scope)
        child.metadata["parent_run_id"] = parent_id
        test_dir = child.run_dir
        spool = test_dir / "collector_spool"
        spool.mkdir()
        junit = spool / "ctest.junit.xml"
        exact = "^" + re.escape(test.name) + "$"
        cmd = [
            "ctest", "--test-dir", str(config.build_dir),
            "-C", config.build_config,
            "--output-on-failure",
            "--output-junit", str(junit),
            "-R", exact,
            *self.config.ctest_args,
        ]
        timeout = test.timeout or self.config.test_timeout
        env_extra = {}
        if capture_mode.lower() != 'coverage' and next(config.build_dir.rglob('AuditorExactRuntime.vcxproj'), None):
            env_extra['EP_AUDITOR_REQUIRE_EXACT'] = '1'
        # Only a real C++ coverage build declares a mandatory producer. Generic
        # CTest projects (including LANGUAGES NONE) cannot promise C++ coverage.
        if capture_mode.lower() == 'coverage' and (config.build_dir / 'CMakeFiles').is_dir():
            executable = Path(test.executable or (test.command[0] if test.command else ''))
            if executable.is_file() and any((config.build_dir / 'CMakeFiles').glob('*/CMakeCXXCompiler.cmake')):
                env_extra['EP_AUDITOR_COVERAGE_BINARIES'] = json.dumps([str(executable)])
        process = self.runner.run(cmd, self.config.project_root, test_dir, capture_mode, log_callback=log,
                                  env_extra=env_extra, timeout=timeout)
        payload = junit.read_bytes() if junit.is_file() else b""
        raw = RawEvidence(test_dir)
        now = time.perf_counter_ns()
        for path, data in (("ctest/junit.xml", payload), ("ctest/compatibility.json", compatibility_bytes)):
            raw.append(path, data, collector="ctest.result", version="1", start_ns=now, end_ns=now,
                       health={"capacity": max(1, len(data)), "observed": len(data), "dropped": 0,
                               "incomplete_reason": None})
        seal_run_capture(child)
        observation_error = None
        try:
            status, test_seconds = junit_result(raw.read_bytes("ctest/junit.xml"), test.name,
                                                process.exit_code, process.cancelled)
        except EvidenceError as error:
            status, test_seconds, observation_error = "INCOMPLETE", None, str(error)
        excerpt = self._failure_excerpt(test_dir / "stdout.log", test_dir / "stderr.log") if status != "PASS" else ""
        result = TestResult(
            test_name=test.name,
            status=status,
            exit_code=process.exit_code,
            wall_seconds=process.wall_seconds,
            run_id=parent_id,
            capture_dir=str(test_dir),
            labels=test.labels,
            peak_rss_bytes=process.peak_rss_bytes,
            peak_threads=process.peak_threads,
            failure_excerpt=excerpt,
            test_id=test.test_id, compatibility_id=compatibility_id, test_run_id=child.run_id,
        )
        lineage = EvidenceLineage(test_dir)
        references = [lineage.reference("raw", path) for path in
                      ("ctest/junit.xml", "ctest/compatibility.json", "process/lifecycle.json")]
        row = normalized_record({**result.to_dict(), "test_seconds": test_seconds,
                                 "observation_error": observation_error, "schemaVersion": 1,
                                 "eventType": "ctest.result", "required": True,
                                 "provenance": "external", "confidence": "derived"},
                                {"ctest.result"}, references[0], 0)
        lineage.write("normalized", "test_result.json", producer_version="ctest-result-1",
                      producer_source=Path(__file__), inputs=references, records=[row])
        self.storage.finish_run(child, status, {"test_id": test.test_id, "parent_run_id": parent_id})
        result.status = child.status
        self.storage.evidence_index.index(test_dir, "tests", "normalized", "test_result.json")
        RunStorage.write_json(test_dir / "test_result.json", result.to_dict())
        return result

    @staticmethod
    def _failure_excerpt(*paths: Path) -> str:
        text = ""
        for path in paths:
            if path.exists():
                text += path.read_text(encoding="utf-8", errors="replace") + "\n"
        lines = [line for line in text.splitlines() if line.strip()]
        return "\n".join(lines[-80:])
