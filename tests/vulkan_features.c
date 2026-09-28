/* Regression test of the actual driver's feature query; no host Vulkan loader. */
#include "vk_ps4_internal.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    VkPs4PhysicalDevice phys = {0};
    phys.features.robustBufferAccess = VK_TRUE;
    VkPhysicalDeviceVulkan11Features f11;
    VkPhysicalDeviceVulkan12Features f12;
    VkPhysicalDeviceVulkan13Features f13;
    struct { VkStructureType sType; void *pNext; unsigned sentinel; } unknown;
    memset(&f11, 0xa5, sizeof(f11));
    memset(&f12, 0xa5, sizeof(f12));
    memset(&f13, 0xa5, sizeof(f13));
    unknown.sType = (VkStructureType)0x7ffffffe;
    unknown.pNext = &f11;
    unknown.sentinel = 0x12345678;
    f13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    f13.pNext = &f12;
    f12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    f12.pNext = &unknown;
    f11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    f11.pNext = NULL;
    VkPhysicalDeviceFeatures2 query = {0};
    query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    query.pNext = &f13;
    vk_ps4_GetPhysicalDeviceFeatures2((VkPhysicalDevice)&phys, &query);
    assert(query.features.robustBufferAccess == VK_TRUE);
    assert(f13.pNext == &f12 && f12.pNext == &unknown && f11.pNext == NULL);
    assert(unknown.sentinel == 0x12345678 && unknown.pNext == &f11);
    assert(f11.storageBuffer16BitAccess == VK_FALSE);
    assert(f12.runtimeDescriptorArray == VK_FALSE);
    assert(f12.shaderFloat16 == VK_FALSE);
    assert(f12.drawIndirectCount == VK_FALSE);
    assert(f12.separateDepthStencilLayouts == VK_FALSE);
    assert(f12.descriptorIndexing == VK_FALSE);
    VkPhysicalDeviceVulkan13Features expected13 = {0};
    expected13.sType = f13.sType;
    expected13.pNext = f13.pNext;
    assert(memcmp(&f13, &expected13, sizeof(f13)) == 0);
#define CHECK_EXTENSION(Type, stype, field) do { \
    Type ext = {0}; ext.sType = stype; query.pNext = &ext; \
    vk_ps4_GetPhysicalDeviceFeatures2((VkPhysicalDevice)&phys, &query); \
    assert(f12.field == ext.field); \
} while (0)
    CHECK_EXTENSION(VkPhysicalDeviceTimelineSemaphoreFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES, timelineSemaphore);
    CHECK_EXTENSION(VkPhysicalDeviceImagelessFramebufferFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGELESS_FRAMEBUFFER_FEATURES, imagelessFramebuffer);
    CHECK_EXTENSION(VkPhysicalDeviceScalarBlockLayoutFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES, scalarBlockLayout);
    CHECK_EXTENSION(VkPhysicalDeviceUniformBufferStandardLayoutFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_UNIFORM_BUFFER_STANDARD_LAYOUT_FEATURES, uniformBufferStandardLayout);
    CHECK_EXTENSION(VkPhysicalDeviceHostQueryResetFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES, hostQueryReset);
    CHECK_EXTENSION(VkPhysicalDeviceBufferDeviceAddressFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES, bufferDeviceAddress);
    CHECK_EXTENSION(VkPhysicalDeviceVulkanMemoryModelFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_MEMORY_MODEL_FEATURES, vulkanMemoryModel);
    CHECK_EXTENSION(VkPhysicalDeviceShaderAtomicInt64Features, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES, shaderBufferInt64Atomics);
    CHECK_EXTENSION(VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures, VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES, shaderSubgroupExtendedTypes);
    VkDeviceCreateInfo create = {0};
    create.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create.pNext = &f13;
    assert(vk_ps4_validate_aggregate_features((VkPhysicalDevice)&phys, &create) == VK_SUCCESS);
    f13.shaderDemoteToHelperInvocation = VK_TRUE;
    assert(vk_ps4_validate_aggregate_features((VkPhysicalDevice)&phys, &create) == VK_ERROR_FEATURE_NOT_PRESENT);
    f13.shaderDemoteToHelperInvocation = VK_FALSE;
    f12.runtimeDescriptorArray = VK_TRUE;
    assert(vk_ps4_validate_aggregate_features((VkPhysicalDevice)&phys, &create) == VK_ERROR_FEATURE_NOT_PRESENT);
    f12.runtimeDescriptorArray = VK_FALSE;
    f11.storageBuffer16BitAccess = VK_TRUE;
    assert(vk_ps4_validate_aggregate_features((VkPhysicalDevice)&phys, &create) == VK_ERROR_FEATURE_NOT_PRESENT);
    f11.storageBuffer16BitAccess = VK_FALSE;
    VkPhysicalDeviceFeatures requested = {0};
    requested.robustBufferAccess = VK_TRUE;
    create.pEnabledFeatures = &requested;
    assert(vk_ps4_validate_aggregate_features((VkPhysicalDevice)&phys, &create) == VK_SUCCESS);
    requested.tessellationShader = VK_TRUE;
    assert(vk_ps4_validate_aggregate_features((VkPhysicalDevice)&phys, &create) == VK_ERROR_FEATURE_NOT_PRESENT);
    create.pEnabledFeatures = NULL;
    query.pNext = &f13;
    query.features = requested;
    create.pNext = &query;
    assert(vk_ps4_validate_aggregate_features((VkPhysicalDevice)&phys, &create) == VK_ERROR_FEATURE_NOT_PRESENT);
    query.features.tessellationShader = VK_FALSE;
    assert(vk_ps4_validate_aggregate_features((VkPhysicalDevice)&phys, &create) == VK_SUCCESS);
    unsigned char storage[512] = {0};
    VkPs4DeviceMemory memory = {0};
    memory.size = sizeof(storage);
    memory.gnm_mem.mapped = storage;
    VkMemoryMapInfo map = {0};
    map.sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO;
    map.memory = (VkDeviceMemory)&memory;
    map.offset = 128;
    map.size = VK_WHOLE_SIZE;
    void *data = NULL;
    assert(vkMapMemory2(VK_NULL_HANDLE, &map, &data) == VK_SUCCESS);
    assert(data == storage + 128 && memory.mapped_size == 384);
    ((unsigned char *)data)[0] = 42;
    assert(storage[128] == 42);
    VkMemoryUnmapInfo unmap = {0};
    unmap.sType = VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO;
    unmap.memory = map.memory;
    assert(vkUnmapMemory2KHR(VK_NULL_HANDLE, &unmap) == VK_SUCCESS);
    assert(memory.mapped_ptr == NULL && memory.gnm_mem.mapped == storage);
    map.size = 64;
    assert(vkMapMemory2KHR(VK_NULL_HANDLE, &map, &data) == VK_SUCCESS);
    assert(data == storage + 128 && memory.mapped_size == 64);
    assert(vkUnmapMemory2(VK_NULL_HANDLE, &unmap) == VK_SUCCESS);
    data = storage;
    map.offset = 513;
    assert(vkMapMemory2(VK_NULL_HANDLE, &map, &data) == VK_ERROR_MEMORY_MAP_FAILED);
    assert(data == storage && memory.mapped_ptr == NULL);
    map.offset = 500;
    assert(vkMapMemory2(VK_NULL_HANDLE, &map, &data) == VK_ERROR_MEMORY_MAP_FAILED);
    map.offset = 0;
    map.flags = VK_MEMORY_MAP_PLACED_BIT_EXT;
    assert(vkMapMemory2KHR(VK_NULL_HANDLE, &map, &data) == VK_ERROR_MEMORY_MAP_FAILED);
    unmap.flags = VK_MEMORY_UNMAP_RESERVE_BIT_EXT;
    assert(vkUnmapMemory2KHR(VK_NULL_HANDLE, &unmap) == VK_ERROR_MEMORY_MAP_FAILED);
    puts("PASS: map-memory2 core/KHR entry points, offsets, whole size, overflow checks, placed mapping rejected");
    VkPs4CommandBuffer command = {0};
    VkPs4DescriptorSet set0 = {0}, set1 = {0};
    VkDescriptorSet bound[2] = {(VkDescriptorSet)&set0, (VkDescriptorSet)&set1};
    vk_ps4_CmdBindDescriptorSets((VkCommandBuffer)&command, VK_PIPELINE_BIND_POINT_GRAPHICS,
        VK_NULL_HANDLE, 0, 1, &bound[0], 0, NULL);
    vk_ps4_CmdBindDescriptorSets((VkCommandBuffer)&command, VK_PIPELINE_BIND_POINT_GRAPHICS,
        VK_NULL_HANDLE, 1, 1, &bound[1], 0, NULL);
    assert(command.resource_error == VK_SUCCESS);
    assert(command.graphics_sets[0] == &set0 && command.graphics_sets[1] == &set1);
    vk_ps4_CmdBindDescriptorSets((VkCommandBuffer)&command, VK_PIPELINE_BIND_POINT_GRAPHICS,
        VK_NULL_HANDLE, UINT32_MAX, 1, bound, 0, NULL);
    assert(command.resource_error == VK_ERROR_FEATURE_NOT_PRESENT);
    assert(command.graphics_sets[0] == &set0 && command.graphics_sets[1] == &set1);
    command.resource_error = VK_SUCCESS;
    vk_ps4_CmdBindDescriptorSets((VkCommandBuffer)&command, VK_PIPELINE_BIND_POINT_GRAPHICS,
        VK_NULL_HANDLE, 1, 2, bound, 0, NULL);
    assert(command.resource_error == VK_ERROR_FEATURE_NOT_PRESENT);
    puts("PASS: independent descriptor-set binding and out-of-range rejection");
    uint32_t version = 0;
    assert(vk_ps4_EnumerateInstanceVersion(&version) == VK_SUCCESS);
    assert(version == VK_API_VERSION_1_1);
    puts("PASS: DUDE feature chain, unknown structure, extension parity, unsupported feature rejection, API 1.1 retained");
    return 0;
}
