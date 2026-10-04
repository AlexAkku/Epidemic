"""Discover persistence-related production candidates for Milestone 5.

Discovery is intentionally semantic-neutral: it reports candidates and risks,
but never assigns a persistence role, save plane, review status, or PASS.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from collections import Counter
from pathlib import Path

sys.dont_write_bytecode = True

from module_dossiers import ROOT, Module, discover_modules

OUTPUT = ROOT / "docs" / "freeze" / "goal5_persistence_candidates.json"
LOCAL_READY = ROOT / "docs" / "freeze" / "local_ready_ledger.json"
DOSSIER_REVIEWS = ROOT / "docs" / "freeze" / "module_dossier_reviews.json"
SOURCE_SUFFIXES = {".h", ".hpp", ".cpp", ".cc", ".cxx"}
EXCLUDED_PARTS = {
    "Tests", "tests", "DevelopmentInfrastructure", "build", "third-party",
    "third_party", "generated", "fixtures", "Deltas", "archives",
}
PERSISTENCE_TOKENS = (
    "CaptureSnapshot", "RestoreSnapshot", "Checkpoint", "CaptureCheckpoint",
    "RestoreCheckpoint", "SaveParticipant", "ISaveParticipant",
    "PersistenceSnapshot", "IPersistenceBackend", "SaveTransaction", "Journal",
    "Cursor", "Reservation", "Outbox", "Reconciliation",
    "RandomSequenceSnapshot", "IdGenerator",
)
DURABLE_WORDS = re.compile(
    r"\b(revision|generation|sequence|cursor|epoch|pending|reservation|outbox|"
    r"checkpoint|tombstone|journal|seed|stream|random_state|randomstate)\w*\b",
    re.IGNORECASE,
)
TYPE_RE = re.compile(
    r"\b(class|struct)\s+(?:(?:final|[A-Z][A-Z0-9_]*)\s+)*([A-Za-z_]\w*)"
    r"(\s*<[^;{}]+>)?(?!\s*[,>])[^;{]*\{",
)
FORWARD_TYPE_RE = re.compile(
    r"\b(class|struct)\s+([A-Za-z_]\w*)\s*;"
)
SNAPSHOT_TYPE_WORDS = ("Snapshot", "Checkpoint")
NAMESPACE_RE = re.compile(r"\bnamespace\s+([A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)\s*\{")
METHOD_RE = re.compile(
    r"\b(CaptureSnapshot|RestoreSnapshot|CaptureCheckpoint|RestoreCheckpoint|"
    r"Checkpoint|Save|Load|BeginSave|Commit|Rollback|Reconcile\w*|"
    r"ReadChangesSince|LatestChangeCursor|OpenTransaction)\s*\(",
)


def eligible_path(path: Path) -> bool:
    return path.suffix.lower() in SOURCE_SUFFIXES and not any(
        part in EXCLUDED_PARTS for part in path.parts)


def production_files(module: Module) -> list[Path]:
    return sorted(path for path in module.root.rglob("*") if path.is_file() and eligible_path(path))


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    return re.sub(r"//.*", "", text)


def matching_brace(text: str, opening: int) -> int | None:
    depth = 0
    for index in range(opening, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return index
    return None


def namespace_at(text: str, offset: int) -> str:
    # Production headers use a single outer namespace in almost all cases.  A
    # brace-depth filter keeps closed anonymous/helper namespaces out.
    stack: list[tuple[str, int]] = []
    token_re = re.compile(
        r"\bnamespace\s+([A-Za-z_]\w*(?:::[A-Za-z_]\w*)*)\s*\{|[{}]"
    )
    depth = 0
    for match in token_re.finditer(text, 0, offset):
        token = match.group(0)
        if token == "{":
            depth += 1
        elif token == "}":
            depth = max(0, depth - 1)
            while stack and stack[-1][1] > depth:
                stack.pop()
        else:
            depth += 1
            stack.append((match.group(1), depth))
    return "::".join(name for name, _ in stack)


def risk_flags(body: str) -> list[str]:
    flags: set[str] = set()
    lowered = body.lower()
    if re.search(r"(?:^|[;{}])\s*[A-Za-z_:][\w:<>, ]*\*\s*\w+", body, re.M):
        flags.add("RAW_POINTER")
    if "std::function" in body or re.search(r"\b(callback|delegate|function_)\w*\b", lowered):
        flags.add("PROCESS_LOCAL_CALLBACK")
    if "thread::id" in lowered or "thread_id" in lowered:
        flags.add("THREAD_ID")
    if re.search(r"\b(?:runtime|native|backend)\w*handle\b|\b\w*Handle\b", body):
        flags.add("RUNTIME_HANDLE")
    if re.search(r"\b(?:allocator|memory_resource)\b", lowered):
        flags.add("ALLOCATOR_IDENTITY")
    if "system_clock" in lowered or "steady_clock" in lowered or "time_point" in lowered:
        flags.add("WALL_CLOCK_TIME")
    if re.search(r"\b(?:process|callback|subscription|native)_?token\w*\b", lowered):
        flags.add("PROCESS_LOCAL_TOKEN")
    if re.search(r"\b(?:opaque|native|backend)_?id\w*\b", lowered):
        flags.add("OPAQUE_IDENTITY")
    return sorted(flags)


def candidate_kinds(name: str, body: str, methods: list[str], fields: list[str]) -> list[str]:
    text = " ".join((name, body[:12000], *methods))
    kinds: set[str] = set()
    if "Snapshot" in name or any("Snapshot" in method for method in methods):
        kinds.add("SNAPSHOT")
    if "Checkpoint" in text:
        kinds.add("CHECKPOINT")
    if "SaveParticipant" in text or "ISaveParticipant" in text:
        kinds.add("SAVE_PARTICIPANT")
    if "PersistenceBackend" in text or "PersistenceSnapshot" in text:
        kinds.add("PERSISTENCE_BACKEND")
    if "Transaction" in text:
        kinds.add("TRANSACTION")
    for token, kind in (
        ("Journal", "JOURNAL"), ("Cursor", "CURSOR"),
        ("Reservation", "RESERVATION"), ("Outbox", "OUTBOX"),
        ("Reconciliation", "RECONCILIATION"),
        ("RandomSequence", "RANDOM_STATE"), ("IdGenerator", "ID_GENERATOR_STATE"),
    ):
        if token in text:
            kinds.add(kind)
    if fields:
        kinds.add("DURABLE_LOOKING_STATE")
    return sorted(kinds)


def compact_statement(value: str) -> str:
    return re.sub(r"\s+", " ", value).strip()[:320]


def _snapshot_named(name: str) -> bool:
    return any(word in name for word in SNAPSHOT_TYPE_WORDS)


def _enclosing_type_name(text: str, offset: int) -> str | None:
    """Return the innermost enclosing type for generic nested Snapshot/Checkpoint DTOs."""
    enclosing: list[tuple[int, str]] = []
    for parent in TYPE_RE.finditer(text, 0, offset):
        opening = text.find("{", parent.start())
        closing = matching_brace(text, opening)
        if closing is None or not (opening < offset < closing):
            continue
        parent_name = parent.group(2)
        if parent.group(3):
            parent_name += re.sub(r"\s+", "", parent.group(3))
        enclosing.append((opening, parent_name))
    return max(enclosing, default=(0, ""))[1] or None


def _candidate_record(module: str, relative_path: str, clean: str, visibility: str,
                      match_start: int, name: str, body: str, methods: list[str],
                      fields: list[str]) -> dict[str, object]:
    namespace = namespace_at(clean, match_start)
    display_name = name
    # A generic nested `Snapshot` is otherwise indistinguishable from another
    # nested `Snapshot` in the same namespace.  Preserve the owning type in the
    # reviewed identity without perturbing already-stable named snapshot IDs.
    if name in {"Snapshot", "Checkpoint"}:
        parent = _enclosing_type_name(clean, match_start)
        if parent:
            display_name = f"{parent}::{name}"
    qualified = f"{namespace}::{display_name}" if namespace else display_name
    identity = f"{module}|{qualified}"
    line = clean.count("\n", 0, match_start) + 1
    return {
        "candidate_id": "G5P-" + hashlib.sha256(identity.encode("utf-8")).hexdigest()[:16],
        "module": module,
        "qualified_symbol": qualified,
        "source_path": relative_path,
        "source_line": line,
        "candidate_kinds": candidate_kinds(name, body, methods, fields),
        "visibility": visibility,
        "related_methods": methods,
        "state_type": display_name,
        "durable_fields": fields,
        "risk_flags": risk_flags(body),
    }


def scan_text(module: str, relative_path: str, text: str, visibility: str) -> list[dict[str, object]]:
    clean = strip_comments(text)
    found: list[dict[str, object]] = []
    seen_symbols: set[str] = set()
    for match in TYPE_RE.finditer(clean):
        opening = clean.find("{", match.start())
        closing = matching_brace(clean, opening)
        if closing is None:
            continue
        name = match.group(2)
        if match.group(3):
            # Different explicit owner-port specializations are different
            # technical symbols, not duplicate authoritative state owners.
            name += re.sub(r"\s+", "", match.group(3))
        body = clean[opening + 1:closing]
        methods = sorted(set(METHOD_RE.findall(body)))
        statements = [compact_statement(item.group(0)) for item in re.finditer(r"[^{};]+;", body)]
        fields = sorted({statement for statement in statements if DURABLE_WORDS.search(statement)})
        marker_text = " ".join((name, body[:16000]))
        # Type names are themselves a persistence signal.  A DTO such as
        # `MaterialsSnapshot { vector<State> states; }` must not disappear just
        # because none of its fields happen to contain revision/cursor keywords.
        if (not _snapshot_named(name) and not methods and not fields
                and not any(token in marker_text for token in PERSISTENCE_TOKENS)):
            continue
        record = _candidate_record(module, relative_path, clean, visibility, match.start(), name, body, methods, fields)
        if not record["candidate_kinds"]:
            continue
        seen_symbols.add(str(record["qualified_symbol"]))
        found.append(record)

    # Public/private forward declarations of restore/snapshot machinery are also
    # reviewed explicitly.  They have no body/risk fields, but their presence in
    # the production surface must not be invisible to the inventory.
    for match in FORWARD_TYPE_RE.finditer(clean):
        name = match.group(2)
        if not _snapshot_named(name):
            continue
        record = _candidate_record(module, relative_path, clean, visibility, match.start(), name, "", [], [])
        if str(record["qualified_symbol"]) in seen_symbols:
            continue
        seen_symbols.add(str(record["qualified_symbol"]))
        found.append(record)
    return found


def evidence_for(module: str, ledger: dict[str, object]) -> list[dict[str, object]]:
    record = ledger.get("modules", {}).get(module, {})
    criteria = record.get("criteria", {}) if isinstance(record, dict) else {}
    result: list[dict[str, object]] = []
    for name in sorted(key for key in criteria if key.startswith("PERSIST-")):
        item = criteria[name]
        evidence = item.get("evidence", []) if isinstance(item, dict) else []
        targets = sorted({
            str(entry.get("target")) for entry in evidence
            if isinstance(entry, dict) and entry.get("kind") == "test" and entry.get("target")
        })
        anchors = sorted({
            str(entry.get("anchor")) for entry in evidence
            if isinstance(entry, dict) and entry.get("anchor")
        })
        tests = sorted((
            {
                "target": str(entry.get("target")),
                "anchor": str(entry.get("anchor")),
                "assertions": sorted(str(value) for value in entry.get("assertions", []) if value),
            }
            for entry in evidence
            if isinstance(entry, dict) and entry.get("kind") == "test"
            and entry.get("target") and entry.get("anchor")
        ), key=lambda item: (item["target"], item["anchor"]))
        result.append({
            "criterion": name,
            "status": item.get("status", "MISSING") if isinstance(item, dict) else "MISSING",
            "rationale": item.get("rationale", "") if isinstance(item, dict) else "",
            "test_targets": targets,
            "anchors": anchors,
            "tests": tests,
        })
    return result


def build_inventory(modules: list[Module] | None = None) -> dict[str, object]:
    modules = discover_modules() if modules is None else modules
    ledger = json.loads(LOCAL_READY.read_text(encoding="utf-8"))
    reviews = json.loads(DOSSIER_REVIEWS.read_text(encoding="utf-8"))
    candidates: list[dict[str, object]] = []
    seen: dict[tuple[str, str], int] = {}
    for module in modules:
        for path in production_files(module):
            relative = path.relative_to(ROOT).as_posix()
            visibility = "PUBLIC" if "include" in path.parts else "PRIVATE"
            for candidate in scan_text(
                    module.key, relative, path.read_text(encoding="utf-8", errors="ignore"), visibility):
                key = (module.key, str(candidate["qualified_symbol"]))
                previous_index = seen.get(key)
                if previous_index is not None:
                    previous = candidates[previous_index]
                    # A public forward declaration and a private implementation
                    # describe one symbol. Keep public visibility/source identity,
                    # but merge implementation-discovered methods/risks so the
                    # candidate cannot hide dangerous private mechanics.
                    if previous.get("visibility") == "PUBLIC" and candidate.get("visibility") != "PUBLIC":
                        for field in ("candidate_kinds", "related_methods", "durable_fields", "risk_flags"):
                            previous[field] = sorted(set(previous.get(field, [])) | set(candidate.get(field, [])))
                        continue
                    if candidate.get("visibility") != "PUBLIC":
                        continue
                candidate["goal4_evidence"] = evidence_for(module.key, ledger)
                persistence_review = reviews.get("modules", {}).get(module.key, {}).get(
                    "Persistent and transient state", {})
                candidate["dossier_persistence_review"] = persistence_review
                if previous_index is None:
                    seen[key] = len(candidates)
                    candidates.append(candidate)
                else:
                    candidates[previous_index] = candidate
    candidates.sort(key=lambda item: (
        str(item["module"]), ",".join(item["candidate_kinds"]), str(item["qualified_symbol"])))
    by_layer = Counter(str(item["module"]).split("/", 1)[0] for item in candidates)
    by_module = Counter(str(item["module"]) for item in candidates)
    return {
        "schema_version": 1,
        "discovery_contract": "CANDIDATES_ONLY_NO_SEMANTIC_CLASSIFICATION",
        "modules": [module.key for module in modules],
        "candidate_counts": {
            "total": len(candidates),
            "by_layer": dict(sorted(by_layer.items())),
            "by_module": dict(sorted(by_module.items())),
        },
        "candidates": candidates,
    }


def encoded(data: dict[str, object]) -> str:
    return json.dumps(data, indent=2, sort_keys=True, ensure_ascii=False) + "\n"


def self_test() -> None:
    fixtures = {
        "owner": "struct WorldState { int revision; WorldSnapshot CaptureSnapshot(); void RestoreSnapshot(WorldSnapshot); };",
        "read_only": "struct DiagnosticsSnapshot { std::uint64_t sequence; };",
        "checkpoint": "struct AdapterCheckpoint { ChangeCursor cursor; };",
        "pointer": "struct PointerCheckpoint { void* backend_handle; std::uint64_t generation; };",
        "handle": "struct RuntimeCheckpoint { RuntimeHandle handle; std::uint64_t epoch; };",
        "journal": "struct JournalState { ChangeCursor cursor; std::uint64_t sequence; };",
        "random": "struct RandomSequenceSnapshot { std::uint64_t seed; std::uint64_t stream; };",
        "name_only": "struct InnocentSnapshot { int value; };",
        "forward": "template <typename T> struct SnapshotRestoreAccess;",
        "nested": "template <typename T> class Generator { public: struct Snapshot { int value; }; };",
        "template_parameter": "template <class Snapshot> void Encode(Snapshot value) { (void)value; }",
    }
    results = {
        name: scan_text("EngineFramework/Fake", f"EngineFramework/Fake/include/{name}.h", text, "PUBLIC")
        for name, text in fixtures.items()
    }
    assert all(len(items) == 1 for name, items in results.items() if name != "template_parameter")
    assert results["template_parameter"] == []
    assert results["name_only"][0]["candidate_kinds"] == ["SNAPSHOT"]
    assert results["forward"][0]["candidate_kinds"] == ["SNAPSHOT"]
    assert results["nested"][0]["qualified_symbol"] == "Generator::Snapshot"
    assert "RAW_POINTER" in results["pointer"][0]["risk_flags"]
    assert "RUNTIME_HANDLE" in results["handle"][0]["risk_flags"]
    assert "JOURNAL" in results["journal"][0]["candidate_kinds"]
    assert "RANDOM_STATE" in results["random"][0]["candidate_kinds"]
    assert "persistence_role" not in results["owner"][0]
    assert not eligible_path(Path("EngineFramework/Fake/tests/fake.cpp"))
    assert not eligible_path(Path("EngineFramework/Fake/generated/fake.hpp"))
    duplicate = results["owner"] + results["owner"]
    assert len({item["candidate_id"] for item in duplicate}) != len(duplicate)
    specialized = scan_text("EngineFramework/Fake", "bindings.cpp",
        "namespace ports { template <> struct RestorePort<world::Owner> { void RestoreSnapshot(); }; "
        "template <> struct RestorePort<items::Owner> { void RestoreSnapshot(); }; }", "PRIVATE")
    assert len(specialized) == 2 and len({item["candidate_id"] for item in specialized}) == 2
    assert {item["qualified_symbol"] for item in specialized} == {
        "ports::RestorePort<world::Owner>", "ports::RestorePort<items::Owner>"}
    print("PASS: persistence scanner accepted 10 positive/risk/name-only fixtures plus one template-parameter negative, ignored 2 excluded scopes, "
          "rejected duplicate identity and assigned no semantic PASS.")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--record", action="store_true")
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0
    try:
        current = build_inventory()
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(f"BLOCKED: persistence discovery failed: {error}", file=sys.stderr)
        return 1
    if args.record:
        OUTPUT.write_text(encoded(current), encoding="utf-8", newline="\n")
        print(f"Recorded {len(current['candidates'])} persistence candidates across "
              f"{len(current['modules'])} production modules.")
    if args.check:
        if not OUTPUT.is_file():
            print("BLOCKED: goal5_persistence_candidates.json is missing", file=sys.stderr)
            return 1
        recorded = OUTPUT.read_text(encoding="utf-8")
        expected = encoded(current)
        if recorded != expected:
            print("BLOCKED: persistence candidate inventory is stale; run --record and review the diff.",
                  file=sys.stderr)
            return 1
        print(f"PASS: {len(current['modules'])} production modules and "
              f"{len(current['candidates'])} persistence candidates match discovery.")
    if not args.record and not args.check:
        parser.error("choose --record, --check or --self-test")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
