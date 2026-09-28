/* Derived from OpenGNM tests/test_triangle_ps4.c; see
 * thirdparty/ps4-native/LICENSE.vulkan-ps4. Modified for Doom3-Ps4:
 * Vulkan 1.4 SPIR-V demote + derivative + independent descriptor-set probe.
 * Uses an experimental internal path; does not advertise feature support. */
#include <vulkan/vulkan.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vk_ps4_log.h"

#include <unistd.h>
#include "probe_spirv.h"
#ifdef PROBE_DUDE_GENERIC
#include "probe_params.h"
#ifdef PROBE_DUDE_AMBIENT
static const float g_vertices[] = {
 -1,-1,0,0, 0,0,1, 1,0,0, 0,1,0, 1,1,1,1,
 3,-1,0,0, 0,0,1, 1,0,0, 0,1,0, 1,1,1,1,
 -1,3,0,0, 0,0,1, 1,0,0, 0,1,0, 1,1,1,1};
#else
static const float g_vertices[] = {-1,-1,0,0,1, 3,-1,80,0,1, -1,3,0,45,1};
#endif
#else
static const float g_vertices[] = {
    -1, -1, 0,0,0, 3,-1,0,0,0, -1,3,0,0,0
};
#endif
#define CHECK(call) do { VkResult result = (call); if (result != VK_SUCCESS) { \
    vk_ps4_log("FAIL %s: %d", #call, result); return 1; } } while (0)

#ifdef PROBE_LIGHTING
#include "probe_lighting.h"
#endif

int main(void) {
    printf("=== vulkan-ps4 PS4 Triangle Test ===\n");

    /* Open breadcrumb log early so all ICD calls are traced */
    vk_ps4_log_open("/data/vk_ps4_breadcrumb.log");
    vk_ps4_log_raw("=== test_triangle_ps4 started ===");

    /* 1. Create instance */
    vk_ps4_log_raw("TEST: creating instance");
    VkInstanceCreateInfo inst_ci = {0};
    inst_ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    VkInstance inst = VK_NULL_HANDLE;
    VkResult vr = vkCreateInstance(&inst_ci, NULL, &inst);
    if (vr != VK_SUCCESS) {
        printf("vkCreateInstance failed: %d\n", vr);
        vk_ps4_log("TEST: vkCreateInstance FAILED: %d", (int)vr);
        vk_ps4_log_close();
        return 1;
    }
    printf("Instance created OK\n");
    vk_ps4_log_raw("TEST: instance OK");

    /* 2. Enumerate physical device */
    uint32_t phys_count = 0;
    vkEnumeratePhysicalDevices(inst, &phys_count, NULL);
    if (phys_count == 0) {
        printf("No physical devices\n");
        vkDestroyInstance(inst, NULL);
        return 1;
    }
    VkPhysicalDevice phys = VK_NULL_HANDLE;
    vkEnumeratePhysicalDevices(inst, &phys_count, &phys);
    printf("Physical device: %u\n", phys_count);

    /* 3. Get queue family */
    uint32_t qf_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(phys, &qf_count, NULL);
    VkQueueFamilyProperties qf[4] = {0};
    vkGetPhysicalDeviceQueueFamilyProperties(phys, &qf_count, qf);
    printf("Queue families: %u, family0: graphics=%d, count=%u\n",
           qf_count, !!(qf[0].queueFlags & VK_QUEUE_GRAPHICS_BIT), qf[0].queueCount);

    /* 4. Create device + queue */
    float queue_pri = 1.0f;
    VkDeviceQueueCreateInfo q_ci = {0};
    q_ci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    q_ci.queueFamilyIndex = 0;
    q_ci.queueCount = 1;
    q_ci.pQueuePriorities = &queue_pri;

    const char *dev_exts[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    VkDeviceCreateInfo dev_ci = {0};
    dev_ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dev_ci.queueCreateInfoCount = 1;
    dev_ci.pQueueCreateInfos = &q_ci;
    dev_ci.enabledExtensionCount = 1;
    dev_ci.ppEnabledExtensionNames = dev_exts;

    VkDevice dev = VK_NULL_HANDLE;
    vk_ps4_log_raw("TEST: creating device");
    vr = vkCreateDevice(phys, &dev_ci, NULL, &dev);
    if (vr != VK_SUCCESS) {
        printf("vkCreateDevice failed: %d\n", vr);
        vk_ps4_log("TEST: vkCreateDevice FAILED: %d", (int)vr);
        vkDestroyInstance(inst, NULL);
        vk_ps4_log_close();
        return 1;
    }
    printf("Device created OK\n");
    vk_ps4_log_raw("TEST: device OK");

    VkQueue queue;
    vkGetDeviceQueue(dev, 0, 0, &queue);

    /* 5. Create swapchain */
    vk_ps4_log_raw("TEST: creating swapchain");
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    /* On PS4, the swapchain creates the surface internally via sceVideoOut.
     * We pass VK_NULL_HANDLE as the surface — the ICD handles it. */
    VkSwapchainCreateInfoKHR sw_ci = {0};
    sw_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sw_ci.surface = surface;
    sw_ci.minImageCount = 2;
    sw_ci.imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
    sw_ci.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    sw_ci.imageExtent.width = 1280;
    sw_ci.imageExtent.height = 720;
    sw_ci.imageArrayLayers = 1;
    sw_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sw_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    sw_ci.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    sw_ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sw_ci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    sw_ci.clipped = VK_TRUE;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    vr = vkCreateSwapchainKHR(dev, &sw_ci, NULL, &swapchain);
    if (vr != VK_SUCCESS) {
        printf("vkCreateSwapchainKHR failed: %d\n", vr);
        vk_ps4_log("TEST: vkCreateSwapchainKHR FAILED: %d", (int)vr);
        vkDestroyDevice(dev, NULL);
        vkDestroyInstance(inst, NULL);
        vk_ps4_log_close();
        return 1;
    }
    printf("Swapchain created OK (1280x720)\n");
    vk_ps4_log_raw("TEST: swapchain OK");

    /* 6. Get swapchain images */
    uint32_t sw_img_count = 0;
    vkGetSwapchainImagesKHR(dev, swapchain, &sw_img_count, NULL);
    VkImage *sw_images = malloc(sw_img_count * sizeof(*sw_images));
    vkGetSwapchainImagesKHR(dev, swapchain, &sw_img_count, sw_images);
    printf("Swapchain images: %u\n", sw_img_count);

    /* 7. Create render pass */
    VkAttachmentDescription att = {0};
    att.format = VK_FORMAT_R8G8B8A8_UNORM;
    att.samples = VK_SAMPLE_COUNT_1_BIT;
    att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    att.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference ref = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass = {0};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &ref;

    VkRenderPassCreateInfo rp_ci = {0};
    rp_ci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_ci.attachmentCount = 1;
    rp_ci.pAttachments = &att;
    rp_ci.subpassCount = 1;
    rp_ci.pSubpasses = &subpass;

    VkRenderPass rp = VK_NULL_HANDLE;
    CHECK(vkCreateRenderPass(dev, &rp_ci, NULL, &rp));
    printf("Render pass created OK\n");

    /* 8. Create image views + framebuffers */
    VkImageView *sw_views = malloc(sw_img_count * sizeof(*sw_views));
    VkFramebuffer *fbs = malloc(sw_img_count * sizeof(*fbs));
    for (uint32_t i = 0; i < sw_img_count; i++) {
        VkImageViewCreateInfo iv_ci = {0};
        iv_ci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        iv_ci.image = sw_images[i];
        iv_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        iv_ci.format = VK_FORMAT_R8G8B8A8_UNORM;
        iv_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        iv_ci.subresourceRange.levelCount = 1;
        iv_ci.subresourceRange.layerCount = 1;
        CHECK(vkCreateImageView(dev, &iv_ci, NULL, &sw_views[i]));

        VkFramebufferCreateInfo fb_ci = {0};
        fb_ci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb_ci.renderPass = rp;
        fb_ci.attachmentCount = 1;
        fb_ci.pAttachments = &sw_views[i];
        fb_ci.width = 1280;
        fb_ci.height = 720;
        fb_ci.layers = 1;
        CHECK(vkCreateFramebuffer(dev, &fb_ci, NULL, &fbs[i]));
    }
    printf("Image views + framebuffers created OK\n");

    /* 9. Create vertex buffer */
    VkBufferCreateInfo vb_ci = {0};
    vb_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vb_ci.size = sizeof(g_vertices);
    vb_ci.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vb_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer vbuf;
    CHECK(vkCreateBuffer(dev, &vb_ci, NULL, &vbuf));

    VkMemoryRequirements vbuf_mem_req;
    vkGetBufferMemoryRequirements(dev, vbuf, &vbuf_mem_req);

    VkMemoryAllocateInfo vbuf_mem_ai = {0};
    vbuf_mem_ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vbuf_mem_ai.allocationSize = vbuf_mem_req.size;
    vbuf_mem_ai.memoryTypeIndex = 0;  /* Onion (host-coherent and GPU-visible) */

    VkDeviceMemory vbuf_mem;
    CHECK(vkAllocateMemory(dev, &vbuf_mem_ai, NULL, &vbuf_mem));
    CHECK(vkBindBufferMemory(dev, vbuf, vbuf_mem, 0));

    /* Copy vertex data */
    void *mapped = NULL;
    CHECK(vkMapMemory(dev, vbuf_mem, 0, VK_WHOLE_SIZE, 0, &mapped));
    if (mapped) {
        memcpy(mapped, g_vertices, sizeof(g_vertices));
        vkUnmapMemory(dev, vbuf_mem);
    }
    printf("Vertex buffer created OK\n");

    /* 10. Create shader modules */
    vk_ps4_log_raw("TEST: creating shader modules");
    VkShaderModuleCreateInfo vs_ci = {0};
    vs_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vs_ci.codeSize = sizeof(g_vert_spv);
    vs_ci.pCode = g_vert_spv;
    VkShaderModule vs_mod;
    CHECK(vkCreateShaderModule(dev, &vs_ci, NULL, &vs_mod));

    VkShaderModuleCreateInfo fs_ci = {0};
    fs_ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fs_ci.codeSize = sizeof(g_frag_spv);
    fs_ci.pCode = g_frag_spv;
    VkShaderModule fs_mod;
    CHECK(vkCreateShaderModule(dev, &fs_ci, NULL, &fs_mod));
    printf("Shader modules created OK\n");
    vk_ps4_log_raw("TEST: shader modules OK");

    /* 11. Create graphics pipeline */
    VkPipelineShaderStageCreateInfo stages[2] = {0};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vs_mod;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fs_mod;
    stages[1].pName = "main";

    VkVertexInputBindingDescription vibd = {0};
    vibd.binding = 0;
    vibd.stride = 5 * sizeof(float);
    vibd.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription viads[6] = {0};
    viads[0].location = 0;
    viads[0].binding = 0;
    viads[0].format = VK_FORMAT_R32G32_SFLOAT;
    viads[0].offset = 0;
    viads[1].location = 1;
    viads[1].binding = 0;
    viads[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    viads[1].offset = 2 * sizeof(float);

    VkPipelineVertexInputStateCreateInfo vii = {0};
    vii.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vii.vertexBindingDescriptionCount = 1;
    vii.pVertexBindingDescriptions = &vibd;
    vii.vertexAttributeDescriptionCount = 2;
    vii.pVertexAttributeDescriptions = viads;
#ifdef PROBE_DUDE_GENERIC
    viads[2] = viads[1]; viads[2].location = 2;
    viads[3] = viads[1]; viads[3].location = 5;
    vii.vertexAttributeDescriptionCount = 4;
#endif

#ifdef PROBE_DUDE_AMBIENT
    vibd.stride = 17*sizeof(float);
    const unsigned offsets[] = {0,2,4,7,10,13};
    for (unsigned i=0;i<6;++i) {
        viads[i].location=i; viads[i].binding=0; viads[i].offset=offsets[i]*sizeof(float);
        viads[i].format=i<2?VK_FORMAT_R32G32_SFLOAT:(i==5?VK_FORMAT_R32G32B32A32_SFLOAT:VK_FORMAT_R32G32B32_SFLOAT);
    }
    vii.vertexAttributeDescriptionCount=6;
#endif
    VkPipelineInputAssemblyStateCreateInfo iai = {0};
    iai.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    iai.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport vp = {0, 0, 1280.0f, 720.0f, 0.0f, 1.0f};
    VkRect2D sc = {{0, 0}, {1280, 720}};
    VkPipelineViewportStateCreateInfo vpsi = {0};
    vpsi.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vpsi.viewportCount = 1;
    vpsi.pViewports = &vp;
    vpsi.scissorCount = 1;
    vpsi.pScissors = &sc;

    VkPipelineRasterizationStateCreateInfo rsi = {0};
    rsi.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rsi.polygonMode = VK_POLYGON_MODE_FILL;
    rsi.lineWidth = 1.0f;
    rsi.cullMode = VK_CULL_MODE_NONE;

    VkPipelineMultisampleStateCreateInfo msi = {0};
    msi.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msi.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState cba = {0};
    cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo cbsi = {0};
    cbsi.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cbsi.attachmentCount = 1;
    cbsi.pAttachments = &cba;

    VkDescriptorSetLayout set_layouts[2];
    VkDescriptorSet sets[2];
    VkBuffer uniform_buffers[2] = {0};
    VkDeviceMemory uniform_memory[2] = {0};
#ifdef PROBE_DUDE_GENERIC
    float uniform_data[2][PROBE_PARAM_FLOATS] = {{0}};
    const unsigned uniform_bytes = sizeof(uniform_data[0]);
    for (unsigned i = 0; i < 4; ++i) uniform_data[0][PARAM_u_mvpMatrix+i*5] = 1;
    uniform_data[0][PARAM_u_diffuseMatrixS] = 1;
    uniform_data[0][PARAM_u_diffuseMatrixT+1] = 1;
    for (unsigned i = 0; i < 4; ++i) {
        uniform_data[0][PARAM_u_vertexColorAdd+i] = 1;
        uniform_data[0][PARAM_u_color+i] = 1;
    }
    uniform_data[0][PARAM_u_alphaTest] = .5f;
    uniform_data[0][PARAM_u_alphaTest+1] = 1;
#ifdef PROBE_LIGHTING
    LightingResources lighting = {0};
    uniform_data[0][PARAM_u_color] = 0;
    for (unsigned i=0;i<3;++i) uniform_data[0][PARAM_u_diffuseModifier+i]=1;
    uniform_data[0][PARAM_u_modelMatrixRow0]=1;
    uniform_data[0][PARAM_u_modelMatrixRow1+1]=1;
    uniform_data[0][PARAM_u_modelMatrixRow2+2]=1;
    uniform_data[0][PARAM_u_lightProjectionS+3]=.5;
    uniform_data[0][PARAM_u_lightProjectionT+3]=.5;
    uniform_data[0][PARAM_u_lightProjectionQ+3]=1;
    uniform_data[0][PARAM_u_lightFalloffS+3]=.5;
#endif
    VkImage texture;
    VkDeviceMemory texture_memory;
    VkImageView texture_view;
    VkSampler texture_sampler;
#else
    const unsigned uniform_bytes = 16;
    const float uniform_data[2][4] = {{1,1,0,0}, {0,1,0,1}};
#endif
    for (unsigned i = 0; i < 2; ++i) {
        VkDescriptorSetLayoutBinding binding = {0};
        binding.binding = 0;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        binding.descriptorCount = 1;
        binding.stageFlags = i ? VK_SHADER_STAGE_FRAGMENT_BIT :
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        VkDescriptorSetLayoutCreateInfo ci = {0};
        ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ci.bindingCount = 1;
#ifdef PROBE_DUDE_GENERIC
        if (i) binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
#endif
        ci.pBindings = &binding;
#ifdef PROBE_LIGHTING
        VkDescriptorSetLayoutBinding light_bindings[7]={0};
        const unsigned light_binding_numbers[]={0,1,2,3,4,9,10};
        if (i) {
            for(unsigned b=0;b<7;++b) {
                light_bindings[b]=binding;
                light_bindings[b].binding=light_binding_numbers[b];
            }
            ci.bindingCount=7; ci.pBindings=light_bindings;
        }
#endif
        CHECK(vkCreateDescriptorSetLayout(dev, &ci, NULL, &set_layouts[i]));
#ifdef PROBE_DUDE_GENERIC
        if (i) continue;
#endif
        VkBufferCreateInfo bi = {0};
        bi.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bi.size = uniform_bytes;
        bi.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        CHECK(vkCreateBuffer(dev, &bi, NULL, &uniform_buffers[i]));
        VkMemoryRequirements req;
        vkGetBufferMemoryRequirements(dev, uniform_buffers[i], &req);
        VkMemoryAllocateInfo ai = {0};
        ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        ai.allocationSize = req.size;
        ai.memoryTypeIndex = 0;
        CHECK(vkAllocateMemory(dev, &ai, NULL, &uniform_memory[i]));
        CHECK(vkBindBufferMemory(dev, uniform_buffers[i], uniform_memory[i], 0));
        void *mapped;
        CHECK(vkMapMemory(dev, uniform_memory[i], 0, uniform_bytes, 0, &mapped));
        memcpy(mapped, uniform_data[i], uniform_bytes);
        vkUnmapMemory(dev, uniform_memory[i]);
    }
    VkDescriptorPoolSize pool_sizes[2] = {{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2}, {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 7}};
    VkDescriptorPoolCreateInfo pool_ci = {0};
    pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_ci.maxSets = 2;
    pool_ci.poolSizeCount = 2;
    pool_ci.pPoolSizes = pool_sizes;
    VkDescriptorPool descriptor_pool;
    CHECK(vkCreateDescriptorPool(dev, &pool_ci, NULL, &descriptor_pool));
    VkDescriptorSetAllocateInfo sets_ai = {0};
    sets_ai.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    sets_ai.descriptorPool = descriptor_pool;
    sets_ai.descriptorSetCount = 2;
    sets_ai.pSetLayouts = set_layouts;
    CHECK(vkAllocateDescriptorSets(dev, &sets_ai, sets));
#if defined(PROBE_DUDE_GENERIC) && !defined(PROBE_LIGHTING)
    VkImageCreateInfo image_ci = {0};
    image_ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_ci.imageType = VK_IMAGE_TYPE_2D;
    image_ci.format = VK_FORMAT_R8G8B8A8_UNORM;
    image_ci.extent = (VkExtent3D){2,1,1};
    image_ci.mipLevels = image_ci.arrayLayers = 1;
    image_ci.samples = VK_SAMPLE_COUNT_1_BIT;
    image_ci.tiling = VK_IMAGE_TILING_LINEAR;
    image_ci.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    image_ci.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    CHECK(vkCreateImage(dev, &image_ci, NULL, &texture));
    VkMemoryRequirements texture_req;
    vkGetImageMemoryRequirements(dev, texture, &texture_req);
    VkMemoryAllocateInfo texture_ai = {0};
    texture_ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    texture_ai.allocationSize = texture_req.size;
    CHECK(vkAllocateMemory(dev, &texture_ai, NULL, &texture_memory));
    CHECK(vkBindImageMemory(dev, texture, texture_memory, 0));
    void *pixels;
    CHECK(vkMapMemory(dev, texture_memory, 0, VK_WHOLE_SIZE, 0, &pixels));
    const unsigned char texels[] = {0,255,0,255, 255,0,0,0};
    memcpy(pixels, texels, sizeof(texels));
    vkUnmapMemory(dev, texture_memory);
    VkImageViewCreateInfo texture_vi = {0};
    texture_vi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    texture_vi.image = texture;
    texture_vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
    texture_vi.format = image_ci.format;
    texture_vi.subresourceRange = (VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
    CHECK(vkCreateImageView(dev, &texture_vi, NULL, &texture_view));
    VkSamplerCreateInfo sampler_ci = {0};
    sampler_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_ci.magFilter = sampler_ci.minFilter = VK_FILTER_NEAREST;
    sampler_ci.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_ci.addressModeU = sampler_ci.addressModeV = sampler_ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    CHECK(vkCreateSampler(dev, &sampler_ci, NULL, &texture_sampler));
#endif
#ifdef PROBE_LIGHTING
    if (setup_lighting(dev, sets[1], &lighting)) return 1;
#endif
    for (unsigned i = 0; i < 2; ++i) {
#ifdef PROBE_LIGHTING
        if (i) continue;
#endif
        VkDescriptorBufferInfo buffer = {uniform_buffers[i], 0, uniform_bytes};
        VkWriteDescriptorSet write = {0};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = sets[i];
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.pBufferInfo = &buffer;
#if defined(PROBE_DUDE_GENERIC) && !defined(PROBE_LIGHTING)
        VkDescriptorImageInfo image_info = {texture_sampler, texture_view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        if (i) {
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.pBufferInfo = NULL;
            write.pImageInfo = &image_info;
        }
#endif
        vkUpdateDescriptorSets(dev, 1, &write, 0, NULL);
    }
    VkPipelineLayoutCreateInfo pl_ci = {0};
    pl_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pl_ci.setLayoutCount = 2;
    pl_ci.pSetLayouts = set_layouts;
    VkPipelineLayout pl;
    CHECK(vkCreatePipelineLayout(dev, &pl_ci, NULL, &pl));

    VkGraphicsPipelineCreateInfo gpci = {0};
    gpci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    gpci.stageCount = 2;
    gpci.pStages = stages;
    gpci.pVertexInputState = &vii;
    gpci.pInputAssemblyState = &iai;
    gpci.pViewportState = &vpsi;
    gpci.pRasterizationState = &rsi;
    gpci.pMultisampleState = &msi;
    gpci.pColorBlendState = &cbsi;
    VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic = {0};
    dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic.dynamicStateCount = 2;
    dynamic.pDynamicStates = dynamic_states;
    gpci.pDynamicState = &dynamic;
    gpci.layout = pl;
    gpci.renderPass = rp;
    gpci.subpass = 0;

    VkPipeline pipeline;
    vk_ps4_log_raw("TEST: creating graphics pipeline");
    vr = vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &gpci, NULL, &pipeline);
    if (vr != VK_SUCCESS) {
        printf("vkCreateGraphicsPipelines failed: %d\n", vr);
        vk_ps4_log("TEST: vkCreateGraphicsPipelines FAILED: %d", (int)vr);
        return 1;
    } else {
        printf("Pipeline created OK\n");
        vk_ps4_log_raw("TEST: pipeline OK");
    }

    /* 12. Create command pool + buffer */
    VkCommandPoolCreateInfo cp_ci = {0};
    cp_ci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cp_ci.queueFamilyIndex = 0;
    cp_ci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VkCommandPool cmd_pool;
    CHECK(vkCreateCommandPool(dev, &cp_ci, NULL, &cmd_pool));

    VkCommandBufferAllocateInfo cb_ai = {0};
    cb_ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cb_ai.commandPool = cmd_pool;
    cb_ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cb_ai.commandBufferCount = 1;
    VkCommandBuffer cmd;
    CHECK(vkAllocateCommandBuffers(dev, &cb_ai, &cmd));

    /* 13. Create sync objects */
    VkSemaphoreCreateInfo sem_ci = {0};
    sem_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkSemaphore image_avail, render_done;
    CHECK(vkCreateSemaphore(dev, &sem_ci, NULL, &image_avail));
    CHECK(vkCreateSemaphore(dev, &sem_ci, NULL, &render_done));

    VkFenceCreateInfo fence_ci = {0};
    fence_ci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_ci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VkFence flight_fence;
    CHECK(vkCreateFence(dev, &fence_ci, NULL, &flight_fence));

    /* 14. Render loop — 6 frames (~10 seconds at 60fps) then exit */
    printf("Starting render loop (6 frames)...\n");
    vk_ps4_log_raw("TEST: render loop start (6 frames)");
    for (int frame = 0; frame < 6; frame++) {
        CHECK(vkWaitForFences(dev, 1, &flight_fence, VK_TRUE, 1000000000ULL));
        CHECK(vkResetFences(dev, 1, &flight_fence));

        uint32_t img_idx = 0;
        vr = vkAcquireNextImageKHR(dev, swapchain, 1000000000ULL,
                                   image_avail, VK_NULL_HANDLE, &img_idx);
        if (vr != VK_SUCCESS) {
            printf("vkAcquireNextImageKHR failed: %d (frame %d)\n", vr, frame);
            vk_ps4_log("TEST: AcquireNextImage FAILED frame=%d vr=%d", frame, (int)vr);
            return 1;
        }

        VkCommandBufferBeginInfo cmd_bi = {0};
        cmd_bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        CHECK(vkBeginCommandBuffer(cmd, &cmd_bi));

#ifdef PROBE_LIGHTING
        if (!frame) upload_lighting(cmd, &lighting);
#elif defined(PROBE_DUDE_GENERIC)
        if (!frame) {
            VkImageMemoryBarrier barrier = {0};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = texture;
            barrier.subresourceRange = texture_vi.subresourceRange;
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,NULL,0,NULL,1,&barrier);
        }
#endif
        VkRenderPassBeginInfo rpbi = {0};
        rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpbi.renderPass = rp;
        rpbi.framebuffer = fbs[img_idx];
        rpbi.renderArea.offset = (VkOffset2D){0, 0};
        rpbi.renderArea.extent = (VkExtent2D){1280, 720};
        VkClearValue clear = {0};
        clear.color.float32[0] = 0.1f;
        clear.color.float32[1] = 0.1f;
        clear.color.float32[2] = 0.2f;
        clear.color.float32[3] = 1.0f;
        rpbi.clearValueCount = 1;
        rpbi.pClearValues = &clear;

        vkCmdBeginRenderPass(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        vkCmdSetViewport(cmd, 0, 1, &vp);
        vkCmdSetScissor(cmd, 0, 1, &sc);
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &vbuf, &offset);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pl, 0, 1, &sets[0], 0, NULL);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pl, 1, 1, &sets[1], 0, NULL);
        vkCmdDraw(cmd, 3, 1, 0, 0);
        vkCmdEndRenderPass(cmd);

        CHECK(vkEndCommandBuffer(cmd));

        VkSubmitInfo submit = {0};
        submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &image_avail;
        VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        submit.pWaitDstStageMask = &wait_stage;
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &cmd;
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &render_done;

        vr = vkQueueSubmit(queue, 1, &submit, flight_fence);
        if (vr != VK_SUCCESS) {
            printf("vkQueueSubmit failed: %d (frame %d)\n", vr, frame);
            vk_ps4_log("TEST: QueueSubmit FAILED frame=%d vr=%d", frame, (int)vr);
            return 1;
        }

        VkPresentInfoKHR present = {0};
        present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present.waitSemaphoreCount = 1;
        present.pWaitSemaphores = &render_done;
        present.swapchainCount = 1;
        present.pSwapchains = &swapchain;
        present.pImageIndices = &img_idx;

        vr = vkQueuePresentKHR(queue, &present);
        if (vr != VK_SUCCESS) {
            printf("vkQueuePresentKHR failed: %d (frame %d)\n", vr, frame);
            vk_ps4_log("TEST: QueuePresent FAILED frame=%d vr=%d", frame, (int)vr);
            return 1;
        }

        if (frame % 60 == 0) {
            printf("  Frame %d OK\n", frame);
            vk_ps4_log("TEST: frame %d OK", frame);
        }
    }
    CHECK(vkQueueWaitIdle(queue));
#ifdef PROBE_LIGHTING
    vk_ps4_log_raw("PROBE CAPTURE READY: cube/ambient lighting");
#elif defined(PROBE_DUDE_GENERIC)
    vk_ps4_log_raw("PROBE CAPTURE READY: DUDE generic texture and alpha test");
#else
    vk_ps4_log_raw("PROBE CAPTURE READY: demote derivative and two sets");
#endif
    sleep(5);
    printf("Render loop done\n");
    vk_ps4_log_raw("TEST: render loop done");

    /* 15. Cleanup */
    vk_ps4_log_raw("TEST: cleanup start");
    CHECK(vkDeviceWaitIdle(dev));
    vkDestroyFence(dev, flight_fence, NULL);
    vkDestroySemaphore(dev, image_avail, NULL);
    vkDestroySemaphore(dev, render_done, NULL);
    vkDestroyCommandPool(dev, cmd_pool, NULL);
    vkDestroyPipeline(dev, pipeline, NULL);
#ifdef PROBE_LIGHTING
    destroy_lighting(dev, &lighting);
#elif defined(PROBE_DUDE_GENERIC)
    vkDestroySampler(dev, texture_sampler, NULL);
    vkDestroyImageView(dev, texture_view, NULL);
    vkDestroyImage(dev, texture, NULL);
    vkFreeMemory(dev, texture_memory, NULL);
#endif
    vkDestroyDescriptorPool(dev, descriptor_pool, NULL);
    for (unsigned i = 0; i < 2; ++i) {
        vkDestroyDescriptorSetLayout(dev, set_layouts[i], NULL);
        vkDestroyBuffer(dev, uniform_buffers[i], NULL);
        vkFreeMemory(dev, uniform_memory[i], NULL);
    }
    vkDestroyPipelineLayout(dev, pl, NULL);
    vkDestroyShaderModule(dev, vs_mod, NULL);
    vkDestroyShaderModule(dev, fs_mod, NULL);
    vkFreeMemory(dev, vbuf_mem, NULL);
    vkDestroyBuffer(dev, vbuf, NULL);
    for (uint32_t i = 0; i < sw_img_count; i++) {
        vkDestroyFramebuffer(dev, fbs[i], NULL);
        vkDestroyImageView(dev, sw_views[i], NULL);
    }
    free(fbs);
    free(sw_views);
    free(sw_images);
    vkDestroyRenderPass(dev, rp, NULL);
    vkDestroySwapchainKHR(dev, swapchain, NULL);
    vkDestroyDevice(dev, NULL);
    vkDestroyInstance(inst, NULL);
    vk_ps4_log_raw("TEST: cleanup done");
    vk_ps4_log_close();

    printf("=== Test complete ===\n");
    return 0;
}
