import unittest

from ep_auditor.analyzers.windows_threads import thread_timeline, project_monotonic_time
from ep_auditor.core.evidence import EvidenceError


def event(kind, timestamp, ordinal, **fields):
    return {"kind": kind, "timestamp_ns": timestamp, "rawOrdinal": ordinal, "fields": fields}


class WindowsThreadTests(unittest.TestCase):
    def test_clock_projection_preserves_bounds_rejects_extrapolation_and_drift(self):
        anchors = [{"perf_begin_ns": 1000, "perf_end_ns": 1100, "utc_unix_ns": 1000000,
                    "provenance": "external", "confidence": "observed"},
                   {"perf_begin_ns": 3000, "perf_end_ns": 3100, "utc_unix_ns": 1002000,
                    "provenance": "external", "confidence": "observed"}]
        bound = project_monotonic_time(2000, anchors, 999000)
        self.assertEqual((bound["lower_ns"], bound["upper_ns"]), (1800, 2100))
        for timestamp, selected in ((999, anchors), (3101, anchors),
                                    (2000, [anchors[0], {**anchors[1], "utc_unix_ns": 5000000}]),
                                    (2000, [{**anchors[0], "provenance": "engine_declared"}, anchors[1]])):
            with self.subTest(timestamp=timestamp), self.assertRaises(EvidenceError):
                project_monotonic_time(timestamp, selected, 999000)

    def test_process_parent_pid_reuse_and_trace_namespaces_are_bound(self):
        rows = [event("P-Start", 0, 40, **{"Process Name ( PID)": "parent (40)", "ParentPID": "1", "UniqueKey": "p"}),
                event("P-Start", 1, 41, **{"Process Name ( PID)": "child (41)", "ParentPID": "40", "UniqueKey": "c"})]
        rows += self._fixture()
        rows += [event("P-End", 16, 42, **{"Process Name ( PID)": "child (41)", "UniqueKey": "c", "Status": "0x0"}),
                 event("P-End", 17, 43, **{"Process Name ( PID)": "parent (40)", "UniqueKey": "p", "Status": "0x0"}),
                 event("P-Start", 20, 44, **{"Process Name ( PID)": "other (41)", "ParentPID": "40", "UniqueKey": "reused"})]
        result = thread_timeline(rows, identity_namespace="trace-A")
        parent, child, reused = result["processes"]
        self.assertEqual(child["parentProcessId"], parent["processId"])
        self.assertIsNone(reused["parentProcessId"])
        self.assertEqual(result["threads"][0]["processId"], child["processId"])
        self.assertNotEqual(child["processId"], reused["processId"])
        self.assertNotEqual(result["threads"][0]["threadId"], thread_timeline(rows, identity_namespace="trace-B")["threads"][0]["threadId"])
        bad = rows + [event("P-Start", 21, 45, **{"Process Name ( PID)": "overlap (41)", "ParentPID": "1"})]
        with self.assertRaises(EvidenceError):
            thread_timeline(bad)
        with self.assertRaises(EvidenceError):
            thread_timeline(rows[:2] + [event("P-End", 2, 46, **{"Process Name ( PID)": "child (41)", "UniqueKey": "wrong"})])

    def test_blocked_stack_is_distinct_from_acquisition_and_ambiguous_is_unknown(self):
        rows = self._fixture()
        rows.insert(6, event("Stack", 5, 30, **{"ThreadID": "9", "No.": "1", "Address": "0x1", "Image!Function": "fixture!wait"}))
        wait = thread_timeline(rows)["waits"][0]
        self.assertEqual(wait["blocked_stack_status"], "observed_frames")
        self.assertEqual(wait["blocked_stack"][0]["rawOrdinal"], 30)
        self.assertIsNone(wait["acquisition_stack"])
        self.assertIsNone(wait["resourceId"])
        rows.insert(6, event("ReadyThread", 5, 31, **{"ThreadID": "9", "Rdy TID": "10"}))
        wait = thread_timeline(rows)["waits"][0]
        self.assertEqual(wait["blocked_stack_status"], "ambiguous")
        self.assertEqual(wait["blocked_stack"], [])

    def _fixture(self):
        return [event("T-Start", 1, 0, **{"ThreadID": "9", "Process Name ( PID)": "fixture.exe (41)"}),
                event("CSwitch", 2, 1, **{"Old TID": "0", "New TID": "9", "OldState": "Running"}),
                event("SampledProfile", 3, 2, ThreadID="9", PrgrmCtr="0xabc", Count="1"),
                event("Stack", 3, 3, **{"ThreadID": "9", "No.": "1", "Address": "0xabc", "Image!Function": "fixture!leaf"}),
                event("Stack", 3, 4, **{"ThreadID": "9", "No.": "2", "Address": "0xdef", "Image!Function": "fixture!root"}),
                event("CSwitch", 5, 5, **{"Old TID": "9", "New TID": "0", "OldState": "Waiting"}),
                event("ReadyThread", 10, 6, **{"Rdy TID": "9"}),
                event("CSwitch", 12, 7, **{"Old TID": "0", "New TID": "9", "OldState": "Running"}),
                event("T-End", 15, 8, ThreadID="9")]

    def test_run_wait_ready_intervals_and_flame_fold_are_raw_bound(self):
        result = thread_timeline(self._fixture())
        thread = result["threads"][0]
        self.assertEqual([(i["state"], i["start_ns"], i["end_ns"]) for i in thread["intervals"]],
                         [("RUN", 2, 5), ("WAIT", 5, 10), ("READY", 10, 12), ("RUN", 12, 15)])
        self.assertEqual(thread["observed_run_ns"], 6)
        self.assertEqual(result["folded_stacks"], {"fixture!root;fixture!leaf": 1})
        self.assertEqual(result["samples"][0]["frames"][0]["rawOrdinal"], 3)

    def test_ambiguous_trigger_never_becomes_a_known_sample_stack(self):
        rows = self._fixture()
        rows.insert(3, event("CSwitch", 3, 20, **{"Old TID": "9", "New TID": "9", "OldState": "Ready"}))
        result = thread_timeline(rows)
        self.assertEqual(result["samples"][0]["stack_status"], "ambiguous")
        self.assertEqual(result["folded_stacks"], {"unresolved@0xabc": 1})

    def test_tid_reuse_is_separate_and_open_interval_has_no_fake_end(self):
        rows = self._fixture()
        rows += [event("T-Start", 20, 9, **{"ThreadID": "9", "Process Name ( PID)": "fixture.exe (41)"}),
                 event("CSwitch", 21, 10, **{"Old TID": "0", "New TID": "9", "OldState": "Running"})]
        result = thread_timeline(rows)
        self.assertNotEqual(result["threads"][0]["threadId"], result["threads"][1]["threadId"])
        self.assertIsNone(result["threads"][1]["open_interval"]["end_ns"])
        self.assertIsNone(result["threads"][1]["end_ns"])

    def test_ready_thread_stack_trigger_collision_is_ambiguous(self):
        rows = self._fixture()
        rows.insert(3, event("ReadyThread", 3, 20, **{"ThreadID": "9", "Rdy TID": "9"}))
        result = thread_timeline(rows)
        self.assertEqual(result["samples"][0]["stack_status"], "ambiguous")
        self.assertEqual(result["samples"][0]["frames"], [])

    def test_overlapping_lifetimes_and_backwards_intervals_fail(self):
        rows = self._fixture()
        duplicate = event("T-Start", 4, 30, **{"ThreadID": "9", "Process Name ( PID)": "other.exe (42)"})
        with self.assertRaises(EvidenceError):
            thread_timeline(rows[:3] + [duplicate])
        with self.assertRaises(EvidenceError):
            thread_timeline(rows[:2] + [event("T-End", 0, 30, ThreadID="9")])
