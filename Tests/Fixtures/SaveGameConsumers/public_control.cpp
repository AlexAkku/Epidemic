#include "Epidemic/GameFramework/SaveGame/save_game.h"
#include <type_traits>
using namespace epidemic::gameplay::savegame;
static_assert(std::is_abstract_v<ISaveParticipant>);
void Consumer() {
    SaveRestoreWorkspace workspace;
    SaveGameImage image;
    (void)workspace;
    (void)image;
}
