/* Resource fixture for the cube and unmodified DUDE ambient-light probes. */
typedef struct LightingResources {
    VkImage images[7];
    VkDeviceMemory memory[7];
    VkImageView views[7];
    VkSampler sampler;
    VkBuffer staging;
    VkDeviceMemory staging_memory;
} LightingResources;
static int setup_lighting(VkDevice dev, VkDescriptorSet set, LightingResources *r) {
    const unsigned bindings[] = {0,1,2,3,4,9,10};
    const unsigned char faces[6][4] = {
        {255,0,0,255},{0,255,0,255},{0,0,255,255},
        {255,255,0,255},{0,255,255,255},{255,0,255,255}
    };
    VkBufferCreateInfo buffer = {0};
    buffer.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer.size = sizeof(faces);
    buffer.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    CHECK(vkCreateBuffer(dev, &buffer, NULL, &r->staging));
    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(dev, r->staging, &req);
    VkMemoryAllocateInfo ai = {0}; ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    ai.allocationSize = req.size;
    CHECK(vkAllocateMemory(dev, &ai, NULL, &r->staging_memory));
    CHECK(vkBindBufferMemory(dev, r->staging, r->staging_memory, 0));
    void *mapped;
    CHECK(vkMapMemory(dev,r->staging_memory,0,VK_WHOLE_SIZE,0,&mapped));
    memcpy(mapped,faces,sizeof(faces));
    vkUnmapMemory(dev,r->staging_memory);
    VkSamplerCreateInfo sampler = {0};
    sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler.magFilter = sampler.minFilter = VK_FILTER_NEAREST;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler.addressModeU = sampler.addressModeV = sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    CHECK(vkCreateSampler(dev,&sampler,NULL,&r->sampler));
    for (unsigned i=0; i<7; ++i) {
        VkImageCreateInfo ci = {0}; ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        ci.imageType = VK_IMAGE_TYPE_2D; ci.format = VK_FORMAT_R8G8B8A8_UNORM;
        ci.extent = (VkExtent3D){1,1,1}; ci.mipLevels=1; ci.arrayLayers=i?1:6;
        ci.samples=VK_SAMPLE_COUNT_1_BIT;
        ci.tiling=i?VK_IMAGE_TILING_LINEAR:VK_IMAGE_TILING_OPTIMAL;
        ci.usage=VK_IMAGE_USAGE_SAMPLED_BIT | (i?0:VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        ci.flags=i?0:VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        ci.initialLayout=i?VK_IMAGE_LAYOUT_PREINITIALIZED:VK_IMAGE_LAYOUT_UNDEFINED;
        if (!i) {
            VkImageCreateInfo unsupported = ci;
            unsupported.arrayLayers = 12;
            VkImage rejected = VK_NULL_HANDLE;
            if (vkCreateImage(dev,&unsupported,NULL,&rejected) != VK_ERROR_FEATURE_NOT_PRESENT) {
                vk_ps4_log_raw("FAIL: cube array must remain unsupported"); return 1;
            }
        }
        CHECK(vkCreateImage(dev,&ci,NULL,&r->images[i]));
        vkGetImageMemoryRequirements(dev,r->images[i],&req);
        ai.allocationSize=req.size;
        CHECK(vkAllocateMemory(dev,&ai,NULL,&r->memory[i]));
        CHECK(vkBindImageMemory(dev,r->images[i],r->memory[i],0));
        if (i) {
            const unsigned char white[]={255,255,255,255};
            const unsigned char normal[]={0,128,255,128};
            const unsigned char diffuse[]={128,128,128,255};
            CHECK(vkMapMemory(dev,r->memory[i],0,VK_WHOLE_SIZE,0,&mapped));
            memcpy(mapped,i==1?normal:(i==4?diffuse:white),4);
            vkUnmapMemory(dev,r->memory[i]);
        }
        VkImageViewCreateInfo view={0}; view.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view.image=r->images[i]; view.viewType=i?VK_IMAGE_VIEW_TYPE_2D:VK_IMAGE_VIEW_TYPE_CUBE;
        view.format=ci.format;
        view.subresourceRange=(VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,i?1:6};
        if (!i) {
            VkImageViewCreateInfo unsupported = view;
            unsupported.viewType = VK_IMAGE_VIEW_TYPE_2D;
            unsupported.subresourceRange.layerCount = 1;
            VkImageView rejected = VK_NULL_HANDLE;
            if (vkCreateImageView(dev,&unsupported,NULL,&rejected) != VK_ERROR_FEATURE_NOT_PRESENT) {
                vk_ps4_log_raw("FAIL: cube face view must remain unsupported"); return 1;
            }
        }
        CHECK(vkCreateImageView(dev,&view,NULL,&r->views[i]));
        VkDescriptorImageInfo info={r->sampler,r->views[i],VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet write={0}; write.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet=set; write.dstBinding=bindings[i]; write.descriptorCount=1;
        write.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; write.pImageInfo=&info;
        vkUpdateDescriptorSets(dev,1,&write,0,NULL);
    }
    return 0;
}
static void upload_lighting(VkCommandBuffer cmd, const LightingResources *r) {
    VkImageMemoryBarrier barriers[7] = {0};
    for (unsigned i=0; i<7; ++i) {
        barriers[i].sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barriers[i].srcAccessMask=i?VK_ACCESS_HOST_WRITE_BIT:0;
        barriers[i].dstAccessMask=i?VK_ACCESS_SHADER_READ_BIT:VK_ACCESS_TRANSFER_WRITE_BIT;
        barriers[i].oldLayout=i?VK_IMAGE_LAYOUT_PREINITIALIZED:VK_IMAGE_LAYOUT_UNDEFINED;
        barriers[i].newLayout=i?VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barriers[i].srcQueueFamilyIndex=barriers[i].dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        barriers[i].image=r->images[i];
        barriers[i].subresourceRange=(VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,i?1:6};
    }
    vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,NULL,0,NULL,7,barriers);
    VkBufferImageCopy regions[6]={0};
    for (unsigned i=0;i<6;++i) {
        regions[i].bufferOffset=4*i;
        regions[i].imageSubresource=(VkImageSubresourceLayers){VK_IMAGE_ASPECT_COLOR_BIT,0,i,1};
        regions[i].imageExtent=(VkExtent3D){1,1,1};
    }
    vkCmdCopyBufferToImage(cmd,r->staging,r->images[0],VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,6,regions);
    barriers[0].srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
    barriers[0].dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
    barriers[0].oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barriers[0].newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,NULL,0,NULL,1,barriers);
}
static void destroy_lighting(VkDevice dev, LightingResources *r) {
    for(unsigned i=0;i<7;++i) {
        vkDestroyImageView(dev,r->views[i],NULL);
        vkDestroyImage(dev,r->images[i],NULL);
        vkFreeMemory(dev,r->memory[i],NULL);
    }
    vkDestroySampler(dev,r->sampler,NULL);
    vkDestroyBuffer(dev,r->staging,NULL);
    vkFreeMemory(dev,r->staging_memory,NULL);
}
