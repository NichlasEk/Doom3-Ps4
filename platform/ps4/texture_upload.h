#pragma once
#include <vulkan/vulkan.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Explicit CPU-owned RGBA8 mip input. Generic Vulkan copy semantics unchanged.
 * Zero size/failure means unsupported: callers retain the generic fallback.
 * Initialize the whole packed allocation to zero before packing its mip levels.
 * Keep staging alive and unmodified until the GPU fence completes. */
size_t doom3_ps4_texture_bytes(VkImage image);
int doom3_ps4_pack_texture_mip(VkImage image, unsigned mip, unsigned w, unsigned h,
    const void *rgba, void *packed, size_t capacity);
void doom3_ps4_copy_texture(VkCommandBuffer cmd, VkBuffer staging,
    VkDeviceSize offset, VkImage image);
#ifdef __cplusplus
}
#endif
