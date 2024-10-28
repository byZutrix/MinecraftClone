#include "../vulkan_base.h"

bool Vulkan::createSwapchain(VkImageUsageFlags usage, VulkanSwapchain* oldSwapchain) {
	VulkanSwapchain result = {};

	// GET SWAPCHAIN SURFACE SUPPORT
	VkBool32 supportsPresent = 0;
	VKA(vkGetPhysicalDeviceSurfaceSupportKHR(context->physicalDevice, context->graphicsQueue.familyIndex, surface, &supportsPresent));
	if (!supportsPresent) {
		LOG_ERROR("Graphics queue does not support present");
		return false;
	}

	// LOAD FORMATS
	uint32_t numFormats = 0;
	VKA(vkGetPhysicalDeviceSurfaceFormatsKHR(context->physicalDevice, surface, &numFormats, 0));
	VkSurfaceFormatKHR* availableFormats = new VkSurfaceFormatKHR[numFormats];
	VKA(vkGetPhysicalDeviceSurfaceFormatsKHR(context->physicalDevice, surface, &numFormats, availableFormats));
	if (numFormats <= 0) {
		LOG_ERROR("No surface formats available");
		delete[] availableFormats;
		return false;
	}

	// FIRST AVAILABE FORMAT SHOULD BE A SENSIBLE DEFAULT IN MOST CASES
	VkFormat format = availableFormats[0].format;
	VkColorSpaceKHR colorSpace = availableFormats[0].colorSpace;

	// GET IMAGE CAPABILITIES
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	VKA(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->physicalDevice, surface, &surfaceCapabilities));
	if (surfaceCapabilities.currentExtent.width == 0xFFFFFFFF) {
		surfaceCapabilities.currentExtent = surfaceCapabilities.minImageExtent;
	}
	if (surfaceCapabilities.currentExtent.height == 0xFFFFFFFF) {
		surfaceCapabilities.currentExtent = surfaceCapabilities.minImageExtent;
	}
	if (surfaceCapabilities.maxImageCount == 0) {
		surfaceCapabilities.maxImageCount = 8;
	}

	// CREATE SWAPCHAIN
	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.surface = surface;
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.minImageCount = 3; // MINIMUM
	createInfo.imageFormat = format;
	createInfo.imageColorSpace = colorSpace;
	createInfo.imageExtent = surfaceCapabilities.currentExtent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = usage;
	createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	/*
	* VK_PRESENT_MODE_IMMEDIATE_KHR -> LOAD AS MANY FRAMES AS POSSIBLE
	* VK_PRESENT_MODE_MAILBOX_KHR -> SET FRAME RATES; LOAD NEXT FRAMES ALREADY IF AT MAX FPS (VSYNC)
	* VK_PRESENT_MODE_FIFO_KHR -> VSYNC ON WITH WAIT(100% funktioniert)
	* VK_PRESENT_MODE_FIFO_RELAXED_KHR -> VYSNC ON WITHOUT WAIT (MAYBE SCREEN TEARING)
	*/
	createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
	createInfo.oldSwapchain = oldSwapchain ? oldSwapchain->swapchain : 0;

	// CREATE SWAPCHAIN
	VKA(vkCreateSwapchainKHR(context->device, &createInfo, 0, &result.swapchain));

	// SET VALUES
	result.format = format;
	result.width = surfaceCapabilities.currentExtent.width;
	result.height = surfaceCapabilities.currentExtent.height;

	// GET SWAPCHAIN IMAGES
	uint32_t numImages;
	VKA(vkGetSwapchainImagesKHR(context->device, result.swapchain, &numImages, 0));

	// AQUIRE SWAPCHAIN IMAGES
	result.images.resize(numImages);
	VKA(vkGetSwapchainImagesKHR(context->device, result.swapchain, &numImages, result.images.data()));

	// CREATE IMAGE VIEWS
	result.imageViews.resize(numImages);
	for (uint32_t i = 0; i < numImages; i++) {
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = result.images[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // 2D bild
		createInfo.format = format;
		createInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY };
		createInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

		VKA(vkCreateImageView(context->device, &createInfo, 0, &result.imageViews[i]));
	}

	delete[] availableFormats;
	swapchain = result;
	return true;
}

// SAFE OLD SWAPCHAIN AN CREATE NEW ONE
void Vulkan::recreateSwapchain() {
	VulkanSwapchain oldSwapchain = swapchain;

	// CHECK FOR MINIMIZE (width, height = 0)
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	VKA(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->physicalDevice, surface, &surfaceCapabilities));
	if (surfaceCapabilities.currentExtent.width == 0 || surfaceCapabilities.currentExtent.height == 0) {
		return; // RETURN IF MINIMIZED
	}

	// WAIT UNTIL EVERYTHING IS DONE
	VKA(vkDeviceWaitIdle(context->device));

	// CREATE NEW SWAPCHAIN
	createSwapchain(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, &oldSwapchain);

	// DESTROY OLD SWAPCHAIN
	cleanupSwapchain(&oldSwapchain);

	// CREATE NEW FRAMEBUFFERS AND RENDERPASS
	cleanupRenderPass();
	renderPass = VK_NULL_HANDLE;
	recreateRenderPass();
}

void Vulkan::cleanupSwapchain(VulkanSwapchain* swapchain) {
	for (uint32_t i = 0; i < swapchain->imageViews.size(); i++) {
		VK(vkDestroyImageView(context->device, swapchain->imageViews[i], 0));
	}
	VK(vkDestroySwapchainKHR(context->device, swapchain->swapchain, 0));
}

