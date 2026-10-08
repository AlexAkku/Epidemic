from __future__ import annotations

import json
import time
from pathlib import Path
from ep_auditor.collectors.etw_broker import BrokerClient, capture_directory
from ep_auditor.collectors.xperf import XperfPostProcessor
from ep_auditor.core.evidence import RawEvidence, digest


class WprCollector:
    def __init__(self, profile: str = "GeneralProfile") -> None:
        self.profile = profile
        self.available = False
        self.client = None
        self.capture_id = None
        self.metadata = {}
        self.active = False
        self.note = ""
        self._run_dir: Path | None = None

    def start(self, run_dir: Path) -> None:
        self._run_dir = run_dir
        try:
            self.client = BrokerClient()
            self.metadata["start"] = self.client.start(self.profile)
            self.capture_id = self.metadata["start"]["capture_id"]
            self.available = self.active = True
        except Exception as exc:  # external tool failure must not break the run
            self.note = f"ETW broker capture unavailable (mandatory ETW qualification remains incomplete): {exc}"
            observed = time.perf_counter_ns()
            payload = json.dumps({"capture_started": False, "profile": self.profile, "error": str(exc),
                "provenance": "external_broker_admission", "confidence": "observed",
                "qualification": "INCOMPLETE"}, sort_keys=True).encode()
            RawEvidence(run_dir).append("broker/admission_failure.json", payload,
                collector="non_elevated_broker_client", version="1", start_ns=observed, end_ns=time.perf_counter_ns(),
                health={"capacity": len(payload), "observed": len(payload), "dropped": 0,
                        "incomplete_reason": "etw_broker_capture_unavailable"})

    def stop(self) -> Path | None:
        if not self.active or not self._run_dir:
            return None
        try:
            stopped = self.client.stop(self.capture_id)
            self.metadata["stop"] = stopped
            directory = capture_directory(stopped)
            source = directory / "trace.etl"
            if not source.is_file() or source.stat().st_size <= 0:
                raise RuntimeError("completed broker trace missing or empty")
            raw = RawEvidence(self._run_dir)
            started = time.perf_counter_ns()
            statistics = XperfPostProcessor().trace_statistics(source, self._run_dir / "xperf")
            self.metadata["trace_statistics"] = statistics
            loss = statistics["trace_loss"]
            loss_reason = ("trace_loss_not_yet_independently_verified" if loss is None else
                           "etw_event_loss" if any(loss.values()) else None)
            report = statistics["report"]
            if report:
                report = Path(report)
                size = report.stat().st_size
                if size <= 1024 * 1024:
                    raw.import_file("xperf/trace_statistics.txt", report, digest(report),
                        collector="external_xperf_trace_statistics", version="1",
                        start_ns=started, end_ns=time.perf_counter_ns(), health={"capacity": 1024 * 1024,
                            "observed": size, "dropped": 0, "incomplete_reason": loss_reason})
            for path in sorted(directory.iterdir()):
                if path.is_file() and (path.name == "trace.etl" or path.suffix in {".json", ".log", ".wprp"}):
                    size = path.stat().st_size
                    if size > 2 * 1024**3:
                        raise RuntimeError("broker output exceeded bounded capture size")
                    raw.import_file(path.name if path.name == "trace.etl" else "broker/" + path.name,
                        path, statistics["trace_sha256"] if path.name == "trace.etl" else digest(path),
                        collector="external_privileged_etw_broker", version="1",
                        start_ns=started, end_ns=time.perf_counter_ns(), health={"capacity": 2 * 1024**3,
                            "observed": size, "dropped": 0, "incomplete_reason": loss_reason})
            self.metadata["release"] = self.client.cancel(self.capture_id)
            if self.metadata["release"].get("state") != "RELEASED":
                raise RuntimeError("completed capture lease release unproven")
            release_file = directory / "release_receipt.json"
            release_value = json.loads(release_file.read_text())
            if release_value.get("trace_sha256") != digest(raw.raw / source.name):
                raise RuntimeError("broker release differs from immutable raw trace")
            raw.import_file("broker/release_receipt.json", release_file, digest(release_file),
                collector="external_privileged_etw_broker", version="1", start_ns=started, end_ns=time.perf_counter_ns(),
                health={"capacity": release_file.stat().st_size, "observed": release_file.stat().st_size,
                        "dropped": 0, "incomplete_reason": loss_reason})
            payload = json.dumps(self.metadata, sort_keys=True).encode()
            raw.append("broker/client_receipt.json", payload, collector="non_elevated_broker_client", version="1",
                start_ns=started, end_ns=time.perf_counter_ns(), health={"capacity": len(payload), "observed": len(payload),
                    "dropped": 0, "incomplete_reason": loss_reason})
            self.active = False
            return raw.raw / source.name
        except Exception as exc:
            self.note = f"ETW broker stop/import failed: {exc}"
            try:
                self.client.cancel(self.capture_id)
            except Exception:
                pass
            self.active = False
            observed = time.perf_counter_ns()
            payload = json.dumps({"capture_id": self.capture_id, "error": str(exc),
                "provenance": "external_broker_stop_import", "confidence": "observed",
                "qualification": "INCOMPLETE"}, sort_keys=True).encode()
            RawEvidence(self._run_dir).append("broker/stop_failure.json", payload,
                collector="non_elevated_broker_client", version="1", start_ns=observed, end_ns=time.perf_counter_ns(),
                health={"capacity": len(payload), "observed": len(payload), "dropped": 0,
                        "incomplete_reason": "etw_broker_stop_or_import_failed"})
            return None
