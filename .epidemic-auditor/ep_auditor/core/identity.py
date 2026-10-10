"""Auditor-owned stable identities; no engine or repository tooling dependency."""
import hashlib
import json
from pathlib import Path

from .evidence import EvidenceError


def module_identity(binary_sha256: str) -> str:
    if (not isinstance(binary_sha256, str) or len(binary_sha256) != 64
            or any(character not in "0123456789abcdef" for character in binary_sha256)):
        raise EvidenceError("module identity requires exact binary fingerprint")
    return "ModuleId:" + binary_sha256


def function_identity(binary_sha256: str, function_rva: int) -> str:
    module = module_identity(binary_sha256)
    if type(function_rva) is not int or not 0 <= function_rva < 2**64:
        raise EvidenceError("function identity requires module-relative address")
    return "FunctionId:" + hashlib.sha256(f"{module}:{function_rva}".encode()).hexdigest()


def test_identity(name: str, executable_identity: str, arguments: list[str]) -> str:
    if not name or not executable_identity or any(not isinstance(value, str) for value in arguments):
        raise EvidenceError("incomplete CTest identity")
    identity = {"ctest_name": name, "executable_semantic_identity": executable_identity, "arguments": arguments}
    encoded = json.dumps(identity, sort_keys=True, ensure_ascii=False, separators=(",", ":"), allow_nan=False).encode()
    return "TestId:" + hashlib.sha256(encoded).hexdigest()


def semantic_command(command: list[str], project: Path, build: Path, configuration: str) -> tuple[str, list[str]]:
    def stable(value: str) -> str:
        value = value.replace("\\", "/")
        for root, tag in ((build, "${build}"), (project, "${source}")):
            prefix = root.as_posix().rstrip("/")
            value = value.replace(prefix + "/", tag + "/")
            if value == prefix:
                value = tag
        return value
    executable = stable(command[0])
    if executable.startswith("${build}/"):
        executable = executable.replace("/" + configuration + "/", "/")
    elif not executable.startswith("${source}/"):
        executable = "tool:" + Path(command[0]).stem.lower()
    return executable, [stable(value) for value in command[1:]]
