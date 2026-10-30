#pragma once
#include "Epidemic/Game/Editor/editor_workspace.h"
#include <windows.h>
namespace epidemic::game::editor {
void ShowContentBrowser(HWND owner, EditorWorkspace &workspace);
void SmokeContentBrowser(HWND owner, EditorWorkspace &workspace);
} // namespace epidemic::game::editor
