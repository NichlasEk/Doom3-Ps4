/* Real two-pass geometry fixture: draw near then far overlapping quads into
 * D32, then let the original DUDE interaction shader sample that attachment. */
typedef struct ShadowCaster {
    VkRenderPass pass;
    VkFramebuffer framebuffer;
    VkPipeline pipeline;
    VkShaderModule shaders[2];
    VkBuffer vertices;
    VkDeviceMemory memory;
#ifdef PROBE_DEPTH_COPY
    VkImage source;
    VkImageView source_view;
    VkDeviceMemory source_memory;
#endif
} ShadowCaster;
static int setup_shadow_caster(VkDevice dev, VkImageView view,
                              const VkGraphicsPipelineCreateInfo *base, ShadowCaster *r) {
#ifdef PROBE_DEPTH_COPY
    VkImageCreateInfo ci={0}; ci.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ci.imageType=VK_IMAGE_TYPE_2D; ci.format=VK_FORMAT_D32_SFLOAT;
#ifdef PROBE_DEPTH_STENCIL
    ci.format=VK_FORMAT_D32_SFLOAT_S8_UINT;
#endif
    ci.extent=(VkExtent3D){128,64,1}; ci.mipLevels=ci.arrayLayers=1;
#ifdef PROBE_DEPTH_REGION
    ci.extent=(VkExtent3D){160,96,1};
#endif
    ci.samples=VK_SAMPLE_COUNT_1_BIT; ci.tiling=VK_IMAGE_TILING_OPTIMAL;
    ci.usage=VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
    CHECK(vkCreateImage(dev,&ci,NULL,&r->source));
    VkMemoryRequirements req0; vkGetImageMemoryRequirements(dev,r->source,&req0);
    VkMemoryAllocateInfo ai0={0}; ai0.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO; ai0.allocationSize=req0.size;
    CHECK(vkAllocateMemory(dev,&ai0,NULL,&r->source_memory));
    CHECK(vkBindImageMemory(dev,r->source,r->source_memory,0));
    VkImageViewCreateInfo vi0={0}; vi0.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    vi0.image=r->source; vi0.viewType=VK_IMAGE_VIEW_TYPE_2D; vi0.format=ci.format;
    vi0.subresourceRange=(VkImageSubresourceRange){VK_IMAGE_ASPECT_DEPTH_BIT,0,1,0,1};
    CHECK(vkCreateImageView(dev,&vi0,NULL,&r->source_view));
    view=r->source_view;
#endif
    VkAttachmentDescription attachment={0};
    attachment.format=VK_FORMAT_D32_SFLOAT;
#ifdef PROBE_DEPTH_STENCIL
    attachment.format=VK_FORMAT_D32_SFLOAT_S8_UINT;
#endif
    attachment.samples=VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout=attachment.finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkAttachmentReference reference={0,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass={0}; subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.pDepthStencilAttachment=&reference;
    VkRenderPassCreateInfo pass={0}; pass.sType=VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    pass.attachmentCount=1; pass.pAttachments=&attachment;
    pass.subpassCount=1; pass.pSubpasses=&subpass;
    CHECK(vkCreateRenderPass(dev,&pass,NULL,&r->pass));
    VkFramebufferCreateInfo fb={0}; fb.sType=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb.renderPass=r->pass; fb.attachmentCount=1; fb.pAttachments=&view;
    fb.width=128; fb.height=64; fb.layers=1;
#ifdef PROBE_DEPTH_REGION
    fb.width=160; fb.height=96;
#endif
    CHECK(vkCreateFramebuffer(dev,&fb,NULL,&r->framebuffer));
    VkPipelineShaderStageCreateInfo stages[2]={0};
    for(unsigned i=0;i<2;++i) {
        VkShaderModuleCreateInfo shader={0}; shader.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        shader.pCode=i?g_caster_frag_spv:g_caster_vert_spv;
        shader.codeSize=i?sizeof(g_caster_frag_spv):sizeof(g_caster_vert_spv);
        CHECK(vkCreateShaderModule(dev,&shader,NULL,&r->shaders[i]));
        stages[i].sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[i].stage=i?VK_SHADER_STAGE_FRAGMENT_BIT:VK_SHADER_STAGE_VERTEX_BIT;
        stages[i].module=r->shaders[i]; stages[i].pName="main";
    }
    VkVertexInputBindingDescription binding={0,3*sizeof(float),VK_VERTEX_INPUT_RATE_VERTEX};
    VkVertexInputAttributeDescription attribute={0,0,VK_FORMAT_R32G32B32_SFLOAT,0};
    VkPipelineVertexInputStateCreateInfo vi={0}; vi.sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi.vertexBindingDescriptionCount=1; vi.pVertexBindingDescriptions=&binding;
    vi.vertexAttributeDescriptionCount=1; vi.pVertexAttributeDescriptions=&attribute;
    VkPipelineDepthStencilStateCreateInfo ds={0}; ds.sType=VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.depthTestEnable=ds.depthWriteEnable=VK_TRUE; ds.depthCompareOp=VK_COMPARE_OP_LESS;
    VkPipelineColorBlendStateCreateInfo blend={0}; blend.sType=VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    VkGraphicsPipelineCreateInfo pipeline=*base;
    pipeline.renderPass=r->pass; pipeline.pStages=stages; pipeline.pVertexInputState=&vi;
    pipeline.pDepthStencilState=&ds; pipeline.pColorBlendState=&blend;
    CHECK(vkCreateGraphicsPipelines(dev,VK_NULL_HANDLE,1,&pipeline,NULL,&r->pipeline));
    const float vertices[]={
        -.5,-.5,.25, .5,-.5,.25, .5,.5,.25,
        -.5,-.5,.25, .5,.5,.25, -.5,.5,.25,
        -.5,-.5,.75, .5,-.5,.75, .5,.5,.75,
        -.5,-.5,.75, .5,.5,.75, -.5,.5,.75};
    VkBufferCreateInfo buffer={0}; buffer.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer.size=sizeof(vertices); buffer.usage=VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    CHECK(vkCreateBuffer(dev,&buffer,NULL,&r->vertices));
    VkMemoryRequirements req; vkGetBufferMemoryRequirements(dev,r->vertices,&req);
    VkMemoryAllocateInfo ai={0}; ai.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO; ai.allocationSize=req.size;
    CHECK(vkAllocateMemory(dev,&ai,NULL,&r->memory));
    CHECK(vkBindBufferMemory(dev,r->vertices,r->memory,0));
    void *mapped; CHECK(vkMapMemory(dev,r->memory,0,VK_WHOLE_SIZE,0,&mapped));
    memcpy(mapped,vertices,sizeof(vertices)); vkUnmapMemory(dev,r->memory);
    return 0;
}
static void draw_shadow_caster(VkCommandBuffer cmd, VkImage image, const ShadowCaster *r, unsigned frame) {
    VkImageMemoryBarrier barrier={0}; barrier.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image=image;
#ifdef PROBE_DEPTH_COPY
    barrier.image=r->source;
#endif
    barrier.subresourceRange=(VkImageSubresourceRange){VK_IMAGE_ASPECT_DEPTH_BIT,0,1,0,1};
    barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
    barrier.oldLayout=frame?VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    barrier.srcAccessMask=frame?VK_ACCESS_SHADER_READ_BIT:0;
    barrier.dstAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,0,0,NULL,0,NULL,1,&barrier);
    VkViewport vp={0,0,128,64,0,1}; VkRect2D sc={{0,0},{128,64}};
    /* A load-op clear must ignore unrelated dynamic draw state. */
    VkViewport unrelated={0,0,1,1,0,1};
    vkCmdSetViewport(cmd,0,1,&unrelated); vkCmdSetScissor(cmd,0,1,&sc);
    VkClearValue clear={.depthStencil={1,0}};
    VkRenderPassBeginInfo begin={0}; begin.sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    begin.renderPass=r->pass; begin.framebuffer=r->framebuffer; begin.renderArea=sc;
#ifdef PROBE_DEPTH_REGION
    begin.renderArea.extent=(VkExtent2D){160,96};
#endif
    begin.clearValueCount=1; begin.pClearValues=&clear;
    vkCmdBeginRenderPass(cmd,&begin,VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,r->pipeline);
    /* Move the projection every other frame. The next clear must remove the
     * prior rectangle, including on the final captured frame (frame 5). */
    vp.x=(frame&1)?16:0;
    vkCmdSetViewport(cmd,0,1,&vp); vkCmdSetScissor(cmd,0,1,&sc);
    VkDeviceSize offset=0; vkCmdBindVertexBuffers(cmd,0,1,&r->vertices,&offset);
    vkCmdDraw(cmd,12,1,0,0);
    vkCmdEndRenderPass(cmd);
#ifdef PROBE_DEPTH_COPY
    barrier.oldLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    barrier.newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
    vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,NULL,0,NULL,1,&barrier);
    VkImageMemoryBarrier dst=barrier; dst.image=image;
    dst.oldLayout=frame?VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:VK_IMAGE_LAYOUT_UNDEFINED;
    dst.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    dst.srcAccessMask=frame?VK_ACCESS_SHADER_READ_BIT:0; dst.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,NULL,0,NULL,1,&dst);
    VkImageCopy copy={0}; copy.srcSubresource=copy.dstSubresource=(VkImageSubresourceLayers){VK_IMAGE_ASPECT_DEPTH_BIT,0,0,1};
    copy.extent=(VkExtent3D){128,64,1};
    vkCmdCopyImage(cmd,r->source,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
#ifdef PROBE_DEPTH_REGION
    /* Repeat with nonzero source/destination offsets after the full copy.
     * This checks different pitches, untouched pixels and immutable params. */
    VkMemoryBarrier order={VK_STRUCTURE_TYPE_MEMORY_BARRIER,NULL,VK_ACCESS_TRANSFER_WRITE_BIT,VK_ACCESS_TRANSFER_WRITE_BIT};
    vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,1,&order,0,NULL,0,NULL);
    copy.srcOffset=(VkOffset3D){8,4,0}; copy.dstOffset=(VkOffset3D){16,32,0};
    copy.extent=(VkExtent3D){112,32,1};
    vkCmdCopyImage(cmd,r->source,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy);
#endif
    dst.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; dst.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    dst.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT; dst.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,NULL,0,NULL,1,&dst);
    barrier.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
#else
    barrier.oldLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
#endif
    barrier.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask=VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT|VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,NULL,0,NULL,1,&barrier);
}
static void destroy_shadow_caster(VkDevice dev, ShadowCaster *r) {
    vkDestroyPipeline(dev,r->pipeline,NULL);
    for(unsigned i=0;i<2;++i) vkDestroyShaderModule(dev,r->shaders[i],NULL);
    vkDestroyFramebuffer(dev,r->framebuffer,NULL); vkDestroyRenderPass(dev,r->pass,NULL);
#ifdef PROBE_DEPTH_COPY
    vkDestroyImageView(dev,r->source_view,NULL);
    vkDestroyImage(dev,r->source,NULL); vkFreeMemory(dev,r->source_memory,NULL);
#endif

    vkDestroyBuffer(dev,r->vertices,NULL); vkFreeMemory(dev,r->memory,NULL);
}
