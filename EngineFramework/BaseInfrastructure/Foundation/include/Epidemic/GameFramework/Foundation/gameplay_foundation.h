#pragma once

#include "Epidemic/GameFramework/Foundation/change_cursor.h"
#include "Epidemic/GameFramework/Foundation/gameplay_context.h"
#include "Epidemic/GameFramework/Foundation/id_generator.h"
#include "Epidemic/GameFramework/Foundation/ids.h"
#include "Epidemic/GameFramework/Foundation/tags.h"
#include "Epidemic/GameFramework/Foundation/time_types.h"
#include "Epidemic/GameFramework/Foundation/type_registry.h"

namespace epidemic::gameplay {
// Ownership-local prepare/publication customization. Defined by upper-layer
// adapters; this declaration creates no dependency on SaveGame or peer owners.
template <typename Owner> struct SnapshotRestoreAccess;
}
