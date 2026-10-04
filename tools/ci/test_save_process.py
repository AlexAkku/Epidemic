"""Fresh process write/reopen/restore and interrupted temp publication smoke."""
import argparse
import subprocess
import tempfile
from pathlib import Path

def main():
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument('executable'); parser.add_argument('--local',action='store_true'); parser.add_argument('--runtime-executable'); args=parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='epidemic-save-process-') as directory:
        if args.local:
            subprocess.run([args.executable,directory],check=True,timeout=120)
            return
        path=Path(directory)/'generation.save'
        def run(mode,ticks,expected=0):
            result=subprocess.run([args.executable,mode,str(path),str(ticks)],capture_output=True,text=True,timeout=90)
            if result.returncode!=expected:
                raise RuntimeError(f'{mode}: exit {result.returncode}, expected {expected}\n{result.stdout}\n{result.stderr}')
        run('write',42); run('read',42)
        baseline=path.read_bytes(); run('interrupt',73,75)
        if path.read_bytes()!=baseline: raise RuntimeError('interrupted pre-replace write changed old valid save')
        if len(list(Path(directory).glob('generation.save.tmp-*'))) != 1: raise RuntimeError('interruption did not exercise a real completed temp file')
        run('read',42); run('write',99); run('read',99)
        if args.runtime_executable:
            for mode in ('runtime-write','runtime-read'):
                subprocess.run([args.runtime_executable,mode,directory],check=True,timeout=90)
    scope='Framework Time and Runtime Time/Persistence' if args.runtime_executable else 'Framework Time'
    print(f'PASS: fresh process reopens/restores {scope}; interrupted publication preserves old generation.')
if __name__=='__main__': main()
