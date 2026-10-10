import csv
from pathlib import Path
import tempfile
import unittest

from ep_auditor.collectors.xperf import iter_windows_events, process_id
from ep_auditor.core.evidence import EvidenceError


class XperfObservationTests(unittest.TestCase):
    def test_real_xperf_unknown_readying_process_retains_target_evidence(self):
        rows = [["BeginHeader"],
                ["ReadyThread", "TimeStamp", "Process Name ( PID)", "Rdy Process Name ( PID)", "ThreadID", "Rdy TID"],
                ["EndHeader"], ["ReadyThread", "1", "Unknown (  -1)", "fixture.exe (41)", "0", "9"]]
        result = self._read(rows)
        self.assertEqual(len(result), 1)
        self.assertEqual(result[0]["scoped_pids"], [41])
        self.assertEqual(result[0]["fields"]["Process Name ( PID)"], "Unknown (  -1)")

    def _read(self, rows):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "events.csv"
            with path.open("w", newline="", encoding="utf-8") as stream:
                csv.writer(stream).writerows(rows)
            return list(iter_windows_events(path, {41}))

    def test_schema_columns_clock_and_thread_scoped_stack_lifetime(self):
        rows = [["BeginHeader"],
                ["T-Start", "ThreadID", "TimeStamp", "Process Name ( PID)"],
                ["T-End", "ThreadID", "TimeStamp", "Process Name ( PID)"],
                ["SampledProfile", "TimeStamp", "Process Name ( PID)", "ThreadID", "PrgrmCtr"],
                ["Stack", "TimeStamp", "ThreadID", "No.", "Address", "Image!Function"],
                ["EndHeader"], ["T-Start", "9", "1.25", "fixture.exe ( 41)"],
                ["T-Start", "10", "1.3", "other.exe (99)"],
                ["SampledProfile", "2", "fixture.exe (41)", "9", "0x123"],
                ["Stack", "2", "9", "0", "0x123", "Unknown"],
                ["Stack", "2", "10", "0", "0x999", "other!function"],
                ["T-End", "9", "3", "fixture.exe (41)"],
                ["Stack", "4", "9", "0", "0x555", "reused_tid!unrelated"]]
        events = self._read(rows)
        self.assertEqual([event["kind"] for event in events], ["T-Start", "SampledProfile", "Stack", "T-End"])
        self.assertEqual(events[0]["timestamp_ns"], 1250)
        self.assertEqual(events[2]["fields"]["Address"], "0x123")
        self.assertEqual(events[2]["fields"]["Image!Function"], "Unknown")
        self.assertEqual(len({event["rawOrdinal"] for event in events}), 4)

    def test_malformed_required_structure_and_scoped_payload_are_rejected(self):
        variants = [[], [["T-Start", "1", "fixture.exe (41)"]],
                    [["BeginHeader"], ["P-Start", "TimeStamp", "Process Name ( PID)"], ["EndHeader"],
                     ["P-Start", "1", "fixture.exe (41)", "unregistered extra field"]],
                    [["BeginHeader"], ["P-Start", "TimeStamp", "Process Name ( PID)"], ["EndHeader"],
                     ["P-Start", "NaN", "fixture.exe (41)"]],
                    [["BeginHeader"], ["P-Start", "TimeStamp", "Process Name ( PID)"], ["EndHeader"],
                     ["P-Start", "1", "unidentifiable"]]]
        for rows in variants:
            with self.subTest(rows=rows), self.assertRaises(EvidenceError):
                self._read(rows)

    def test_unknown_optional_events_do_not_invent_process_identity(self):
        self.assertEqual(self._read([["BeginHeader"], ["FutureOptional", "TimeStamp", "Extension"],
                                     ["EndHeader"], ["FutureOptional", "1", "opaque"]]), [])
        self.assertEqual(process_id("kernel worker ( 41)"), 41)
        with self.assertRaises(EvidenceError):
            process_id("worker without OS PID")

    def test_foreign_versioned_schemas_are_ignored_but_required_ambiguity_is_rejected(self):
        prefix = [["BeginHeader"], ["ForeignProvider", "TimeStamp", "Extension"],
                  ["ForeignProvider", "TimeStamp", "OtherExtension", "Version"],
                  ["P-Start", "TimeStamp", "Process Name ( PID)"]]
        self.assertEqual(len(self._read(prefix + [["EndHeader"], ["P-Start", "1", "fixture.exe (41)"]])), 1)
        with self.assertRaises(EvidenceError):
            self._read(prefix + [["P-Start", "Process Name ( PID)", "TimeStamp"], ["EndHeader"]])

    def test_explicit_native_codepage_and_strict_no_replacement(self):
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "native.csv"
            path.write_bytes(b"BeginHeader\nP-Start,TimeStamp,Process Name ( PID)\nEndHeader\nP-Start,1,fixture\xae.exe (41)\n")
            with self.assertRaises(UnicodeDecodeError):
                list(iter_windows_events(path, {41}))
            event = list(iter_windows_events(path, {41}, encoding="cp1252"))[0]
            self.assertEqual(event["fields"]["Process Name ( PID)"], "fixture\u00ae.exe (41)")
            self.assertEqual(event["decoder_text_encoding"], "cp1252")

    def test_observed_xperf_thread_function_extension_is_retained_opaque(self):
        prefix = [["BeginHeader"], ["T-Start", "TimeStamp", "Process Name ( PID)", "ThreadID", "Image!Function"], ["EndHeader"]]
        event = self._read(prefix + [["T-Start", "1", "fixture.exe (41)", "9", "fixture.exe!0x123", "Unknown"]])[0]
        self.assertEqual(event["fields"]["Image!Function"], "fixture.exe!0x123")
        self.assertEqual(event["decoder_extensions"], ["Unknown"])
        with self.assertRaises(EvidenceError):
            self._read(prefix + [["T-Start", "1", "fixture.exe (41)", "9", "fixture.exe!0x123", "Unknown", "unexpected second extension"]])
