#pragma once
#include <windows.h>
#include "Epidemic/Game/Editor/editor_workspace.h"

namespace epidemic::game::editor {
void ShowContentBrowser(HWND owner, EditorWorkspace &workspace);
void SmokeContentBrowser(HWND owner, EditorWorkspace &workspace);
} // namespace epidemic::game::editor
