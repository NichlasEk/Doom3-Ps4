/* Compare GPU-written backing bytes against the unchanged generic uploader.
 * Two updates per shape, all mips, padded dimensions, nonzero binding/staging
 * offsets and prefix/suffix guards. This oracle is independent of screenshot
 * sampling and catches wrong mip offsets, missing copies and lifetime errors. */
#include "texture_upload.h"
static int probe_texture_upload(VkDevice dev, VkQueue queue, VkCommandBuffer cmd) {
    const unsigned shapes[][2]={{1,1},{3,5},{8,8},{17,9},{513,509},{1024,1},{1,1024},{256,128},{1024,1024},{1024,512},{512,1024}};
    if(doom3_ps4_texture_bytes(VK_NULL_HANDLE)) return 1;
    for(unsigned reject=0;reject<2;++reject) {
        VkImageCreateInfo ci={0};ci.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;ci.imageType=VK_IMAGE_TYPE_2D;
        ci.format=reject?VK_FORMAT_R8G8B8A8_UNORM:VK_FORMAT_R8G8B8A8_SRGB;
        ci.extent=(VkExtent3D){reject?1025:8,8,1};ci.mipLevels=ci.arrayLayers=1;
        ci.samples=VK_SAMPLE_COUNT_1_BIT;ci.tiling=VK_IMAGE_TILING_OPTIMAL;
        ci.usage=VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        VkImage image;VkDeviceMemory memory;VkMemoryRequirements req;
        CHECK(vkCreateImage(dev,&ci,NULL,&image));vkGetImageMemoryRequirements(dev,image,&req);
        VkMemoryAllocateInfo ai={0};ai.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;ai.allocationSize=req.size;
        CHECK(vkAllocateMemory(dev,&ai,NULL,&memory));CHECK(vkBindImageMemory(dev,image,memory,0));
        if(doom3_ps4_texture_bytes(image)) return 1;
        vkDestroyImage(dev,image,NULL);vkFreeMemory(dev,memory,NULL);
    }
    unsigned long long checked=0;
    for (unsigned heap=0;heap<2;++heap) for (unsigned shape=0; shape<sizeof(shapes)/sizeof(shapes[0]); ++shape) {
        unsigned w=shapes[shape][0],h=shapes[shape][1],count=1;
        for (unsigned m=w>h?w:h; m>1; m>>=1) ++count;
        VkImage images[2]; VkDeviceMemory memory[2]; unsigned char *mapped[2];
        VkMemoryRequirements req={0}; VkImageCreateInfo ci={0};
        ci.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;ci.imageType=VK_IMAGE_TYPE_2D;
        ci.format=VK_FORMAT_R8G8B8A8_UNORM;ci.extent=(VkExtent3D){w,h,1};
        ci.mipLevels=count;ci.arrayLayers=1;ci.samples=VK_SAMPLE_COUNT_1_BIT;
        ci.tiling=VK_IMAGE_TILING_OPTIMAL;ci.usage=VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        for (unsigned i=0;i<2;++i) {
            CHECK(vkCreateImage(dev,&ci,NULL,&images[i]));vkGetImageMemoryRequirements(dev,images[i],&req);
            VkMemoryAllocateInfo ai={0};ai.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            ai.allocationSize=req.size+2*req.alignment;ai.memoryTypeIndex=heap;
            CHECK(vkAllocateMemory(dev,&ai,NULL,&memory[i]));
            CHECK(vkBindImageMemory(dev,images[i],memory[i],req.alignment));
            CHECK(vkMapMemory(dev,memory[i],0,VK_WHOLE_SIZE,0,(void**)&mapped[i]));
            memset(mapped[i],0xcd,ai.allocationSize);
        }
        size_t bytes=doom3_ps4_texture_bytes(images[1]);
        if (!bytes || bytes>req.size) {vk_ps4_log("FAIL static texture layout %ux%u",w,h);return 1;}
        VkBuffer buffers[2];VkDeviceMemory bm[2];unsigned char *bp[2];
        for(unsigned i=0;i<2;++i) {
            VkBufferCreateInfo bi={0};bi.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bi.size=bytes+32;bi.usage=VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            CHECK(vkCreateBuffer(dev,&bi,NULL,&buffers[i]));VkMemoryRequirements br;
            vkGetBufferMemoryRequirements(dev,buffers[i],&br);
            VkMemoryAllocateInfo ai={0};ai.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;ai.allocationSize=br.size;
            CHECK(vkAllocateMemory(dev,&ai,NULL,&bm[i]));CHECK(vkBindBufferMemory(dev,buffers[i],bm[i],0));
            CHECK(vkMapMemory(dev,bm[i],0,VK_WHOLE_SIZE,0,(void**)&bp[i]));
        }
        VkFenceCreateInfo fi={0};fi.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;VkFence fence;
        CHECK(vkCreateFence(dev,&fi,NULL,&fence));
        for (unsigned pass=0;pass<2;++pass) {
            for(unsigned i=0;i<2;++i) {memset(mapped[i]+req.alignment,0,req.size);memset(bp[i],0xa7,bytes+32);}
            memset(bp[1]+16,0,bytes);
            VkBufferImageCopy regions[11]={0};size_t linear=0;
            for(unsigned mip=0;mip<count;++mip) {
                unsigned mw=w>>mip?w>>mip:1,mh=h>>mip?h>>mip:1;
                size_t n=(size_t)mw*mh*4;
                if (linear+n>bytes) return 1;
                for(size_t j=0;j<n;++j) bp[0][16+linear+j]=(unsigned char)(j*37+(j/4/mw)*71+mip*53+pass*113+1);
                if(!doom3_ps4_pack_texture_mip_cached(images[1],mip,mw,mh,bp[0]+16+linear,bp[1]+16,bytes)) return 1;
                regions[mip].bufferOffset=16+linear;
                regions[mip].imageSubresource=(VkImageSubresourceLayers){VK_IMAGE_ASPECT_COLOR_BIT,mip,0,1};
                regions[mip].imageExtent=(VkExtent3D){mw,mh,1};linear+=n;
            }
            if(doom3_ps4_pack_texture_mip_cached(images[1],count,w,h,bp[0],bp[1]+16,bytes) ||
               doom3_ps4_pack_texture_mip_cached(images[1],0,w,h,bp[0],bp[1]+16,bytes-1) ||
               doom3_ps4_pack_texture_mip_cached(images[1],0,w+1,h,bp[0],bp[1]+16,bytes)) return 1;
            VkMappedMemoryRange ranges[2]={{0},{0}};
            for(unsigned i=0;i<2;++i) {ranges[i].sType=VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;ranges[i].memory=memory[i];ranges[i].size=VK_WHOLE_SIZE;}
            CHECK(vkFlushMappedMemoryRanges(dev,2,ranges));
            CHECK(vkResetCommandBuffer(cmd,0));VkCommandBufferBeginInfo begin={0};begin.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            CHECK(vkBeginCommandBuffer(cmd,&begin));
            VkImageMemoryBarrier b[2]={0};
            for(unsigned i=0;i<2;++i) {b[i].sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                b[i].srcAccessMask=VK_ACCESS_HOST_WRITE_BIT;b[i].dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
                b[i].oldLayout=VK_IMAGE_LAYOUT_UNDEFINED;b[i].newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                b[i].srcQueueFamilyIndex=b[i].dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;
                b[i].image=images[i];b[i].subresourceRange=(VkImageSubresourceRange){VK_IMAGE_ASPECT_COLOR_BIT,0,count,0,1};}
            vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,NULL,0,NULL,2,b);
            vkCmdCopyBufferToImage(cmd,buffers[0],images[0],VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,count,regions);
            doom3_ps4_copy_texture(cmd,buffers[1],16,images[1]);
            for(unsigned i=0;i<2;++i) {b[i].srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;b[i].dstAccessMask=VK_ACCESS_HOST_READ_BIT;
                b[i].oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;b[i].newLayout=VK_IMAGE_LAYOUT_GENERAL;}
            vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,0,NULL,0,NULL,2,b);
            CHECK(vkEndCommandBuffer(cmd));CHECK(vkResetFences(dev,1,&fence));
            VkSubmitInfo submit={0};submit.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;submit.commandBufferCount=1;submit.pCommandBuffers=&cmd;
            CHECK(vkQueueSubmit(queue,1,&submit,fence));CHECK(vkWaitForFences(dev,1,&fence,VK_TRUE,UINT64_MAX));
            CHECK(vkInvalidateMappedMemoryRanges(dev,2,ranges));
            if(memcmp(mapped[0]+req.alignment,mapped[1]+req.alignment,req.size)) {
                for(size_t k=0;k<req.size;++k) if(mapped[0][req.alignment+k]!=mapped[1][req.alignment+k]) {
                    vk_ps4_log("FAIL static texture %ux%u pass=%u offset=%llu generic=%u packed=%u",w,h,pass,(unsigned long long)k,mapped[0][req.alignment+k],mapped[1][req.alignment+k]);break;}
                return 1;
            }
            unsigned nonzero=0;
            for(size_t k=0;k<req.size;++k) nonzero|=mapped[0][req.alignment+k];
            if(!nonzero) return 1;
            for(unsigned i=0;i<2;++i) {
                for(size_t k=0;k<req.alignment;++k)
                    if(mapped[i][k]!=0xcd || mapped[i][req.alignment+req.size+k]!=0xcd) return 1;
                for(unsigned k=0;k<16;++k) if(bp[i][k]!=0xa7 || bp[i][bytes+16+k]!=0xa7) return 1;
            }
            checked+=req.size;
        }
        CHECK(vkResetCommandBuffer(cmd,0));
        vkDestroyFence(dev,fence,NULL);
        for(unsigned i=0;i<2;++i) {vkUnmapMemory(dev,bm[i]);vkDestroyBuffer(dev,buffers[i],NULL);vkFreeMemory(dev,bm[i],NULL);
            vkUnmapMemory(dev,memory[i]);vkDestroyImage(dev,images[i],NULL);vkFreeMemory(dev,memory[i],NULL);}
        vk_ps4_log("PASS static texture heap=%u %ux%u mips=%u updates=2",heap,w,h,count);
    }
    doom3_ps4_clear_texture_cache();
    doom3_ps4_clear_texture_cache();
    vk_ps4_log("PASS static texture upload cases=22 updates=44 compared_bytes=%llu",checked);
    return 0;
}
