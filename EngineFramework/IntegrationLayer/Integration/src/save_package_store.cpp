#include "Epidemic/GameFramework/Integration/core_adapters.h"
#include "Epidemic/Platform/iplatform_runtime.h"

namespace epidemic::platform::detail {
void SetAtomicFileFaultPointForTesting(std::string_view);
void ClearAtomicFileFaultPointForTesting() noexcept;
} // namespace epidemic::platform::detail
namespace epidemic::gameplay::integration {
namespace detail {
void SetSaveStoreFaultPointForTesting(std::string_view point) {
  platform::detail::SetAtomicFileFaultPointForTesting(point);
}
void ClearSaveStoreFaultPointForTesting() noexcept {
  platform::detail::ClearAtomicFileFaultPointForTesting();
}
} // namespace detail
foundation::Result<std::vector<std::byte>> FileSavePackageStore::Load() {
  return platform::ReadFilePackage(foundation::Path(path_),
                                   limits_.max_wire_bytes);
}
foundation::Result<void>
FileSavePackageStore::Commit(std::span<const std::byte> package) {
  auto validated = savegame::DecodeSaveGameImage(package, limits_);
  if (!validated)
    return foundation::Result<void>::Failure(validated.GetError());
  return platform::CommitFilePackage(foundation::Path(path_), package,
                                     limits_.max_wire_bytes);
}
} // namespace epidemic::gameplay::integration
