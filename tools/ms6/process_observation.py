"""Independent success oracle retaining the distinction between cleanup and Stop."""


def require_successful_process(lifecycle):
    if lifecycle["exit_code"] != 0 or lifecycle.get("cancelled") is not False:
        raise RuntimeError("process success lacks zero exit and explicit non-cancellation")
    termination = lifecycle["termination"]
    if termination is not None:
        if (termination["reason"] != "root_exit_live_descendants"
                or termination["purpose"] != "scope_cleanup"
                or termination["method"] != "TerminateJobObject"
                or type(termination["active_processes_before"]) is not int
                or termination["active_processes_before"] < 1
                or termination["provenance"] != "external"
                or termination["confidence"] != "observed"):
            raise RuntimeError("successful root contains cancellation or unproved descendant cleanup")
    if not lifecycle["creation_filetime_100ns"] or not lifecycle["image_sha256"]:
        raise RuntimeError("process physical identity absent")
