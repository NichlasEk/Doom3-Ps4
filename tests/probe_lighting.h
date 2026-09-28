/* Synthetic textures for cube sampling and unmodified DUDE lighting shaders.
 * Depth cubes are uploaded data. The shadowcast variant renders the 2D map. */
#ifdef PROBE_INTERACTION
#define LIGHT_IMAGES 15
#else
#define LIGHT_IMAGES 7
#endif
typedef struct LightingResources {
    VkImage images[LIGHT_IMAGES];
    VkDeviceMemory memory[LIGHT_IMAGES];
    VkImageView views[LIGHT_IMAGES];
    VkSampler sampler;
    VkSampler shadow_sampler;
    VkBuffer staging;
    VkDeviceMemory staging_memory;
} LightingResources;
static int setup_lighting(VkDevice dev, VkDescriptorSet set, LightingResources *r) {
#ifdef PROBE_INTERACTION
    const unsigned bindings[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14};
#else
    const unsigned bindings[] = {0,1,2,3,4,9,10};
#endif
    unsigned char faces[6][4] = {
        {255,0,0,255},{0,255,0,255},{0,0,255,255},
        {255,255,0,255},{0,255,255,255},{255,0,255,255}
    };
#ifdef PROBE_INTERACTION
    for(unsigned f=0;f<6;++f) { faces[f][0]=128; faces[f][1]=128; faces[f][2]=255; faces[f][3]=255; }
#endif
    VkBufferCreateInfo buffer = {0};
    buffer.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer.size = sizeof(faces)*2;
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
    const float depths[6]={.25f,.75f,.25f,.75f,.25f,.75f};
    memcpy((char*)mapped+sizeof(faces),depths,sizeof(depths));
    vkUnmapMemory(dev,r->staging_memory);
    VkSamplerCreateInfo sampler = {0};
    sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler.magFilter = sampler.minFilter = VK_FILTER_NEAREST;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler.addressModeU = sampler.addressModeV = sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    CHECK(vkCreateSampler(dev,&sampler,NULL,&r->sampler));
    sampler.compareEnable=VK_TRUE; sampler.compareOp=VK_COMPARE_OP_LESS;
    CHECK(vkCreateSampler(dev,&sampler,NULL,&r->shadow_sampler));
    for (unsigned i=0; i<LIGHT_IMAGES; ++i) {
        bool cube = i==0;
        bool depth = false;
#ifdef PROBE_INTERACTION
        cube = cube || i==8 || i==12;
        depth = i==7 || i==8 || i==12;
#endif
        VkImageCreateInfo ci = {0}; ci.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        ci.imageType = VK_IMAGE_TYPE_2D; ci.format = VK_FORMAT_R8G8B8A8_UNORM;
        if(depth) ci.format=VK_FORMAT_D32_SFLOAT;
        ci.extent = (VkExtent3D){1,1,1}; ci.mipLevels=1; ci.arrayLayers=cube?6:1;
        ci.samples=VK_SAMPLE_COUNT_1_BIT;
        ci.tiling=cube?VK_IMAGE_TILING_OPTIMAL:VK_IMAGE_TILING_LINEAR;
        ci.usage=VK_IMAGE_USAGE_SAMPLED_BIT | (cube?VK_IMAGE_USAGE_TRANSFER_DST_BIT:0);
        ci.flags=cube?VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT:0;
        ci.initialLayout=cube?VK_IMAGE_LAYOUT_UNDEFINED:VK_IMAGE_LAYOUT_PREINITIALIZED;
#ifdef PROBE_SHADOW_CAST
        if (i==7) {
            ci.extent=(VkExtent3D){128,64,1};
            ci.tiling=VK_IMAGE_TILING_OPTIMAL;
            ci.usage=VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT;
            ci.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
            for(unsigned bad=0;bad<3;++bad) {
                VkImageCreateInfo unsupported=ci;
                if(bad==0) unsupported.format=VK_FORMAT_D16_UNORM;
                if(bad==1) unsupported.arrayLayers=2;
                if(bad==2) unsupported.mipLevels=2;
                VkImage rejected=VK_NULL_HANDLE;
                if(vkCreateImage(dev,&unsupported,NULL,&rejected)!=VK_ERROR_FEATURE_NOT_PRESENT) {
                    vk_ps4_log_raw("FAIL: unsupported sampled depth attachment accepted"); return 1;
                }
            }
        }
#endif
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
        if (!cube
#ifdef PROBE_SHADOW_CAST
            && i!=7
#endif
        ) {
            const unsigned char white[]={255,255,255,255};
            const unsigned char normal[]={0,128,255,128};
            const unsigned char diffuse[]={128,128,128,255};
            CHECK(vkMapMemory(dev,r->memory[i],0,VK_WHOLE_SIZE,0,&mapped));
            if(depth) { const float d=.5f; memcpy(mapped,&d,4); }
            else memcpy(mapped,i==1?normal:(i==4?diffuse:white),4);
            vkUnmapMemory(dev,r->memory[i]);
        }
        VkImageViewCreateInfo view={0}; view.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view.image=r->images[i]; view.viewType=cube?VK_IMAGE_VIEW_TYPE_CUBE:VK_IMAGE_VIEW_TYPE_2D;
        view.format=ci.format;
        view.subresourceRange=(VkImageSubresourceRange){depth?VK_IMAGE_ASPECT_DEPTH_BIT:VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,cube?6:1};
        if (!i) {
            VkImageViewCreateInfo unsupported = view;
            unsupported.viewType = VK_IMAGE_VIEW_TYPE_2D;
            unsupported.subresourceRange.layerCount = 1;
            VkImageView rejected = VK_NULL_HANDLE;
            if (vkCreateImageView(dev,&unsupported,NULL,&rejected) != VK_ERROR_FEATURE_NOT_PRESENT) {
                vk_ps4_log_raw("FAIL: cube face view must remain unsupported"); return 1;
            }
        }
#ifdef PROBE_SHADOW_CAST
        if(i==7) {
            VkImageViewCreateInfo unsupported=view;
            unsupported.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
            VkImageView rejected=VK_NULL_HANDLE;
            if(vkCreateImageView(dev,&unsupported,NULL,&rejected)!=VK_ERROR_FEATURE_NOT_PRESENT) {
                vk_ps4_log_raw("FAIL: color view of sampled depth attachment accepted"); return 1;
            }
        }
#endif
        CHECK(vkCreateImageView(dev,&view,NULL,&r->views[i]));
        VkDescriptorImageInfo info={depth?r->shadow_sampler:r->sampler,r->views[i],VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        VkWriteDescriptorSet write={0}; write.sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet=set; write.dstBinding=bindings[i]; write.descriptorCount=1;
        write.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; write.pImageInfo=&info;
        vkUpdateDescriptorSets(dev,1,&write,0,NULL);
    }
    return 0;
}
static void upload_lighting(VkCommandBuffer cmd, const LightingResources *r) {
    for (unsigned i=0; i<LIGHT_IMAGES; ++i) {
#ifdef PROBE_SHADOW_CAST
        if(i==7) continue; /* Rendered by the caster pass. */
#endif
        bool cube = i==0, depth=false;
#ifdef PROBE_INTERACTION
        cube = cube || i==8 || i==12;
        depth = i==7 || i==8 || i==12;
#endif
        VkImageMemoryBarrier b={0}; b.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        b.srcAccessMask=cube?0:VK_ACCESS_HOST_WRITE_BIT;
        b.dstAccessMask=cube?VK_ACCESS_TRANSFER_WRITE_BIT:VK_ACCESS_SHADER_READ_BIT;
        b.oldLayout=cube?VK_IMAGE_LAYOUT_UNDEFINED:VK_IMAGE_LAYOUT_PREINITIALIZED;
        b.newLayout=cube?VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
        b.image=r->images[i];
        b.subresourceRange=(VkImageSubresourceRange){depth?VK_IMAGE_ASPECT_DEPTH_BIT:VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,cube?6:1};
        vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT|VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,NULL,0,NULL,1,&b);
        if(!cube) continue;
        VkBufferImageCopy regions[6]={0};
        for(unsigned f=0;f<6;++f) {
            regions[f].bufferOffset=4*f+(depth?24:0);
            regions[f].imageSubresource=(VkImageSubresourceLayers){b.subresourceRange.aspectMask,0,f,1};
            regions[f].imageExtent=(VkExtent3D){1,1,1};
        }
        vkCmdCopyBufferToImage(cmd,r->staging,r->images[i],VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,6,regions);
        b.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT; b.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
        b.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; b.newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,0,0,NULL,0,NULL,1,&b);
    }
}
static void destroy_lighting(VkDevice dev, LightingResources *r) {
    for(unsigned i=0;i<LIGHT_IMAGES;++i) {
        vkDestroyImageView(dev,r->views[i],NULL);
        vkDestroyImage(dev,r->images[i],NULL);
        vkFreeMemory(dev,r->memory[i],NULL);
    }
    vkDestroySampler(dev,r->sampler,NULL);
    vkDestroySampler(dev,r->shadow_sampler,NULL);
    vkDestroyBuffer(dev,r->staging,NULL);
    vkFreeMemory(dev,r->staging_memory,NULL);
}
