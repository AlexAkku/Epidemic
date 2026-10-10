from copy import deepcopy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.ms6.process_observation import require_successful_process


class ProcessSuccessOracleTests(unittest.TestCase):
    def test_cleanup_is_accepted_only_with_original_success_and_exact_observed_purpose(self):
        original = {"exit_code": 0, "cancelled": False, "creation_filetime_100ns": 123,
                    "image_sha256": "a" * 64, "termination": None}
        require_successful_process(original)
        cleanup = deepcopy(original)
        cleanup["termination"] = {"reason": "root_exit_live_descendants", "purpose": "scope_cleanup",
            "method": "TerminateJobObject", "active_processes_before": 1,
            "provenance": "external", "confidence": "observed"}
        require_successful_process(cleanup)
        for key, wrong in (("reason", "user_stop"), ("purpose", "cancellation"),
                           ("method", "unobserved"), ("active_processes_before", 0),
                           ("active_processes_before", True), ("provenance", "engine"),
                           ("confidence", "declared")):
            candidate = deepcopy(cleanup)
            candidate["termination"][key] = wrong
            with self.assertRaises(RuntimeError):
                require_successful_process(candidate)
        for key, wrong in (("exit_code", None), ("exit_code", 1), ("cancelled", True),
                           ("cancelled", None), ("creation_filetime_100ns", None), ("image_sha256", "")):
            candidate = deepcopy(original)
            candidate[key] = wrong
            with self.assertRaises(RuntimeError):
                require_successful_process(candidate)
