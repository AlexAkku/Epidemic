# Epidemic Editor product

## Platform
Windows native desktop, C++20 / Win32 common controls. An Operate surface.

## User and task
Game developers author Scene, Prefab and Graph documents, save/reopen projects,
and execute isolated Play sessions. Command API is the sole edit boundary.

## Constraints
Preserve stable IDs, properties, Undo/Redo, dirty/save points, typed connections,
Prefab parameters/overrides and activation diagnostics. Reuse Platform, Workspace,
Runtime and Serialization. No new UI library, Graph Canvas or Framework API change.

## Success
A dominant Scene viewport, clear hierarchy/Inspector, visible draft and edit mode,
legible failures, native keyboard/accessibility, DPI and high contrast. Real desktop
acceptance is manual when computer use is prohibited. ../README.md owns API detail.
