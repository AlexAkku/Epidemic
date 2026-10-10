"""Evidence-scoped concurrency candidates, never inferred semantic causal edges."""
from ep_auditor.core.evidence import EvidenceError
from ep_auditor.core.lineage import SHA256


def candidates(graph, processes, actions=(), wait_snapshots=(), *, ready_threshold_ns=100000000,
               wait_threshold_ns=1000000000, owner_threshold_ns=1000000000):
    thresholds = (ready_threshold_ns, wait_threshold_ns, owner_threshold_ns)
    if any(type(value) is not int or value <= 0 for value in thresholds):
        raise EvidenceError("candidate thresholds must be positive trace durations")
    findings = []

    def scheduler_references(ordinals):
        reference = graph.get("rawReference")
        if not isinstance(reference, dict):
            raise EvidenceError("scheduler raw reference missing")
        if any(type(ordinal) is not int or ordinal < 0 for ordinal in ordinals):
            raise EvidenceError("scheduler raw ordinal invalid")
        return [{**reference, "rawOrdinal": ordinal} for ordinal in ordinals]

    def emit(rule, subject, evidence, measurements, scope):
        if not evidence or any(not isinstance(ref, dict) or ref.get("layer") != "raw"
                               or not isinstance(ref.get("path"), str) or not ref["path"]
                               or not SHA256.fullmatch(str(ref.get("sha256", ""))) for ref in evidence):
            raise EvidenceError("concurrency candidate lacks raw evidence")
        findings.append({"ruleId": rule, "subjectId": subject, "rawReferences": evidence,
                         "measurements": measurements, "scope": scope, "status": "candidate",
                         "provenance": "external", "confidence": "derived"})

    by_id = {process["processId"]: process for process in processes}
    if len(by_id) != len(processes):
        raise EvidenceError("ambiguous process identities")
    for thread in graph["threads"]:
        for interval in thread["intervals"]:
            duration = interval["end_ns"] - interval["start_ns"]
            if duration < 0:
                raise EvidenceError("negative scheduler duration")
            state = interval["state"]
            threshold = ready_threshold_ns if state == "READY" else wait_threshold_ns
            if state in {"READY", "WAIT"} and duration >= threshold:
                emit("windows.scheduler.starvation_candidate" if state == "READY" else "windows.scheduler.hung_wait_candidate",
                     thread["threadId"], scheduler_references(interval["rawOrdinals"]), {"duration_ns": duration, "threshold_ns": threshold},
                     "observed scheduler interval; WAIT does not establish resource ownership or deadlock")
    for process in processes:
        parent = by_id.get(process.get("parentProcessId"))
        if not parent or parent.get("end_ns") is None:
            continue
        if process.get("end_ns") is None:
            # No finite right edge means no observed survival duration claim.
            continue
        if process["end_ns"] > parent["end_ns"]:
            evidence = parent["rawReferences"] + process["rawReferences"]
            emit("windows.process.orphan_child_candidate", process["processId"], evidence,
                 {"survived_parent_ns": process["end_ns"] - parent["end_ns"]}, "OS parent lifetime only; object-owner semantics unknown")
            for thread in graph["threads"]:
                if thread.get("processId") != process["processId"]:
                    continue
                for interval in thread["intervals"]:
                    if interval["state"] == "RUN" and interval["end_ns"] > parent["end_ns"]:
                        duration = interval["end_ns"] - max(interval["start_ns"], parent["end_ns"])
                        emit("windows.process.work_after_parent_exit_candidate", thread["threadId"], evidence + scheduler_references(interval["rawOrdinals"]),
                             {"observed_run_after_parent_exit_ns": duration}, "RUN interval after OS parent exit; no semantic ownership inferred")
                        break
    for action in actions:
        process = by_id.get(action["processId"])
        if action.get("observation_kind") != "external_control_request" or not action.get("correlationEvidence"):
            raise EvidenceError("cooperative control requires externally observed request and explicit target correlation")
        if action.get("kind") not in {"cancel", "complete"}:
            raise EvidenceError("unsupported external control kind")
        if not process or process.get("end_ns") is None:
            continue
        start, deadline, ended = action["request_ns"], action["deadline_ns"], process["end_ns"]
        if not start <= deadline or ended < start:
            raise EvidenceError("invalid control/target clock ordering")
        if ended > deadline:
            emit("windows.control.cancellation_latency_candidate" if action["kind"] == "cancel" else "windows.control.late_completion_candidate",
                 process["processId"], action["rawReferences"] + action["correlationEvidence"] + process["rawReferences"],
                 {"request_to_process_end_ns": ended - start, "deadline_overrun_ns": ended - deadline},
                 "process-end response to external target control; no function-level completion or causal DAG inferred")
    observed = {}
    for snapshot in wait_snapshots:
        if snapshot.get("provenance") != "external" or not snapshot.get("rawReferences"):
            raise EvidenceError("wait ownership needs external snapshot evidence")
        resource, owner = snapshot.get("resourceId"), snapshot.get("ownerThreadId")
        if not snapshot.get("available") or resource is None or owner is None:
            continue
        key = (resource, owner)
        prior = observed.get(key)
        if prior and snapshot["timestamp_ns"] < prior["timestamp_ns"]:
            raise EvidenceError("wait snapshots reversed")
        if prior and snapshot["timestamp_ns"] - prior["timestamp_ns"] >= owner_threshold_ns:
            emit("windows.lock.long_owner_candidate", owner, prior["rawReferences"] + snapshot["rawReferences"],
                 {"snapshot_separation_ns": snapshot["timestamp_ns"] - prior["timestamp_ns"]},
                 "same resource/owner at separate WCT snapshots; continuous hold duration unproven")
        else:
            observed.setdefault(key, snapshot)
        if len(set(snapshot.get("waiterThreadIds", []))) > 1 and snapshot.get("cycle") is False:
            emit("windows.lock.contention_without_observed_deadlock_candidate", resource, snapshot["rawReferences"],
                 {"observed_waiters": len(set(snapshot["waiterThreadIds"]))}, "WCT snapshot reports no cycle; future deadlock freedom unproven")
    return findings
