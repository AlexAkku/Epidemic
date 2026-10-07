import unittest

from ep_auditor.analyzers.concurrency import candidates
from ep_auditor.core.evidence import EvidenceError


def reference(ordinal):
    return {"layer": "raw", "path": "etw/events.csv", "sha256": "a" * 64, "rawOrdinal": ordinal}


class ConcurrencyCandidateTests(unittest.TestCase):
    def test_unbound_ordinals_are_not_raw_evidence_references(self):
        graph = {"threads": [{"threadId": "t", "intervals": [
            {"state": "READY", "start_ns": 1, "end_ns": 20, "rawOrdinals": [1, 2]}]}]}
        with self.assertRaises(EvidenceError):
            candidates(graph, [], ready_threshold_ns=10)

    def test_related_positive_negative_candidates_remain_evidence_scoped(self):
        graph = {"rawReference": reference(0), "threads": [{"threadId": "t", "processId": "child", "intervals": [
            {"state": "READY", "start_ns": 1, "end_ns": 20, "rawOrdinals": [1, 2]},
            {"state": "WAIT", "start_ns": 20, "end_ns": 40, "rawOrdinals": [2, 3]},
            {"state": "RUN", "start_ns": 40, "end_ns": 50, "rawOrdinals": [3, 4]}]}]}
        processes = [{"processId": "parent", "end_ns": 30, "rawReferences": [reference(5)]},
                     {"processId": "child", "parentProcessId": "parent", "end_ns": 60, "rawReferences": [reference(6)]}]
        actions = [{"processId": "child", "observation_kind": "external_control_request", "correlationEvidence": [reference(7)],
                    "request_ns": 10, "deadline_ns": 50, "rawReferences": [reference(7)], "kind": kind} for kind in ("cancel", "complete")]
        snapshots = [{"available": True, "provenance": "external", "resourceId": "mutex", "ownerThreadId": "owner",
                      "timestamp_ns": timestamp, "rawReferences": [reference(8 + index)], "waiterThreadIds": ["t", "other"], "cycle": False}
                     for index, timestamp in enumerate((1, 20))]
        bad = candidates(graph, processes, actions, snapshots, ready_threshold_ns=10, wait_threshold_ns=10, owner_threshold_ns=10)
        rules = {value["ruleId"] for value in bad}
        self.assertEqual(len(rules), 8)
        self.assertTrue(all(value["status"] == "candidate" and value["rawReferences"] and value["scope"] for value in bad))
        self.assertEqual(candidates(graph, [{**processes[0], "end_ns": 100}, processes[1]],
                                    [{**action, "deadline_ns": 100} for action in actions], [],
                                    ready_threshold_ns=100, wait_threshold_ns=100), [])

    def test_unknown_edges_and_uncorrelated_controls_never_produce_false_findings(self):
        graph = {"threads": []}
        processes = [{"processId": "p", "end_ns": None, "rawReferences": [1]}]
        self.assertEqual(candidates(graph, processes, wait_snapshots=[{
            "available": False, "provenance": "external", "timestamp_ns": 1, "rawReferences": [2]}]), [])
        with self.assertRaises(EvidenceError):
            candidates(graph, processes, actions=[{"processId": "p", "observation_kind": "engine_declared", "correlationEvidence": []}])
        with self.assertRaises(EvidenceError):
            candidates(graph, processes, ready_threshold_ns=0)
