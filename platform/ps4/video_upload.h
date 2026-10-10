#pragma once
#include <vulkan/vulkan.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
// Explicit CPU-owned RGBA8 video input; never used for generic Vulkan copies.
// NULL destination queries padded storage size. Zero means unsupported/failure.
size_t doom3_ps4_pack_video(VkImage image, unsigned w, unsigned h,
    const void *rgba, void *packed, size_t capacity);
void doom3_ps4_copy_video(VkCommandBuffer cmd, VkBuffer staging,
    VkDeviceSize offset, VkImage image);
#ifdef __cplusplus
}
#endif
