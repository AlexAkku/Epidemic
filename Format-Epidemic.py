#!/usr/bin/env python3
"""Repository-wide conservative clang-format + one empty line after closing brace.

Run from the repository root, using Format-Epidemic.ps1.
Only git-tracked C/C++ files are considered. No include sorting or source parsing edits.
"""
from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

EXTENSIONS = {".cpp", ".hpp", ".h", ".c", ".cc", ".cxx", ".hh", ".hxx", ".inl", ".ipp"}
IGNORED_PARTS = {
    ".git", "build", "out", "vendor", "third_party", "thirdparty", "external", "temp",
    ".vs", "node_modules", "generated", "_deps", "cmakefiles",
}
RAW_START = re.compile(r'(?:u8|u|U|L)?R"([^ ()\\\t\r\n]{0,16})\(')
CLOSING_END = re.compile(r'}\s*;?\s*$')


def run(command: list[str], cwd: Path, **kwargs: object) -> subprocess.CompletedProcess[bytes]:
    return subprocess.run(command, cwd=cwd, check=True, stdout=subprocess.PIPE,
                          stderr=subprocess.PIPE, **kwargs)


def tracked_sources(root: Path) -> list[Path]:
    output = run(["git", "ls-files", "-z", "--cached"], root).stdout
    results = []
    for part in output.split(b"\0"):
        if not part:
            continue
        relative = Path(os.fsdecode(part))
        if relative.suffix.lower() not in EXTENSIONS:
            continue
        if any(s.lower() in IGNORED_PARTS for s in relative.parts[:-1]):
            continue
        path = root / relative
        if path.is_file() and not path.is_symlink():
            results.append(path)
    return sorted(results, key=lambda p: str(p).lower())


def current_changes(root: Path) -> set[str]:
    # Both staged and unstaged modifications are protected. Untracked code is never touched.
    changes = set()
    for cmd in (["git", "diff", "--name-only", "-z"],
                ["git", "diff", "--cached", "--name-only", "-z"]):
        for item in run(cmd, root).stdout.split(b"\0"):
            if item:
                changes.add(os.path.normcase(os.fsdecode(item).replace("/", os.sep)))
    return changes


def code_before_line_comment(line: str, state: dict[str, object]) -> str:
    """Return only real code tokens, preserving raw/block-comment/string state across lines.

    Strings are replaced by spaces, so a brace inside a literal is not mistaken
    for a C++ closing brace. This deliberately avoids altering raw literals.
    """
    code = []
    i = 0
    while i < len(line):
        raw_end = state["raw_end"]
        if raw_end is not None:
            end = line.find(str(raw_end), i)
            if end == -1:
                return "".join(code)
            i = end + len(str(raw_end))
            state["raw_end"] = None
            continue
        if state["block_comment"]:
            end = line.find("*/", i)
            if end == -1:
                return "".join(code)
            i = end + 2
            state["block_comment"] = False
            continue
        quote = state["quote"]
        if quote is not None:
            while i < len(line):
                if line[i] == "\\":
                    i += 2
                elif line[i] == quote:
                    i += 1
                    state["quote"] = None
                    break
                else:
                    i += 1
            continue
        if line.startswith("//", i):
            break
        if line.startswith("/*", i):
            state["block_comment"] = True
            i += 2
            continue
        raw = RAW_START.match(line, i)
        if raw and (i == 0 or not (line[i - 1].isalnum() or line[i - 1] == "_")):
            state["raw_end"] = ")" + raw.group(1) + '"'
            i = raw.end()
            continue
        if line[i] in {'"', "'"}:
            state["quote"] = line[i]
            i += 1
            continue
        code.append(line[i])
        i += 1
    # Normal C++ quoted literals continue across lines only with a trailing backslash.
    if state["quote"] is not None and not line.endswith("\\"):
        state["quote"] = None
    return "".join(code)


def add_blank_after_closing_braces(text: str) -> str:
    """Make exactly one empty line after a line ending in `}` or `};`.

    Skip #define continuations, formatting-disabled sections, and braces within
    raw strings/comments. This is a whitespace-only transformation.
    """
    lines = text.split("\n")
    state: dict[str, object] = {"raw_end": None, "block_comment": False, "quote": None}
    disabled = False
    directive_continuation = False
    result: list[str] = []
    for index, line in enumerate(lines):
        stripped = line.lstrip()
        is_directive = directive_continuation or stripped.startswith("#")
        directive_continuation = is_directive and line.rstrip().endswith("\\")

        if "clang-format off" in line:
            disabled = True
        actual_code = code_before_line_comment(line, state)
        result.append(line)
        if "clang-format on" in line:
            disabled = False
        if disabled or is_directive or index + 1 >= len(lines):
            continue
        if not CLOSING_END.search(actual_code):
            continue
        # Do not double blank lines or add trailing whitespace to the file.
        if not lines[index + 1].strip():
            continue
        result.append("")
    return "\n".join(result)


def newline_style(data: bytes, path: Path) -> bytes:
    no_crlf = data.replace(b"\r\n", b"")
    if b"\r" in no_crlf:
        raise ValueError(f"Unsupported standalone CR newline: {path}")
    if b"\r\n" in data and b"\n" in no_crlf:
        raise ValueError(f"Mixed LF/CRLF newlines: {path}")
    return b"\r\n" if b"\r\n" in data else b"\n"


def formatted_bytes(path: Path, root: Path) -> bytes:
    original = path.read_bytes()
    style = newline_style(original, path)
    bom = original.startswith(b"\xef\xbb\xbf")
    # Validate encoding before touching the repository.
    original.decode("utf-8-sig")
    completed = run(["clang-format", "--style=file", "--assume-filename=" + str(path)],
                    root, input=original)
    after = completed.stdout.decode("utf-8-sig").replace("\r\n", "\n").replace("\r", "\n")
    after = add_blank_after_closing_braces(after)
    payload = after.encode("utf-8")
    if style == b"\r\n":
        payload = payload.replace(b"\n", b"\r\n")
    if bom:
        payload = b"\xef\xbb\xbf" + payload
    return payload


def self_test() -> None:
    example = ('int main() {\n'
               '  if (true) {\n'
               '    f(1, 2);\n'
               '  }\n'
               '  g();\n'
               '}\n'
               'int next();\n'
               'const char* sample = R"tag(\n}\n)tag";\n'
               '#define MACRO(x) \\\n  if (x) { \\\n  }\n'
               'int end();\n')
    result = add_blank_after_closing_braces(example)
    assert "  }\n\n  g();" in result
    assert "}\n\nint next();" in result
    assert 'R"tag(\n}\n)tag";' in result
    assert "  }\n\nint end();" not in result  # Macro continuation remains protected.
    assert add_blank_after_closing_braces(result) == result
    print("Blank-line lexer self-test: PASS")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--check", action="store_true", help="preview, write nothing")
    mode.add_argument("--apply", action="store_true", help="format all eligible tracked sources")
    mode.add_argument("--self-test", action="store_true", help="test lexer only")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0
    root = Path(__file__).resolve().parent
    if not (root / ".clang-format").is_file():
        parser.error("Place the formatter and .clang-format in the git repository root")
    if not (root / ".git").exists():
        parser.error("Repository .git directory not found")
    if not shutil.which("clang-format"):
        parser.error("clang-format executable not on PATH")
    version = run(["clang-format", "--version"], root).stdout.decode("utf-8", "replace")
    match = re.search(r"version\s+(\d+)", version)
    if not match or int(match.group(1)) < 14:
        parser.error("clang-format >= 14 required: " + version.strip())
    sources = tracked_sources(root)
    if args.apply:
        changed = current_changes(root)
        dirty = [str(p.relative_to(root)) for p in sources
                 if os.path.normcase(str(p.relative_to(root))) in changed]
        if dirty:
            print("ABORT: commit or otherwise preserve current tracked C/C++ changes first:",
                  *dirty[:20], sep="\n  ", file=sys.stderr)
            return 2
    updates: list[tuple[Path, bytes]] = []
    for number, path in enumerate(sources, 1):
        try:
            desired = formatted_bytes(path, root)
        except (OSError, UnicodeError, ValueError, subprocess.CalledProcessError) as err:
            print(f"ABORT before writing, file: {path.relative_to(root)}: {err}", file=sys.stderr)
            return 3
        if desired != path.read_bytes():
            updates.append((path, desired))
        if number % 100 == 0:
            print(f"Inspected {number}/{len(sources)} files")
    print(f"Eligible files: {len(sources)}; files with formatting changes: {len(updates)}")
    for path, _ in updates[:20]:
        print("  ", path.relative_to(root))
    if len(updates) > 20:
        print(f"  ... and {len(updates) - 20} more")
    if args.apply:
        for path, payload in updates:
            path.write_bytes(payload)
        print("Applied. Review 'git diff --check' and build/test before committing.")
    else:
        print("Preview only. No files changed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
