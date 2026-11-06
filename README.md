# Epidemic Engine

**Epidemic Engine** is a modular game engine written in C++23, designed for developing 2D games with a path toward full 3D support.

The engine provides a native game runtime, an extensible gameplay framework, a visual editor, and tools for building interactive applications. Its development focuses on performance, modularity, predictable execution, and automated testing.

Windows is the primary development platform. Experimental WebAssembly support enables selected engine components and games to run in a web browser.

## Features

### Game Development

- **2D Rendering:** Sprite rendering, transformations, layered scenes, cameras, and visual presentation.
- **Scene Management:** Hierarchical scene objects, reusable entities, world management, and scene serialization.
- **Game Objects:** Hybrid architecture combining object hierarchies with entity-component principles.
- **Physics:** Engine-level systems for physical simulation and collision handling.
- **Animation:** Runtime infrastructure for animated objects and characters.
- **Audio:** Sound playback and audio resource management.
- **Input:** Keyboard, mouse, and platform input processing.
- **Resources:** Asset loading, resource management, and reusable content.
- **Persistence:** Serialization, scene storage, and persistent game data.
- **Time and Simulation:** Fixed-step updates, independent execution clocks, and controlled simulation.

Some runtime subsystems are still being expanded and integrated into the editor.

### Visual Editor

Epidemic Editor is the development environment for creating and managing games.

Its capabilities include:

- Reusable prefabs and scene content.
- Visual scripting and execution graphs.
- In-editor game execution and testing.
- Customizable workspaces.

The editor is under active development. Feature availability depends on the current development milestone.

### Programming and Scripting

Epidemic supports multiple approaches to implementing game logic.

**C++23:** Native programming provides direct access to engine systems and is intended for performance-sensitive code, engine extensions, and gameplay implementation.

**C#:** The engine includes a native-to-managed interoperability layer designed to support gameplay scripting through C#.

**Visual Scripting:** A graph-based execution system supports control flow, events, branching, repetition, scheduled execution, and asynchronous waits.

The graph runtime includes compilation, a virtual machine, execution scheduling, and serialization.

These approaches are intended to coexist within a single game project as their integrations mature.

### Development and Diagnostics

The engine includes infrastructure for automated verification and debugging:

- Modular CMake builds.
- Automated module and integration tests.
- Headless execution for testing without a graphical window.
- Runtime diagnostics and logging.
- Deterministic execution and persistence verification.
- Independent test and diagnostic applications.
- SDK packaging for external game projects.

## Supported Platforms

| Platform | Status |
|---|---|
| Windows 11 | Primary development platform |
| Win32 / Direct3D 11 | Primary native backend |
| WebAssembly / Emscripten | Experimental |
| WebGL2 / Canvas2D | Experimental browser presentation |
| iOS, macOS, Android | Not currently supported as complete game targets |

Additional graphics backends and platform support are planned for future development.

## Building

The primary build environment is Windows with CMake, a C++23-compatible compiler, and the required Windows development SDK.

From the repository root:

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

Run the registered tests:

```powershell
ctest --test-dir build -C Debug --output-on-failure
```

The available build targets depend on the selected configuration and development branch.

For game integration and SDK usage, see [Game Authoring](Documentation/game-authoring.md).

## Documentation

- [Architecture](<Documentation/Epidemic Engine Vault/90 Первоисточники/Documentation/architecture.md>)
- [Game Development Guide](Documentation/game-authoring.md)
- [EngineBase Documentation](<Documentation/Epidemic Engine Vault/90 Первоисточники/Documentation/EngineBase/README.md>)
- [EngineRuntime Documentation](<Documentation/Epidemic Engine Vault/90 Первоисточники/Documentation/EngineRuntime/README.md>)
- [GameFramework Documentation](<Documentation/Epidemic Engine Vault/90 Первоисточники/Documentation/EngineFramework/README.md>)
- [Web Platform](Documentation/web-platform.md)

## Project Status

Epidemic Engine is under active development.

The current development direction focuses on completing the 2D game development workflow, integrating editor functionality with runtime systems, and improving stability through automated testing.

Future development includes expanded 3D capabilities, additional rendering backends, and broader platform support.

## TODO

- [x] Переход на C++ 23
- [ ] Engine Freeze
  - [x] Цель 1. Проверяемый baseline и единый freeze contract
  - [x] Цель 2. Полный локальный freeze-аудит EngineBase
  - [x] Цель 3. Полный локальный freeze-аудит EngineRuntime
  - [x] Цель 4. Полный локальный freeze-аудит EngineFramework
  - [x] Цель 5. Доказать persistence и determinism всего движка
  - [x] Цель 6. Build independent observer
  - [ ] Цель 7. Hardering
  - [ ] Цель 8. Integration qualification
  - [ ] Цель 9. Destructive/stress/fault qualification
- [ ] Editor UI
- [ ] Documentation
- [ ] Интеграция DX11/12/Vulacan/Metal для Win/Mac/Android
- [ ] Добавление дефолтного проекта
- [ ] Переход на С++ 26

Текущая базовая платформа — Windows 11 / Win32 / D3D11.

## License

This project is licensed under the PolyForm Noncommercial License 1.0.0.

Non-commercial use, study, research, modification, and hobby projects are permitted under the terms of the license.

Commercial use requires a separate license from the copyright holder.

See [LICENSE](LICENSE) for the full license terms.
