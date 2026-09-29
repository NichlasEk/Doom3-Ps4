#pragma once
#include <vulkan/vulkan.h>
#ifdef __cplusplus
extern "C" {
#endif
VkResult PS4_PresentDraw(VkDevice device,VkCommandBuffer command,VkImage scene,VkImageView source,VkImage target,VkExtent2D size);
void PS4_DestroyPresent(VkDevice device);

#ifdef __cplusplus
}
#endif
