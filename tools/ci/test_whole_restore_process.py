"""Reopen a production whole-engine generation in a new executable process."""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('executable')
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='epidemic-whole-restore-') as directory:
        package = Path(directory) / 'generation.bin'
        fingerprints = []
        for mode in ('--write', '--read'):
            result = subprocess.run([args.executable, mode, str(package)],
                                    capture_output=True, text=True, timeout=150)
            if result.returncode:
                raise RuntimeError(f'{mode}: exit={result.returncode}\n{result.stdout}\n{result.stderr}')
            found = re.findall(r'^WHOLE_FINGERPRINT ([0-9]+)$', result.stdout, re.MULTILINE)
            if len(found) != 1:
                raise RuntimeError(f'{mode}: missing/duplicate semantic continuation fingerprint')
            fingerprints.append(found[0])
        if fingerprints[0] != fingerprints[1]:
            raise RuntimeError(f'fresh-process continuation differs: expected={fingerprints[0]}, actual={fingerprints[1]}')
    print('PASS: fresh writer/restore processes preserve representative whole-engine continuation.')


if __name__ == '__main__':
    main()
