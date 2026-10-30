#include "Epidemic/GameFramework/SaveGame/save_game.h"
struct Owner {};
void Consumer(Owner &owner,
              epidemic::gameplay::savegame::SaveRestoreWorkspace &workspace) {
  workspace.Bind(owner);
}
