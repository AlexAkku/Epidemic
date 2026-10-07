"""ETW scheduler intervals; no call tree or cross-thread causal inference."""
from collections import Counter
import hashlib

from ep_auditor.collectors.xperf import process_id
from ep_auditor.core.evidence import EvidenceError


def project_monotonic_time(timestamp_ns, calibrations, trace_origin_utc_ns, *, max_offset_spread_ns=2000000):
    """Project a bracketed external perf timestamp to an ETL-relative interval.

    Bounds retain measured UTC/monotonic offset spread. They do not prove
    continuity between observations; callers must corroborate trace identities.
    """
    if (type(timestamp_ns) is not int or type(trace_origin_utc_ns) is not int
            or type(max_offset_spread_ns) is not int or max_offset_spread_ns <= 0 or len(calibrations) < 2):
        raise EvidenceError("bounded clock projection requires two external anchors")
    bounds, previous = [], None
    for anchor in calibrations:
        begin, end, utc = (anchor.get(name) for name in ("perf_begin_ns", "perf_end_ns", "utc_unix_ns"))
        if (anchor.get("provenance") != "external" or anchor.get("confidence") != "observed"
                or any(type(value) is not int for value in (begin, end, utc)) or begin > end
                or (previous is not None and begin < previous)):
            raise EvidenceError("external clock anchors malformed or reversed")
        previous = end
        bounds.append((utc - end - 100, utc - begin + 100))
    if not calibrations[0]["perf_begin_ns"] <= timestamp_ns <= calibrations[-1]["perf_end_ns"]:
        raise EvidenceError("external time outside calibrated interval")
    lower, upper = min(value[0] for value in bounds), max(value[1] for value in bounds)
    if upper - lower > max_offset_spread_ns:
        raise EvidenceError("clock drift or calibration uncertainty exceeds declared bound")
    result = {"lower_ns": timestamp_ns + lower - trace_origin_utc_ns,
              "upper_ns": timestamp_ns + upper - trace_origin_utc_ns,
              "clock_domain": "etl_relative_trace_ns", "confidence": "derived",
              "scope": "observed calibration envelope; corroboration required, no unobserved continuity claim"}
    if result["lower_ns"] < 0:
        raise EvidenceError("external observation precedes trace origin")
    return result


def process_timeline(events, identity_namespace):
    active, processes = {}, []
    for event in events:
        kind, timestamp, fields = event["kind"], event["timestamp_ns"], event["fields"]
        if kind not in {"P-Start", "P-DCStart", "P-End", "P-DCEnd"}:
            continue
        pid = process_id(fields["Process Name ( PID)"])
        if kind in {"P-Start", "P-DCStart"}:
            if pid in active:
                raise EvidenceError("overlapping OS process lifetimes")
            identity = hashlib.sha256(f"{identity_namespace}:{pid}:{timestamp}:{fields.get('UniqueKey', '')}".encode()).hexdigest()
            process = {"processId": "ProcessId:" + identity, "pid": pid,
                       "ppid": int(fields["ParentPID"]) if fields.get("ParentPID") else None,
                       "parentProcessId": None, "creation_ns": timestamp if kind == "P-Start" else None,
                       "first_observation_ns": timestamp, "end_ns": None, "exit_code": None,
                       "command_line": fields.get("CommandLine"), "unique_key": fields.get("UniqueKey"),
                       "cwd": None, "environment": None, "metadata_scope": "cwd/environment unavailable from these ETW events",
                       "rawOrdinals": [event["rawOrdinal"]], "provenance": "external", "confidence": "derived"}
            active[pid] = process
            processes.append(process)
        elif pid in active:
            process = active[pid]
            if timestamp < process["first_observation_ns"] or (process["unique_key"] and fields.get("UniqueKey")
                    and process["unique_key"] != fields["UniqueKey"]):
                raise EvidenceError("ETW process-end identity or clock mismatch")
            # DCEnd is a rundown observation, not evidence of process termination.
            process["rawOrdinals"].append(event["rawOrdinal"])
            if kind == "P-End":
                active.pop(pid)
                process["end_ns"] = timestamp
                process["exit_code"] = int(fields["Status"], 0) if fields.get("Status") else None
    for process in processes:
        if process["creation_ns"] is None:
            continue
        parents = [parent for parent in processes if parent["pid"] == process["ppid"]
                   and parent["first_observation_ns"] <= process["creation_ns"]
                   and (parent["end_ns"] is None or parent["end_ns"] > process["creation_ns"])]
        if len(parents) > 1:
            raise EvidenceError("ambiguous observed process parent lifetime")
        if parents:
            process["parentProcessId"] = parents[0]["processId"]
    return processes


def thread_timeline(events, *, identity_namespace="unscoped"):
    if not isinstance(identity_namespace, str) or not identity_namespace:
        raise EvidenceError("trace identity namespace required")
    events = list(events)
    processes = process_timeline(events, identity_namespace)
    active, threads, samples = {}, [], []
    anchors, stacks, ambiguous, scheduler_anchors = {}, {}, set(), set()
    blocked_anchors, trigger_counts = {}, Counter()

    def finish(thread, timestamp, evidence, terminal=False):
        state = thread.pop("pending_state", None)
        if state is not None:
            name, start, origin = state
            if timestamp < start:
                raise EvidenceError("ETW scheduler interval reversed")
            thread["intervals"].append({"state": name, "start_ns": start, "end_ns": timestamp,
                "terminated": terminal, "rawOrdinals": [origin, evidence], "confidence": "derived"})

    for event in events:
        kind, timestamp, fields = event["kind"], event["timestamp_ns"], event["fields"]
        ordinal = event["rawOrdinal"]
        if kind in {"T-Start", "T-DCStart"}:
            tid = int(fields["ThreadID"])
            if tid in active:
                raise EvidenceError("overlapping OS thread lifetimes")
            pid = process_id(fields["Process Name ( PID)"])
            identity = hashlib.sha256(f"{identity_namespace}:{pid}:{tid}:{timestamp}".encode()).hexdigest()
            observed_processes = [process for process in processes if process["pid"] == pid
                                  and process["first_observation_ns"] <= timestamp
                                  and (process["end_ns"] is None or process["end_ns"] >= timestamp)]
            if len(observed_processes) > 1:
                raise EvidenceError("ambiguous thread process lifetime")
            thread = {"threadId": "ThreadId:" + identity, "os_tid": tid, "pid": pid,
                      "processId": observed_processes[0]["processId"] if observed_processes else None,
                      "creation_ns": timestamp if kind == "T-Start" else None, "first_observation_ns": timestamp,
                      "end_ns": None, "lifecycle_rawOrdinals": [ordinal], "intervals": [], "open_interval": None,
                      "provenance": "external", "confidence": "derived"}
            active[tid] = thread
            threads.append(thread)
        elif kind == "CSwitch":
            old, new = int(fields["Old TID"]), int(fields["New TID"])
            trigger_counts[(old, timestamp)] += 1
            for tid in (old, new):
                key = (tid, timestamp)
                if key in anchors:
                    ambiguous.add(key)
                scheduler_anchors.add(key)
            if old in active:
                thread = active[old]
                finish(thread, timestamp, ordinal)
                state = fields["OldState"]
                if state == "Waiting":
                    thread["pending_state"] = ("WAIT", timestamp, ordinal)
                    blocked_anchors[ordinal] = {"key": (old, timestamp), "reason": fields.get("OldWaitReason")}
                elif state in {"Ready", "DeferredReady", "Standby"}:
                    thread["pending_state"] = ("READY", timestamp, ordinal)
            if new in active:
                thread = active[new]
                finish(thread, timestamp, ordinal)
                thread["pending_state"] = ("RUN", timestamp, ordinal)
        elif kind == "ReadyThread":
            # ReadyThread stacks belong to the readying thread, not the target.
            if "ThreadID" in fields:
                key = (int(fields["ThreadID"]), timestamp)
                trigger_counts[key] += 1
                if key in anchors:
                    ambiguous.add(key)
                scheduler_anchors.add(key)
            tid = int(fields["Rdy TID"])
            if tid in active:
                thread = active[tid]
                state = thread.get("pending_state")
                if state is None or state[0] == "WAIT":
                    finish(thread, timestamp, ordinal)
                    thread["pending_state"] = ("READY", timestamp, ordinal)
        elif kind == "T-End":
            tid = int(fields["ThreadID"])
            if tid in active:
                thread = active.pop(tid)
                finish(thread, timestamp, ordinal, terminal=True)
                thread["end_ns"] = timestamp
                thread["lifecycle_rawOrdinals"].append(ordinal)
        elif kind == "SampledProfile":
            tid = int(fields["ThreadID"])
            key = (tid, timestamp)
            trigger_counts[key] += 1
            if key in anchors or key in scheduler_anchors:
                ambiguous.add(key)
            thread = active.get(tid)
            sample = {"threadId": thread["threadId"] if thread else None, "os_tid": tid,
                      "timestamp_ns": timestamp, "address": fields["PrgrmCtr"], "weight": int(fields["Count"]),
                      "rawOrdinal": ordinal, "frames": [], "stack_status": "unavailable"}
            if sample["weight"] <= 0:
                raise EvidenceError("invalid CPU sample weight")
            anchors[key] = sample
            samples.append(sample)
        elif kind == "Stack":
            key = (int(fields["ThreadID"]), timestamp)
            stacks.setdefault(key, []).append({"index": int(fields["No."]), "address": fields["Address"],
                                              "function": fields["Image!Function"], "rawOrdinal": ordinal})
    for thread in active.values():
        state = thread.pop("pending_state", None)
        if state:
            name, start, ordinal = state
            thread["open_interval"] = {"state": name, "start_ns": start, "end_ns": None,
                                       "rawOrdinals": [ordinal], "confidence": "derived"}
    for key, sample in anchors.items():
        frames = stacks.get(key, [])
        indices = [frame["index"] for frame in frames]
        if key in ambiguous or len(indices) != len(set(indices)):
            sample["stack_status"] = "ambiguous"
        elif frames and sample["threadId"] is not None:
            sample["frames"] = sorted(frames, key=lambda frame: frame["index"])
            sample["stack_status"] = "observed_frames"
    folded = Counter()
    for sample in samples:
        if sample["stack_status"] == "observed_frames":
            names = [frame["function"] or frame["address"] for frame in reversed(sample["frames"])]
        else:
            names = ["unresolved@" + sample["address"]]
        names = [name.replace(";", "\\x3b").replace("\n", "\\x0a").replace("\r", "\\x0d") for name in names]
        folded[";".join(names)] += sample["weight"]
    waits = []
    for thread in threads:
        thread["observed_run_ns"] = sum(interval["end_ns"] - interval["start_ns"] for interval in thread["intervals"] if interval["state"] == "RUN")
        thread["run_measurement_scope"] = "lower_bound_from_observed_context_switch_pairs"
        intervals = thread["intervals"] + ([thread["open_interval"]] if thread["open_interval"] else [])
        for interval in intervals:
            if interval["state"] != "WAIT":
                continue
            origin = interval["rawOrdinals"][0]
            anchor = blocked_anchors.get(origin)
            frames = stacks.get(anchor["key"], []) if anchor else []
            indices = [frame["index"] for frame in frames]
            unique = bool(anchor) and trigger_counts[anchor["key"]] == 1 and len(indices) == len(set(indices))
            status = "observed_frames" if frames and unique else ("ambiguous" if frames else "unavailable")
            waits.append({"threadId": thread["threadId"], "start_ns": interval["start_ns"], "end_ns": interval["end_ns"],
                          "resourceId": None, "ownerThreadId": None, "wait_reason": anchor["reason"] if anchor else None,
                          "blocked_stack": sorted(frames, key=lambda frame: frame["index"]) if status == "observed_frames" else [],
                          "blocked_stack_status": status, "acquisition_stack": None,
                          "rawOrdinals": interval["rawOrdinals"], "provenance": "external", "confidence": "derived",
                          "scope": "scheduler blocking stack; resource owner and acquisition stack require independent observations"})
    return {"processes": processes, "threads": threads, "waits": waits, "samples": samples, "folded_stacks": dict(folded),
            "identity_namespace": identity_namespace, "identity_scope": "trace-local" if identity_namespace == "unscoped" else "caller-bound trace/run",
            "stack_correlation": "exact ETW timestamp and OS TID within observed thread lifetime; ambiguous anchors unavailable",
            "scope": "scheduler states only; no resource identity, lock owner or semantic cancellation inferred",
            "provenance": "external", "confidence": "derived"}
