#pragma once
#include <Epidemic/RHI/irhi_device.h>
namespace epidemic::rhi::webgl {
// Implements the existing RHI v1 clear/present contract, not a sprite/drawing
// API.
[[nodiscard]] foundation::Result<std::shared_ptr<IRhiDevice>>
CreateWebGL2RhiDevice(const RhiDeviceDesc &desc);
} // namespace epidemic::rhi::webgl
