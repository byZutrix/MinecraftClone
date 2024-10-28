#include "../vulkan_base.h"

bool Vulkan::createRenderPass(VkFormat format) {
	VkRenderPass renderPass;

	VkAttachmentDescription attachmentDescriptions[2];
	attachmentDescriptions[0] = {};
	attachmentDescriptions[0].format = format;
	attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	attachmentDescriptions[1] = {};
	attachmentDescriptions[1].format = VK_FORMAT_D32_SFLOAT;
	attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE; // VK_ATTACHMENT_STORE_OP_DONT_CARE
	attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL_KHR; //VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL

	/*attachmentDescriptions[2] = {};
	attachmentDescriptions[2].format = format;
	attachmentDescriptions[2].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[2].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescriptions[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescriptions[2].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;*/

	VkAttachmentReference attachmentReference = {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
	VkAttachmentReference depthStencilReference = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
	//VkAttachmentReference resolveTargetReference = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &attachmentReference;
	subpass.pDepthStencilAttachment = &depthStencilReference;
	//subpass.pResolveAttachments = &resolveTargetReference;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo createInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	createInfo.attachmentCount = ARRAY_COUNT(attachmentDescriptions);
	createInfo.pAttachments = attachmentDescriptions;
	createInfo.subpassCount = 1;
	createInfo.pSubpasses = &subpass;
	createInfo.dependencyCount = 1;
	createInfo.pDependencies = &dependency;
	VKA(vkCreateRenderPass(context->device, &createInfo, 0, &renderPass));

	this->renderPass = renderPass;
	return true;
}

bool Vulkan::recreateRenderPass() {
	if (renderPass == VK_NULL_HANDLE) {
		// CLEAR FRAMEBUFFERS
		for (uint32_t i = 0; i < framebuffers.size(); i++) {
			VK(vkDestroyFramebuffer(context->device, framebuffers[i], 0));
		}
		for (uint32_t i = 0; i < depthBuffers.size(); i++) {
			cleanupImage(&depthBuffers[i]);
		}
		//for (uint32_t i = 0; i < colorBuffers.size(); i++) {
			//cleanupImage(&colorBuffers[i]);
		//}
		// DESTROY RENDERPASS
		cleanupRenderPass();
	}
	// CLEAR FRAMEBUFFERS
	framebuffers.clear();
	depthBuffers.clear();
	//colorBuffers.clear();

	// CREATE RENDERPASS
	if (!createRenderPass(swapchain.format)) // 4 BIT SAMPLER
		return false;
	// CREATE FRAMEBUFFERS
	framebuffers.resize(swapchain.images.size());
	depthBuffers.resize(swapchain.images.size());
	//colorBuffers.resize(swapchain.images.size());
	for (uint32_t i = 0; i < swapchain.images.size(); i++) {
		createImage(&depthBuffers.data()[i], swapchain.width, swapchain.height, VK_FORMAT_D32_SFLOAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT); // 4 BIT SAMPLER
		//createImage(&colorBuffers.data()[i], swapchain.width, swapchain.height, swapchain.format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_SAMPLE_COUNT_4_BIT); // 4 BIT SAMPLER
		/*VkImageView attachments[] = {
			colorBuffers[i].view,
			depthBuffers[i].view,
			swapchain.imageViews[i],
		};*/

		VkImageView attachments[] = {
			swapchain.imageViews[i],
			depthBuffers[i].view,
		};

		VkFramebufferCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		createInfo.renderPass = renderPass;
		createInfo.attachmentCount = ARRAY_COUNT(attachments);
		createInfo.pAttachments = attachments;
		createInfo.width = swapchain.width;
		createInfo.height = swapchain.height;
		createInfo.layers = 1;
		VKA(vkCreateFramebuffer(context->device, &createInfo, 0, &framebuffers[i]));
	}

	return true;
}

void Vulkan::cleanupRenderPass() {
	VK(vkDestroyRenderPass(context->device, renderPass, 0));
}