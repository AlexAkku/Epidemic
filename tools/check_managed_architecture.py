"""Fail-closed project-reference preflight for future production C# projects.

Ownership follows native folders; C# lives under an owning Managed/ directory.
Runtime peers connect through Support, Framework core never references Runtime.
Explicit binding sources stay under their owning project. Native generated code
continues to use ArchitectureFreeze.cmake. This does not claim C# semantic coverage
or evaluate MSBuild conditions, imports, packages or generated implicit SDK items.
"""
from pathlib import Path
import argparse
import tempfile
import xml.etree.ElementTree as ET

ROOTS = ('EngineBase', 'EngineRuntime', 'EngineFramework', 'Editor', 'Game')
BOUNDARY = {'RuntimeFoundation', 'World', 'Physics', 'Environment', 'Navigation'}


def owner(path, root):
    parts = path.relative_to(root).parts
    if len(parts) < 3 or 'Managed' not in parts[:-1] or parts[0] not in ROOTS:
        raise ValueError('managed source must have an owning layer/Managed directory')
    if parts[0] == 'EngineBase':
        raise ValueError('EngineBase cannot own a managed runtime')
    return parts


def allowed(source, target):
    layer, other = source[0], target[0]
    if layer == 'EngineRuntime':
        return other == layer and (source[1] == 'Support' or target[1] in {source[1], 'RuntimeFoundation'})
    if layer == 'EngineFramework':
        if other == layer:
            areas = {
                'BaseInfrastructure': {'BaseInfrastructure'},
                'GameplayWorldStateOwners': {'BaseInfrastructure', 'GameplayWorldStateOwners'},
                'RuntimeBoundary': {'BaseInfrastructure', 'RuntimeBoundary'},
                'IntegrationLayer': {'BaseInfrastructure', 'GameplayWorldStateOwners', 'RuntimeBoundary', 'IntegrationLayer'},
            }
            return target[1] in areas.get(source[1], set())
        return other == 'EngineRuntime' and (
            ((source[1] == 'IntegrationLayer' and target[1] == 'RuntimeFoundation') or
             (source[1] == 'RuntimeBoundary' and target[1] in BOUNDARY)))
    if layer == 'Editor':
        return other in {'EngineRuntime', 'EngineFramework', 'Editor'}
    return layer == 'Game' and other in {'Game', 'EngineFramework', 'EngineRuntime'}


def literal_path(project, value, root):
    if not value or any(token in value for token in ('$(', '@(', '%(', '*', '?', ';')):
        raise ValueError('unsupported dynamic MSBuild path')
    target = (project.parent / value.replace('\\', '/')).resolve()
    if not target.is_relative_to(root):
        raise ValueError('reference escaped the repository')
    if not target.is_file():
        raise ValueError('referenced file is missing')
    return target


def validate(root):
    root = root.resolve()
    projects = [p for layer in ROOTS for p in (root / layer).rglob('*.csproj')]
    for layer in ROOTS:
        for source_file in (root / layer).rglob('*.cs'):
            owner(source_file, root)
            if not any(project.parent in source_file.parents for project in projects):
                raise ValueError('managed source requires an owning project')
    for project in projects:
        source = owner(project, root)
        tree = ET.parse(project)
        for element in tree.iter():
            tag = element.tag.rsplit('}', 1)[-1]
            if tag in {'Import', 'Reference'}:
                raise ValueError('explicit imports/binary references need an evaluated ownership policy')
            if tag == 'ProjectReference':
                target = literal_path(project, element.get('Include'), root)
                if target == project or target.suffix != '.csproj' or not allowed(source, owner(target, root)):
                    raise ValueError('forbidden project reference')
            if tag == 'Compile' and 'Include' in element.attrib:
                target = literal_path(project, element.get('Include'), root)
                if not target.is_relative_to(project.parent):
                    raise ValueError('binding source escaped its owning project')
    return len(projects)


def self_test(parent):
    with tempfile.TemporaryDirectory(prefix='managed-architecture-', dir=parent) as temporary:
        root = Path(temporary)
        projects = ['EngineRuntime/RuntimeFoundation/Managed/Foundation.csproj',
                    'EngineRuntime/Graph/Managed/Graph.csproj',
                    'EngineRuntime/World/Managed/World.csproj',
                    'EngineFramework/GameplayWorldStateOwners/Probe/Managed/Probe.csproj']
        for name in projects:
            p = root / name
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_text('<Project/>', encoding='utf-8')
        probe = root / projects[1]

        def case(target, expected):
            import os
            include = os.path.relpath(root / target, probe.parent)
            probe.write_text(f'<Project><ItemGroup><ProjectReference Include="{include}"/></ItemGroup></Project>', encoding='utf-8')
            try:
                validate(root)
            except ValueError as error:
                if expected != str(error):
                    raise AssertionError(f'wrong failure: {error}') from error
            else:
                if expected:
                    raise AssertionError('invalid project reference accepted')
            finally:
                probe.write_text('<Project/>', encoding='utf-8')

        case(projects[0], None)
        case(projects[2], 'forbidden project reference')
        # Reverse layer: Runtime -> Framework; both project files exist.
        case(projects[3], 'forbidden project reference')
        probe = root / projects[3]
        case(projects[2], 'forbidden project reference')
        probe = root / projects[1]
        case(projects[0], None)
        orphan = root / 'EngineRuntime/Probe/Managed/Orphan.cs'
        orphan.parent.mkdir(parents=True)
        orphan.write_text('class Orphan {}', encoding='utf-8')
        try:
            validate(root)
        except ValueError as error:
            if str(error) != 'managed source requires an owning project':
                raise AssertionError(f'wrong source ownership failure: {error}') from error
        else:
            raise AssertionError('unowned managed source accepted')
        orphan.unlink()
        rogue = root / 'EngineRuntime/Probe/Rogue.cs'
        rogue.write_text('class Rogue {}', encoding='utf-8')
        try:
            validate(root)
        except ValueError as error:
            if str(error) != 'managed source must have an owning layer/Managed directory':
                raise AssertionError(f'wrong source layer failure: {error}') from error
        else:
            raise AssertionError('managed source outside Managed accepted')
        rogue.unlink()
        shared = root / projects[2]
        shared.with_suffix('.cs').write_text('// owned by World', encoding='utf-8')
        import os
        include = os.path.relpath(shared.with_suffix('.cs'), probe.parent)
        probe.write_text(f'<Project><ItemGroup><Compile Include="{include}"/></ItemGroup></Project>', encoding='utf-8')
        try:
            validate(root)
        except ValueError as error:
            if str(error) != 'binding source escaped its owning project':
                raise AssertionError(f'wrong failure: {error}') from error
        else:
            raise AssertionError('unowned generated binding accepted')
        probe.write_text('<Project/>', encoding='utf-8')
        # Regression: core -> boundary -> Runtime must not bypass the core rule.
        boundary = root / 'EngineFramework/RuntimeBoundary/Probe/Managed/Boundary.csproj'
        boundary.parent.mkdir(parents=True, exist_ok=True)
        reference = os.path.relpath(root / projects[2], boundary.parent)
        boundary.write_text(f'<Project><ItemGroup><ProjectReference Include="{reference}"/></ItemGroup></Project>', encoding='utf-8')
        probe = root / projects[3]
        case(boundary.relative_to(root), 'forbidden project reference')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('root', type=Path)
    parser.add_argument('--self-test', type=Path)
    args = parser.parse_args()
    try:
        count = validate(args.root)
        if args.self_test:
            self_test(args.self_test)
        print(f'Managed architecture PASS ({count} production projects; project-reference preflight)')
    except (ValueError, ET.ParseError, AssertionError) as error:
        parser.exit(1, f'Managed architecture FAIL: {error}\n')


if __name__ == '__main__':
    main()
