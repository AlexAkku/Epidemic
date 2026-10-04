"""Check the public/private restore boundary; compiler fixtures run at configure."""
import argparse, copy, re, sys
from pathlib import Path
sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
PUBLIC = ROOT / 'EngineFramework/GameplayWorldStateOwners/SaveGame/include/Epidemic/GameFramework/SaveGame/save_game.h'
FIXTURES = ('public_control', 'internal_mechanics_control', 'restore_access', 'generic_participant', 'codec', 'fault', 'workspace', 'private_include', 'workspace_copy')

def validate(public, cmake, fixtures, owner_cmake):
    errors = []
    if 'include("${CMAKE_SOURCE_DIR}/cmake/SaveGamePrivateSurface.cmake")' not in owner_cmake:
        errors.append('external consumer compile fixtures are disconnected from SaveGame configure')
    for pattern in (r'struct\s+SnapshotRestoreAccess[^;]*\{', r'class\s+SnapshotParticipant[^;]*\{',
                    r'class\s+SnapshotWriter', r'VisitSnapshotFields', r'fail_snapshot_prepare_for_test',
                    r'SaveGameInternal/'):
        if re.search(pattern, public): errors.append('privileged restore definition leaked: ' + pattern)
    if set(fixtures) != set(FIXTURES) or any(not text.strip() for text in fixtures.values()):
        errors.append('external compiler fixture set is incomplete')
    for name in ('restore_access', 'generic_participant', 'internal_mechanics_control'):
        for token in ('Snapshot CaptureSnapshot() const', 'Result<void> RestoreSnapshot('):
            if token not in fixtures.get(name, ''): errors.append('invalid owner in compiler fixture: ' + name)
    control_owner = fixtures.get('internal_mechanics_control', '').partition('struct Snapshot')[2].partition('void Consumer')[0].strip()
    for name in ('restore_access', 'generic_participant'):
        owner = fixtures.get(name, '').partition('struct Snapshot')[2].partition('void Consumer')[0].strip()
        if not control_owner or owner != control_owner: errors.append('negative owner differs from compiling internal control: ' + name)
    for token in ('try_compile(', 'public_control', 'elseif(_save_consumer_compiles)', 'FATAL_ERROR', *FIXTURES):
        if token not in cmake: errors.append('missing fail-closed compiler contract: ' + token)
    return errors

def main():
    ap = argparse.ArgumentParser(description=__doc__); ap.add_argument('--check', action='store_true'); ap.add_argument('--self-test', action='store_true'); args = ap.parse_args()
    public = PUBLIC.read_text(encoding='utf-8'); cmake = (ROOT / 'cmake/SaveGamePrivateSurface.cmake').read_text(encoding='utf-8')
    owner_cmake = (ROOT / 'EngineFramework/GameplayWorldStateOwners/SaveGame/CMakeLists.txt').read_text(encoding='utf-8')
    fixtures = {p.stem:p.read_text(encoding='utf-8') for p in (ROOT / 'Tests/Fixtures/SaveGameConsumers').glob('*.cpp')}
    errors = validate(public, cmake, fixtures, owner_cmake)
    if args.self_test:
        assert not errors, errors
        for suffix in ('struct SnapshotRestoreAccess {', 'class SnapshotParticipant {', 'class SnapshotWriter {', 'fail_snapshot_prepare_for_test'):
            assert validate(public + suffix, cmake, fixtures, owner_cmake)
        bad = copy.deepcopy(fixtures); bad.pop('public_control'); assert validate(public, cmake, bad, owner_cmake)
        assert validate(public, cmake.replace('try_compile(', ''), fixtures, owner_cmake)
        assert validate(public, cmake, fixtures, '')
        bad = copy.deepcopy(fixtures); bad['restore_access'] = bad['restore_access'].replace('Snapshot CaptureSnapshot()', 'void InvalidOwner()')
        assert validate(public, cmake, bad, owner_cmake)
    elif not args.check: ap.error('choose --check or --self-test')
    if errors: raise ValueError('\n'.join(errors))
    print('PASS: private restore boundary; two positive controls and seven compile rejection fixtures are required by Framework configure.')
    return 0
if __name__ == '__main__': raise SystemExit(main())
