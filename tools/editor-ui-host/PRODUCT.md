# Editor UI product context

Source of truth: `Temp/!Tasks/U_Phase.md` and `Temp/!Tasks/UI_Roadmap.md`. The user explicitly requests autonomous implementation without interviews; the supplied specification fixes product and visual decisions.

Audience: Windows game developers authoring Epidemic projects. Task: start in a project launcher, navigate Project/Scene/Graph/Code contexts, inspect stable project data across native windows and discover planned capabilities without false backend success. C++ authoring, serialization, Resources, history and Play remain authoritative in the existing engine. One native backend and one shared UI session serve all windows. Legacy editing stays available until feature parity.

Platform: Windows desktop, IntelliJ Platform ToolWindowManager/FileEditorProvider/ActionManager, shared Java 21 views and existing C++23 backend/Renderer. The standalone Swing comparison is diagnostic only and excluded from the Platform plugin. Installed IU hosts development qualification; an open-module product distribution is still required. See Development/EditorUI/UI_TECH_ADR.md.
