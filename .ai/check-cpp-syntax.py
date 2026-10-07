"""Syntax-only preflight of one TU using the actual compilation database."""
import argparse
import ctypes
import json
import os
from pathlib import Path
import shlex
import subprocess


def split_command(command):
    if os.name != 'nt':
        return shlex.split(command)
    count = ctypes.c_int()
    split = ctypes.windll.shell32.CommandLineToArgvW
    split.argtypes = (ctypes.c_wchar_p, ctypes.POINTER(ctypes.c_int))
    split.restype = ctypes.POINTER(ctypes.c_wchar_p)
    native = split(command, ctypes.byref(count))
    if not native:
        raise OSError('CommandLineToArgvW failed')
    try:
        return [native[i] for i in range(count.value)]
    finally:
        free = ctypes.windll.kernel32.LocalFree
        free.argtypes = (ctypes.c_void_p,)
        free(native)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('source')
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    source = (root / args.source).resolve()
    if not source.is_relative_to(root):
        parser.error('Source must remain inside the project')
    entries = json.loads((root / 'compile_commands.json').read_text(encoding='utf-8'))
    selected = [entry for entry in entries if Path(entry['file']).resolve() == source]
    if len(selected) != 1:
        parser.error(f'Expected exactly one command, found {len(selected)}')
    entry = selected[0]
    arguments = entry.get('arguments') or split_command(entry['command'])
    cleaned = [arguments[0]]
    skip = False
    for argument in arguments[1:]:
        if skip:
            skip = False
            continue
        if argument in ('-o', '-MF', '-MT', '-MQ'):
            skip = True
            continue
        if argument in ('-c', '-MD', '-MMD'):
            continue
        cleaned.append(argument)
    cleaned.append('-fsyntax-only')
    result = subprocess.run(cleaned, cwd=entry['directory'], timeout=60)
    if result.returncode == 0:
        print(f'PASS: syntax-only {args.source}; no objects, link or tests')
    return result.returncode


if __name__ == '__main__':
    raise SystemExit(main())
