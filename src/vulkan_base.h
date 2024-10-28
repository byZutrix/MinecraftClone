#ifndef VULKAN_H
#define VULKAN_H

#include <vulkan/vulkan.h>
#include <vector>

#include "game_engine/window.h"
#include "game_engine/cameramanager.h"
#include "game_engine/worldmanager.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm/ext/matrix_transform.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

#define ASSERT_VULKAN(val) if(val != VK_SUCCESS) {assert(false);}

#ifndef VK
#define VK(f) (f)
#endif

#ifndef VKA
#define VKA(f) ASSERT_VULKAN(VK(f))
#endif

#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))
#define ALIGN_UP_POW2(x, p) (((x)+(p) - 1) &~((p) - 1))

#define FRAMES_IN_FLIGHT 2
#define UNIFORM_BUFFER_COUNT 2

class Vulkan {
public:
	Vulkan(SDL_Window* window);

	// QUEUE
	struct VulkanQueue {
		VkQueue queue;
		uint32_t familyIndex;
	};

	// SWAPCHAIN
	struct VulkanSwapchain {
		VkSwapchainKHR swapchain;
		uint32_t width;
		uint32_t height;
		VkFormat format;
		std::vector<VkImage> images;
		std::vector<VkImageView> imageViews;
	};

	// PIPELINE
	struct VulkanPipeline {
		VkPipeline pipeline;
		VkPipelineLayout pipelineLayout;
	};

	// CONTEXT
	struct VulkanContext {
		VkInstance instance;
		VkPhysicalDevice physicalDevice;
		VkPhysicalDeviceProperties physicalDeviceProperties;
		VkDevice device;
		VulkanQueue graphicsQueue;
		VkDebugUtilsMessengerEXT debugCallback;
	};

	// BUFFER
	struct VulkanBuffer {
		VkBuffer buffer;
		VkDeviceMemory memory;
	};

	// IMAGE
	struct VulkanImage {
		VkImage image;
		VkImageView view;
		VkDeviceMemory memory;
	};

	// Uniform Buffer
	struct UniformBufferObject {
		glm::mat4 modelViewProj;
		glm::mat4 modelView;
	};

	// TextureArray
	struct TextureArray {
		std::vector<VulkanImage*> images;
		void* data;
		size_t size;
		uint32_t width;
		uint32_t height;
		uint32_t layerCount;
		uint32_t arrayLayer;
		VkImageLayout finalLayout;
		VkAccessFlags dstAccessMask;
	};

	SDL_Window* window;
	VulkanContext* context;
	VkSurfaceKHR surface;
	VulkanSwapchain swapchain;
	VulkanSwapchain oldSwapchain;
	VkRenderPass renderPass;
	std::vector<VulkanImage> depthBuffers;
	std::vector<VkFramebuffer> framebuffers;
	VkCommandPool commandPools[FRAMES_IN_FLIGHT];
	VkCommandBuffer commandBuffers[FRAMES_IN_FLIGHT];
	VkFence fences[FRAMES_IN_FLIGHT];
	VkSemaphore acquireSemaphores[FRAMES_IN_FLIGHT];
	VkSemaphore releaseSemaphores[FRAMES_IN_FLIGHT];

	VkDescriptorSet descriptorSets[UNIFORM_BUFFER_COUNT][FRAMES_IN_FLIGHT];
	VkDescriptorSetLayout descriptorSetLayout;

	TextureArray textureArray;
	std::vector<const char*> paths;

	VulkanPipeline pipeline;
	VkSampler sampler;
	VkDescriptorPool descriptorPool;
	VulkanBuffer uniformBuffers[UNIFORM_BUFFER_COUNT][FRAMES_IN_FLIGHT];
	CameraManager cameraManager;

	uint32_t maxUniformSize;
	uint64_t singleElementSize;

	VkVertexInputAttributeDescription vertexAttributeDescriptions[4];
	VkVertexInputBindingDescription vertexInputBinding;

	WorldManager worldManager;
	const uint32_t viewDistance = 10;

	float mipmapLevels;

	void initVulkan();

	// RENDER
	void renderVulkan();
	void updateVulkan(float delta);

	void cleanup();
private:

	// DEVICE
	void loadDeviceExtensions();
	static VkBool32 VKAPI_CALL debugReportCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT messageTypes,
		const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData);
	VkDebugUtilsMessengerEXT registerDebugCallback();
	bool initVulkanInstance();

	bool initVulkanContext();

	bool selectPhysicalDevice();

	bool createLogicalDevice(uint32_t deviceExtensionCount, const char** deviceExtensions);
	void cleanupContext();

	uint32_t instanceExtensionCount;
	const char** enabledInstanceExtensions;

	// SWAPCHAIN
	bool createSwapchain(VkImageUsageFlags usage, VulkanSwapchain* oldSwapchain);
	void recreateSwapchain();
	void cleanupSwapchain(VulkanSwapchain* swapchain);

	// RENDERPASS
	bool createRenderPass(VkFormat format);
	bool recreateRenderPass();
	void cleanupRenderPass();

	// PIPELINE
	VkShaderModule createShaderModule(const char* shaderFilename);
	bool createPipeline(const char* vertexShaderFilename, const char* fragmentShaderFilename, VkVertexInputBindingDescription* binding);
	void cleanupPipeline();

	// UTILS
	// BUFFER
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags memoryProperties);
	bool createBuffer(VkBuffer* buffer, VkDeviceMemory* memory, uint64_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties);
	void uploadDataToBuffer(VulkanBuffer* buffer, void* data, size_t size);
	void cleanupBuffer(VkBuffer* buffer, VkDeviceMemory* memory);

	// IMAGE
	void createImage(VulkanImage* image, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, uint32_t arrayLayers = 1);
	void uploadDataToImage(const TextureArray& uploadInfo);
	void cleanupImage(VulkanImage* image);

	// CREATES
	void createSampler();
	//void createTextureImage(TextureArray& uploadInfo, const char* path);
	void createTextureArray();
	void createDescriptorPool();
	void createUniformBuffers();
	void createDescriptorSets();
	void createVextexInputAttributes();
	void createFencesAndSemaphores();
	void createAndAllocateCommands();

	// RENDER
	void renderInCommand(VkCommandBuffer commandBuffer, uint32_t frameIndex);
	// CHUNK
	void uploadChunkMesh(Chunk* chunk);
	void renderChunk(VkCommandBuffer commandBuffer, uint32_t frameIndex);
};

#endif // VULKAN_H