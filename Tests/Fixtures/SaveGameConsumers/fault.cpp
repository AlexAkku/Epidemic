#include "Epidemic/GameFramework/SaveGame/save_game.h"
void Consumer() {
    epidemic::gameplay::savegame::detail::fail_snapshot_prepare_for_test = true;
}
