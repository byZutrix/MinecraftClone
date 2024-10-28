#include "vulkan_base.h"

// CONSTRUCTOR
Vulkan::Vulkan(SDL_Window* window) {
	this->window = window;
	context = new VulkanContext;
	context->device = nullptr;
	mipmapLevels = 4.0f;
}

// INIT VULKAN
void Vulkan::initVulkan() {
	
	// Init Vulkan Context and Surface
	this->loadDeviceExtensions();
	if (!this->initVulkanContext()) {
		LOG_ERROR("Error creating vulkan context!");
		return;
	}
	
	// Create Swapchain
	if (!this->createSwapchain(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 0)) {
		LOG_ERROR("Error creating swapchain!");
		return;
	}

	// Create RenderPass
	if (!this->recreateRenderPass()) {
		LOG_ERROR("Error creating renderPass!");
		return;
	}

	createSampler();

	// CREATE TEXTURE ARRAY
	paths.push_back("../data/textures/stone.png"); // 1
	paths.push_back("../data/textures/dirt.png"); // 2
	paths.push_back("../data/textures/grass_block_side.png"); // 3
	paths.push_back("../data/textures/grass_block_top.png"); // 4
	paths.push_back("../data/textures/oak_log.png"); // 5
	paths.push_back("../data/textures/oak_log_top.png"); // 6
	createTextureArray();

	// CAMERA
	cameraManager.initCamera();

	//worldManager.generateChunksAround(cameraManager.camera.cameraPosition, viewDistance);
	//worldManager.processChunkQueue(1);

	createDescriptorPool();

	createUniformBuffers();

	createDescriptorSets();

	createVextexInputAttributes();

	createPipeline("../shaders/texture_vert.spv", "../shaders/texture_frag.spv", &vertexInputBinding);

	createFencesAndSemaphores();

	createAndAllocateCommands();

	// Use any command queue
	{
		VkCommandPool commandPool = commandPools[0];
		VkCommandBuffer commandBuffer = commandBuffers[0];

		VKA(vkResetCommandPool(context->device, commandPool, 0));
		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VKA(vkBeginCommandBuffer(commandBuffer, &begin_info));
	}
}

// LOAD DEVICE EXTENSIONS
void Vulkan::loadDeviceExtensions() {
	const char* additionalInstanceExtensions[] = {
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME
	};

	SDL_Vulkan_GetInstanceExtensions(window, &instanceExtensionCount, 0);

	enabledInstanceExtensions = new const char* [instanceExtensionCount + ARRAY_COUNT(additionalInstanceExtensions)];
	SDL_Vulkan_GetInstanceExtensions(window, &instanceExtensionCount, enabledInstanceExtensions);

	for (uint32_t i = 0; i < ARRAY_COUNT(additionalInstanceExtensions); i++) {
		enabledInstanceExtensions[instanceExtensionCount++] = additionalInstanceExtensions[i];
	}
}

// DEBUG REPORT CALLBACK
VkBool32 VKAPI_CALL Vulkan::debugReportCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData)
{
	if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		LOG_ERROR(callbackData->pMessage);
	}
	else {
		LOG_WARNING(callbackData->pMessage);
	}

	return VK_FALSE;
}

// REGISTER DEBUG CALLBACK
VkDebugUtilsMessengerEXT Vulkan::registerDebugCallback() {
	PFN_vkCreateDebugUtilsMessengerEXT pfnCreateDebugUtilsMessengerEXT;
	pfnCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context->instance, "vkCreateDebugUtilsMessengerEXT");

	VkDebugUtilsMessengerCreateInfoEXT callbackInfo = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	callbackInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
	callbackInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
	callbackInfo.pfnUserCallback = debugReportCallback;

	VkDebugUtilsMessengerEXT callback = 0;
	VKA(pfnCreateDebugUtilsMessengerEXT(context->instance, &callbackInfo, 0, &callback));

	return callback;
}

// INIT VULKAN INSTANCE
bool Vulkan::initVulkanInstance() {

	//LOAD LAYERS
	uint32_t layerPropertyCount = 0;
	VKA(vkEnumerateInstanceLayerProperties(&layerPropertyCount, 0));
	VkLayerProperties* layerProperties = new VkLayerProperties[layerPropertyCount];
	VKA(vkEnumerateInstanceLayerProperties(&layerPropertyCount, layerProperties));
	delete[] layerProperties;

	const char* enabledLayers[] = { "VK_LAYER_KHRONOS_validation" };
	// VALIDATION
	VkValidationFeatureEnableEXT enableValidationFeatures[] = {
		//VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT, //OPTIMIERUNG
		//VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
		VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT,
	};
	VkValidationFeaturesEXT validationFeatures = { VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT };
	validationFeatures.enabledValidationFeatureCount = ARRAY_COUNT(enableValidationFeatures);
	validationFeatures.pEnabledValidationFeatures = enableValidationFeatures;

	// Load AppilcationInfo
	VkApplicationInfo applicationInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	applicationInfo.pApplicationName = "Test Game";
	applicationInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1); // 0.0.1
	applicationInfo.apiVersion = VK_API_VERSION_1_0;

	// Load Create Info
	VkInstanceCreateInfo createInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	createInfo.pNext = &validationFeatures;
	createInfo.pApplicationInfo = &applicationInfo;
	createInfo.enabledLayerCount = ARRAY_COUNT(enabledLayers);
	createInfo.ppEnabledLayerNames = enabledLayers;
	createInfo.enabledExtensionCount = instanceExtensionCount;
	createInfo.ppEnabledExtensionNames = enabledInstanceExtensions;

	// Create Vulkan Instance
	if (VK(vkCreateInstance(&createInfo, 0, &context->instance)) != VK_SUCCESS) {
		LOG_ERROR("Error creating vulkan instance");
		return false;
	}

	context->debugCallback = registerDebugCallback();

	return true;
}

// SELECT PHYSICAL DEVICE
bool Vulkan::selectPhysicalDevice() {

	uint32_t numDevices = 0;
	VKA(vkEnumeratePhysicalDevices(context->instance, &numDevices, 0));
	// Check if there are GPUs with Vulkan support
	if (numDevices == 0) {
		LOG_ERROR("Failed to find GPUs with Vulkan support");
		context->physicalDevice = 0;
		return false;
	}
	// get physical devices
	VkPhysicalDevice* physicalDevices = new VkPhysicalDevice[numDevices];
	VKA(vkEnumeratePhysicalDevices(context->instance, &numDevices, physicalDevices));

	LOG_INFO("Found ", numDevices, " GPU(s):");
	for (uint32_t i = 0; i < numDevices; i++) {
		VkPhysicalDeviceProperties properties = {};
		VK(vkGetPhysicalDeviceProperties(physicalDevices[i], &properties));

		// GPU INFOS:
		//LOG_INFO("GPU (", i, "): ", properties.deviceName);
	}
	// set the physical device
	context->physicalDevice = physicalDevices[0];
	// set physical device properties
	VK(vkGetPhysicalDeviceProperties(context->physicalDevice, &context->physicalDeviceProperties));
	LOG_INFO("Selected GPU: ", context->physicalDeviceProperties.deviceName);

	delete[] physicalDevices;

	return true;

}

// CREATE LOGICAL DEVICE
bool Vulkan::createLogicalDevice(uint32_t deviceExtensionCount, const char** deviceExtensions) {

	// LOAD QUEUES
	uint32_t numQueueFamilies = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(context->physicalDevice, &numQueueFamilies, 0);
	VkQueueFamilyProperties* queueFamilies = new VkQueueFamilyProperties[numQueueFamilies];
	vkGetPhysicalDeviceQueueFamilyProperties(context->physicalDevice, &numQueueFamilies, queueFamilies);
	delete[] queueFamilies;

	// FIND GRAPHICS QUEUE
	uint32_t graphicsQueueIndex = 0;
	for (uint32_t i = 0; i < numQueueFamilies; i++) {
		VkQueueFamilyProperties queueFamily = queueFamilies[i];
		if ((queueFamily.queueCount > 0) && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
			graphicsQueueIndex = i;
			break;
		}
	}

	// CREATE GRAPHICS QUEUE
	float priorities[] = { 1.0f };
	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = priorities;
	queueCreateInfo.queueFamilyIndex = graphicsQueueIndex;


	VkPhysicalDeviceFeatures enabledFeatures = {};
	enabledFeatures.samplerAnisotropy = VK_TRUE;

	// CREATE DEVICE INFO
	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = 1;
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.enabledExtensionCount = deviceExtensionCount;
	createInfo.ppEnabledExtensionNames = deviceExtensions;
	createInfo.pEnabledFeatures = &enabledFeatures;

	if (vkCreateDevice(context->physicalDevice, &createInfo, 0, &context->device)) {
		LOG_ERROR("Failed to create vulkan logical device");
		return false;
	}

	// GET QUEUES
	context->graphicsQueue.familyIndex = graphicsQueueIndex;
	VK(vkGetDeviceQueue(context->device, graphicsQueueIndex, 0, &context->graphicsQueue.queue));

	VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
	VK(vkGetPhysicalDeviceMemoryProperties(context->physicalDevice, &deviceMemoryProperties));
	LOG_INFO("Num device memory heaps: ", deviceMemoryProperties.memoryHeapCount);
	for (uint32_t i = 0; i < deviceMemoryProperties.memoryHeapCount; i++) {
		const char* isDeviceLocal = "false";
		if (deviceMemoryProperties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
			isDeviceLocal = "true";
		}
		LOG_INFO("Heap ", i, ": (Size: ", deviceMemoryProperties.memoryHeaps[i].size, "bytes | Device local: ", isDeviceLocal, ")");
	}

	return true;
}

// INIT VULKAN CONTEXT
bool Vulkan::initVulkanContext() {

	const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	// INIT VULKAN
	LOG_INFO("Initializing vulkan...");
	if (!initVulkanInstance()) {
		return false;
	}

	// SELECT THE PHYSICAL DEVICE (gpu)
	LOG_INFO("Selecting physical device (GPU)...");
	if (!selectPhysicalDevice()) {
		LOG_ERROR("Error creating physical device");
		return false;
	}

	// SELECT THE LOGICAL DEVICE (gpu "software" to render)
	LOG_INFO("Creating logical device...");
	if (!createLogicalDevice(ARRAY_COUNT(deviceExtensions), deviceExtensions)) {
		LOG_ERROR("Error creating logical device");
		return false;
	}

	// CREATE VULKAN SURFACE
	if (SDL_Vulkan_CreateSurface(window, context->instance, &surface) != SDL_TRUE) {
		LOG_ERROR("Failed to create Vulkan surface!");
		return false;
	}

	return true;
}

// CLEANUP THE CONTEXT
void Vulkan::cleanupContext() {
	VK(vkDestroySurfaceKHR(context->instance, surface, 0));
	VK(vkDestroyDevice(context->device, 0));

	if (context->debugCallback) {
		PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroyDebugUtilsMessengerEXT;
		pfnDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(context->instance, "vkDestroyDebugUtilsMessengerEXT");

		pfnDestroyDebugUtilsMessengerEXT(context->instance, context->debugCallback, 0);

		context->debugCallback = 0;
	}
	vkDestroyInstance(context->instance, 0);
}

// CLEANUP
void Vulkan::cleanup() {
	LOG_INFO("Exiting vulkan...");
	VKA(vkDeviceWaitIdle(context->device));

	VK(vkDestroyDescriptorPool(context->device, descriptorPool, 0));

	VK(vkDestroyDescriptorSetLayout(context->device, descriptorSetLayout, 0));

	for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		for (uint32_t j = 0; j < UNIFORM_BUFFER_COUNT; j++) {
			cleanupBuffer(&uniformBuffers[j][i].buffer, &uniformBuffers[j][i].memory);
		}
	}

	for (auto& chunkPair : worldManager.getChunks()) {
		worldManager.cleanupBuffers(context->device, chunkPair.second.get());
	}
	worldManager.clearChunks();
	
	for (uint32_t i = 0; i < textureArray.images.size(); i++) {
		cleanupImage(textureArray.images[i]);
	}
	textureArray.images.clear();
	paths.clear();

	for (uint32_t i = 0; i < ARRAY_COUNT(fences); i++) {
		VK(vkDestroyFence(context->device, fences[i], 0));
	}

	for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {

		VK(vkDestroySemaphore(context->device, acquireSemaphores[i], 0));
		VK(vkDestroySemaphore(context->device, releaseSemaphores[i], 0));
	}
	for (uint32_t i = 0; i < ARRAY_COUNT(commandPools); i++) {
		VK(vkDestroyCommandPool(context->device, commandPools[i], 0));
	}

	cleanupPipeline();

	vkDestroySampler(context->device, sampler, 0);

	// Frame Buffers
	for (uint32_t i = 0; i < framebuffers.size(); i++) {
		VK(vkDestroyFramebuffer(context->device, framebuffers[i], 0));
	}
	framebuffers.clear();

	// COLOR BUFFERS
	//for (uint32_t i = 0; i < colorBuffers.size(); i++) {
	//	cleanupImage(&colorBuffers[i]);
	//}
	//colorBuffers.clear();

	// DEPTH BUFFERS
	for (uint32_t i = 0; i < depthBuffers.size(); i++) {
		cleanupImage(&depthBuffers[i]);
	}
	depthBuffers.clear();

	cleanupRenderPass();
	cleanupSwapchain(&swapchain);
	cleanupContext();
}