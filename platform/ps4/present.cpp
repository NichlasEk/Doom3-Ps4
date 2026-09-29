/* Single device, fixed-format PS4 scene -> VideoOut fullscreen sampled draw.
 * Caller waits for idle before destroying/recreating scene/swapchain objects. */
#include "present.h"
#include "present_spirv.h"
#include <string.h>
static struct {
 VkRenderPass pass; VkPipelineLayout layout; VkPipeline pipeline;
 VkDescriptorSetLayout setLayout; VkDescriptorPool pool; VkDescriptorSet sets[2];
 VkSampler sampler; VkImageView sources[2];
 VkImage images[2]; VkImageView views[2]; VkFramebuffer buffers[2];
} state={};
void PS4_DestroyPresent(VkDevice device) {
 for(unsigned i=0;i<2;++i) {
  if(state.buffers[i]) vkDestroyFramebuffer(device,state.buffers[i],NULL);
  if(state.views[i]) vkDestroyImageView(device,state.views[i],NULL);
 }
 if(state.pipeline) vkDestroyPipeline(device,state.pipeline,NULL);
 if(state.layout) vkDestroyPipelineLayout(device,state.layout,NULL);
 if(state.pool) vkDestroyDescriptorPool(device,state.pool,NULL);
 if(state.setLayout) vkDestroyDescriptorSetLayout(device,state.setLayout,NULL);
 if(state.sampler) vkDestroySampler(device,state.sampler,NULL);
 if(state.pass) vkDestroyRenderPass(device,state.pass,NULL);
 memset(&state,0,sizeof(state));
}
#define CHECK(call) do {VkResult status=(call);if(status!=VK_SUCCESS)return status;}while(0)
static VkResult initialize(VkDevice device) {
 VkAttachmentDescription att={};att.format=VK_FORMAT_R8G8B8A8_UNORM;att.samples=VK_SAMPLE_COUNT_1_BIT;
 att.loadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;att.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
 att.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;att.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
 att.initialLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;att.finalLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
 VkAttachmentReference ref={0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
 VkSubpassDescription sub={};sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;sub.colorAttachmentCount=1;sub.pColorAttachments=&ref;
 VkRenderPassCreateInfo rp={VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};rp.attachmentCount=1;rp.pAttachments=&att;rp.subpassCount=1;rp.pSubpasses=&sub;
 CHECK(vkCreateRenderPass(device,&rp,NULL,&state.pass));
 VkDescriptorSetLayoutBinding binding={0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,NULL};
 VkDescriptorSetLayoutCreateInfo sl={VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};sl.bindingCount=1;sl.pBindings=&binding;
 CHECK(vkCreateDescriptorSetLayout(device,&sl,NULL,&state.setLayout));
 VkPipelineLayoutCreateInfo pl={VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};pl.setLayoutCount=1;pl.pSetLayouts=&state.setLayout;
 CHECK(vkCreatePipelineLayout(device,&pl,NULL,&state.layout));
 VkDescriptorPoolSize ps={VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,2};
 VkDescriptorPoolCreateInfo pool={VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=2;pool.poolSizeCount=1;pool.pPoolSizes=&ps;
 CHECK(vkCreateDescriptorPool(device,&pool,NULL,&state.pool));
 VkDescriptorSetAllocateInfo sa={VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};sa.descriptorPool=state.pool;VkDescriptorSetLayout layouts[2]={state.setLayout,state.setLayout};sa.descriptorSetCount=2;sa.pSetLayouts=layouts;
 CHECK(vkAllocateDescriptorSets(device,&sa,state.sets));
 VkSamplerCreateInfo sm={VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};sm.magFilter=sm.minFilter=VK_FILTER_NEAREST;sm.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST;sm.addressModeU=sm.addressModeV=sm.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
 CHECK(vkCreateSampler(device,&sm,NULL,&state.sampler));
 VkShaderModule modules[2]={};
 VkShaderModuleCreateInfo mi={VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};mi.codeSize=sizeof(present_vert);mi.pCode=present_vert;
 CHECK(vkCreateShaderModule(device,&mi,NULL,&modules[0]));mi.codeSize=sizeof(present_frag);mi.pCode=present_frag;
 VkResult status=vkCreateShaderModule(device,&mi,NULL,&modules[1]);
 if(status!=VK_SUCCESS){vkDestroyShaderModule(device,modules[0],NULL);return status;}
 VkPipelineShaderStageCreateInfo stages[2]={};
 for(unsigned i=0;i<2;++i){stages[i].sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;stages[i].stage=i?VK_SHADER_STAGE_FRAGMENT_BIT:VK_SHADER_STAGE_VERTEX_BIT;stages[i].module=modules[i];stages[i].pName="main";}
 VkPipelineVertexInputStateCreateInfo vi={VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
 VkPipelineInputAssemblyStateCreateInfo ia={VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};ia.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
 VkPipelineViewportStateCreateInfo vp={VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};vp.viewportCount=vp.scissorCount=1;
 VkPipelineRasterizationStateCreateInfo rs={VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};rs.polygonMode=VK_POLYGON_MODE_FILL;rs.cullMode=VK_CULL_MODE_NONE;rs.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE;rs.lineWidth=1;
 VkPipelineMultisampleStateCreateInfo ms={VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
 VkPipelineColorBlendAttachmentState blend={};blend.colorWriteMask=15;
 VkPipelineColorBlendStateCreateInfo cb={VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};cb.attachmentCount=1;cb.pAttachments=&blend;
 VkDynamicState dynamic[]={VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
 VkPipelineDynamicStateCreateInfo dy={VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};dy.dynamicStateCount=2;dy.pDynamicStates=dynamic;
 VkGraphicsPipelineCreateInfo pi={VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};pi.stageCount=2;pi.pStages=stages;pi.pVertexInputState=&vi;pi.pInputAssemblyState=&ia;pi.pViewportState=&vp;pi.pRasterizationState=&rs;pi.pMultisampleState=&ms;pi.pColorBlendState=&cb;pi.pDynamicState=&dy;pi.layout=state.layout;pi.renderPass=state.pass;
 status=vkCreateGraphicsPipelines(device,VK_NULL_HANDLE,1,&pi,NULL,&state.pipeline);
 for(unsigned i=0;i<2;++i)vkDestroyShaderModule(device,modules[i],NULL);
 return status;
}
VkResult PS4_PresentDraw(VkDevice device,VkCommandBuffer cmd,VkImage scene,VkImageView source,VkImage target,VkExtent2D size) {
 if(!state.pipeline){VkResult r=initialize(device);if(r!=VK_SUCCESS){PS4_DestroyPresent(device);return r;}}
 unsigned slot=0;for(;slot<2;++slot)if(!state.images[slot]||state.images[slot]==target)break;
 if(slot==2)return VK_ERROR_TOO_MANY_OBJECTS;
 if(!state.images[slot]) {
  VkImageViewCreateInfo v={VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};v.image=target;v.viewType=VK_IMAGE_VIEW_TYPE_2D;v.format=VK_FORMAT_R8G8B8A8_UNORM;v.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
  CHECK(vkCreateImageView(device,&v,NULL,&state.views[slot]));
  VkFramebufferCreateInfo fb={VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};fb.renderPass=state.pass;fb.attachmentCount=1;fb.pAttachments=&state.views[slot];fb.width=size.width;fb.height=size.height;fb.layers=1;
  CHECK(vkCreateFramebuffer(device,&fb,NULL,&state.buffers[slot]));state.images[slot]=target;
 }
 if(state.sources[slot]!=source) {
  VkDescriptorImageInfo image={state.sampler,source,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
  VkWriteDescriptorSet write={VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=state.sets[slot];write.descriptorCount=1;write.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;write.pImageInfo=&image;
  vkUpdateDescriptorSets(device,1,&write,0,NULL);state.sources[slot]=source;
 }
 VkImageMemoryBarrier barriers[2]={};for(auto &b:barriers){b.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;b.srcQueueFamilyIndex=b.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;b.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};}
 barriers[0].image=scene;barriers[0].oldLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;barriers[0].newLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;barriers[0].srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_TRANSFER_READ_BIT;barriers[0].dstAccessMask=VK_ACCESS_SHADER_READ_BIT;
 barriers[1].image=target;barriers[1].oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barriers[1].newLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;barriers[1].dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
 vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,0,0,NULL,0,NULL,2,barriers);
 VkRenderPassBeginInfo begin={VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};begin.renderPass=state.pass;begin.framebuffer=state.buffers[slot];begin.renderArea.extent=size;
 vkCmdBeginRenderPass(cmd,&begin,VK_SUBPASS_CONTENTS_INLINE);
 vkCmdBindPipeline(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,state.pipeline);vkCmdBindDescriptorSets(cmd,VK_PIPELINE_BIND_POINT_GRAPHICS,state.layout,0,1,&state.sets[slot],0,NULL);
 VkViewport vp={0,0,float(size.width),float(size.height),0,1};VkRect2D sc={{0,0},size};vkCmdSetViewport(cmd,0,1,&vp);vkCmdSetScissor(cmd,0,1,&sc);vkCmdDraw(cmd,3,1,0,0);vkCmdEndRenderPass(cmd);
 barriers[0].oldLayout=VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;barriers[0].newLayout=VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;barriers[0].srcAccessMask=VK_ACCESS_SHADER_READ_BIT;barriers[0].dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;
 barriers[1].oldLayout=barriers[1].newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;barriers[1].srcAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;barriers[1].dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;
 vkCmdPipelineBarrier(cmd,VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,NULL,0,NULL,2,barriers);
 return VK_SUCCESS;
}
