"""One admitted backend for GUI and synchronous headless operations."""
from copy import deepcopy
import uuid

from ep_auditor.core.build import BuildController
from ep_auditor.core.lifecycle import mode_selection
from ep_auditor.core.storage import RunStorage
from ep_auditor.core.tests import TestCatalog, TestExecutor

OPERATIONS = frozenset({"configure", "build", "build-and-run", "discover-tests", "run-all-tests",
                        "run-one-test", "run-selected-tests", "run-group", "start-game", "stop", "reproduction-matrix", "stress"})


def admit(manager, operation_id, mode, test_ids, labels, entry_point):
    if operation_id not in OPERATIONS or operation_id == "stop":
        raise ValueError("unknown or inadmissible backend operation")
    test_ids = [] if test_ids is None else test_ids
    labels = [] if labels is None else labels
    if (not isinstance(test_ids, list) or any(not isinstance(value, str) or not value for value in test_ids)
            or len(set(test_ids)) != len(test_ids) or not isinstance(labels, list)
            or any(not isinstance(value, str) or not value for value in labels) or len(set(labels)) != len(labels)):
        raise ValueError("invalid operation selection")
    if operation_id == "run-one-test" and len(test_ids) != 1:
        raise ValueError("one-test operation requires one TestId")
    if operation_id == "run-selected-tests" and not test_ids or operation_id == "run-group" and not labels:
        raise ValueError("selected/group operation requires explicit selection")
    if test_ids and operation_id not in {"run-one-test", "run-selected-tests"} or labels and operation_id != "run-group":
        raise ValueError("selection is not supported by this operation")
    with manager._operation_lock:
        if manager._closed or manager.busy:
            raise RuntimeError("Auditor is closed or busy")
        snapshot = deepcopy(manager.config).for_mode(mode)
        if operation_id == 'stress':
            from ep_auditor.core.run_manager import validate_stress
            snapshot.data['stress'] = validate_stress(snapshot.data.get('stress', {}))
        mode_selection(mode, snapshot.data.get("capture", {}).get("target_scope"))
        parallelisms = snapshot.data.get("cmake", {}).get("reproduction_parallelisms", [1, 2])
        if operation_id == "reproduction-matrix" and (not isinstance(parallelisms, list)
                or not 2 <= len(parallelisms) <= 10
                or any(type(value) is not int or not 1 <= value <= 10 for value in parallelisms)
                or len(set(parallelisms)) != len(parallelisms)):
            raise ValueError("reproduction matrix requires 2–10 distinct parallelisms in 1–10")
        request = {"operation_id": operation_id, "invocation_id": str(uuid.uuid4()), "entry_point": entry_point,
                   "capture_mode": mode, "test_ids": list(test_ids), "labels": list(labels)}
        if operation_id == "reproduction-matrix":
            request["parallelisms"] = list(parallelisms)
        snapshot.backend_operation = deepcopy(request)
        manager._cancel_requested.clear()
        manager._cancel_reason = None
        manager.runner._pending_stop_reason = None
        manager._busy.set()
    manager.events.emit("busy", busy=True, title=operation_id, **request)
    return snapshot, request


def perform(manager, admitted):
    snapshot, request = admitted
    original = (manager.config, manager.storage, manager.builder, manager.catalog, manager.tests, manager.runner.config)
    try:
        manager.config = snapshot
        manager.storage = RunStorage(snapshot)
        manager.runner.config = snapshot
        manager.builder = BuildController(snapshot, manager.runner)
        manager.catalog = TestCatalog(snapshot, manager.runner)
        manager.tests = TestExecutor(snapshot, manager.runner, manager.storage)
        if manager._cancel_requested.is_set():
            raise RuntimeError("operation cancelled before launch")
        snapshot.admit_project()
        operation, mode = request["operation_id"], request["capture_mode"]
        if operation in {"configure", "build"}:
            manager._run_process_operation(operation, operation, mode, getattr(manager.builder, operation))
        elif operation == "build-and-run":
            manager._build_and_run(mode)
        elif operation == "start-game":
            manager._start_game(mode)
        elif operation == "discover-tests":
            manager._discover_tests(mode)
        elif operation == 'stress':
            manager._run_stress(mode)
        elif operation == "reproduction-matrix":
            for index, parallelism in enumerate(request["parallelisms"]):
                if manager._cancel_requested.is_set():
                    break
                snapshot.data["cmake"]["parallel"] = parallelism
                snapshot.backend_operation["matrix_attempt"] = {"index": index, "parallelism": parallelism}
                manager._run_process_operation("build", "Reproduction matrix parallelism " + str(parallelism),
                                               mode, manager.builder.build)
        else:
            tests = manager.catalog.discover()
            manager.events.emit("tests_discovered", tests=[test.to_dict() for test in tests])
            if operation in {"run-one-test", "run-selected-tests"}:
                by_id = {test.test_id: test for test in tests}
                if any(identity not in by_id for identity in request["test_ids"]):
                    raise ValueError("selected TestId is absent from current profile/catalog")
                tests = [by_id[identity] for identity in request["test_ids"]]
            elif operation == "run-group":
                tests = [test for test in tests if any(label in test.labels or label == test.group for label in request["labels"])]
                if not tests:
                    raise ValueError("requested group has no tests in current catalog")
            manager._run_tests(tests, mode, operation)
        return deepcopy(request)
    finally:
        current_catalog = manager.catalog
        manager.config, manager.storage, manager.builder, manager.catalog, manager.tests, manager.runner.config = original
        manager.catalog.tests = current_catalog.tests
        with manager._operation_lock:
            manager._busy.clear()
        manager.events.emit("busy", busy=False, title=request["operation_id"], **request)
        manager.events.emit("runs_changed")
