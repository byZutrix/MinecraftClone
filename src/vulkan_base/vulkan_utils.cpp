#include "../vulkan_base.h"

uint32_t Vulkan::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags memoryProperties) {

	// GET DEVICE MEMORY PROPERTIES
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
	VK(vkGetPhysicalDeviceMemoryProperties(context->physicalDevice, &deviceMemoryProperties));

	for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++) {
		// CHECK IF REQUIRED MEMORY TYPE IS ALLOWED
		if ((typeFilter & (1 << i)) != 0) {
			// CHECK IF REQUIRED PROPERTIES ARE STATISFIED
			if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & memoryProperties) == memoryProperties) {
				// RETURN THIS MEMORY TYPE INDEX
				return i;
			}
		}
	}

	// NO MATCHING AVAILABLE MEMORY TYPE FOUND
	assert(false);
	return UINT32_MAX;
}

bool Vulkan::createBuffer(VkBuffer* buffer, VkDeviceMemory* memory, uint64_t size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProperties) {

	// CREATE BUFFER
	VkBufferCreateInfo createInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	createInfo.size = size;
	createInfo.usage = usage;
	VKA(vkCreateBuffer(context->device, &createInfo, 0, buffer));

	// GET MEMORY REQUIRMENTS
	VkMemoryRequirements memoryRequirements;
	VK(vkGetBufferMemoryRequirements(context->device, *buffer, &memoryRequirements));

	// GET MEMORY INDEX
	uint32_t memoryIndex = findMemoryType(memoryRequirements.memoryTypeBits, memoryProperties);
	assert(memoryIndex != UINT32_MAX);

	// ALLOCATE INFO
	VkMemoryAllocateInfo allocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = memoryIndex;

	// ALLOCATE MEMORY
	VKA(vkAllocateMemory(context->device, &allocateInfo, 0, memory));

	// BIND TO BUFFER
	VKA(vkBindBufferMemory(context->device, *buffer, *memory, 0));

	return true;
}

// UPLOAD DATA TO BUFFER
void Vulkan::uploadDataToBuffer(VulkanBuffer* buffer, void* data, size_t size) {
#if 0
	void* mapped;
	VKA(vkMapMemory(context->device, buffer->memory, 0, size, 0, &mapped));
	memcpy(mapped, data, size);
	VK(vkUnmapMemory(context->device, buffer->memory));
#else
	// UPLOAD WITH STAGING BUFFER
	VulkanQueue* queue = &context->graphicsQueue;
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;
	VulkanBuffer stagingBuffer;

	// CREATE AND MAP THE BUFFER
	createBuffer(&stagingBuffer.buffer, &stagingBuffer.memory, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	void* mapped;
	VKA(vkMapMemory(context->device, stagingBuffer.memory, 0, size, 0, &mapped));
	memcpy(mapped, data, size);
	VK(vkUnmapMemory(context->device, stagingBuffer.memory));

	// CREATE COMMAND QUEUE
	{
		VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		createInfo.queueFamilyIndex = queue->familyIndex;
		VKA(vkCreateCommandPool(context->device, &createInfo, 0, &commandPool));
	}

	// CREATE COMMAND BUFFER ALLOCATE
	{
		VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.commandPool = commandPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = 1;
		VKA(vkAllocateCommandBuffers(context->device, &allocateInfo, &commandBuffer));
	}

	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VKA(vkBeginCommandBuffer(commandBuffer, &beginInfo));

	VkBufferCopy region = { 0, 0, size };
	VK(vkCmdCopyBuffer(commandBuffer, stagingBuffer.buffer, buffer->buffer, 1, &region));

	VKA(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	VKA(vkQueueSubmit(queue->queue, 1, &submitInfo, VK_NULL_HANDLE));
	VKA(vkQueueWaitIdle(queue->queue));

	VK(vkDestroyCommandPool(context->device, commandPool, 0));
	cleanupBuffer(&stagingBuffer.buffer, &stagingBuffer.memory);

#endif
}

void Vulkan::cleanupBuffer(VkBuffer* buffer, VkDeviceMemory* memory) {
	VK(vkDestroyBuffer(context->device, *buffer, 0));
	// ASSUMES THAT THE BUFFER OWNS IT OWN MEMORY BLOCK
	VK(vkFreeMemory(context->device, *memory, 0));
}

void Vulkan::createImage(VulkanImage* image, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, uint32_t arrayLayers) {

	// CREATE IMAGE
	{
		VkImageCreateInfo createInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
		createInfo.imageType = VK_IMAGE_TYPE_2D;
		createInfo.extent.width = width;
		createInfo.extent.height = height;
		createInfo.extent.depth = 1;
		createInfo.mipLevels = 1;
		createInfo.arrayLayers = arrayLayers;
		createInfo.format = format;
		createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		createInfo.usage = usage;
		//createInfo.samples = sampleCount;
		createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VKA(vkCreateImage(context->device, &createInfo, 0, &image->image));
	}

	// MAP MEMORY
	VkMemoryRequirements memoryRequirments;
	VK(vkGetImageMemoryRequirements(context->device, image->image, &memoryRequirments));
	VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocateInfo.allocationSize = memoryRequirments.size;
	allocateInfo.memoryTypeIndex = findMemoryType(memoryRequirments.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VKA(vkAllocateMemory(context->device, &allocateInfo, 0, &image->memory));
	VKA(vkBindImageMemory(context->device, image->image, image->memory, 0));

	// CHECK FOR DEPTH BUFFER OR NOT
	VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	if (format == VK_FORMAT_D32_SFLOAT) {
		aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	// CREATE IMAGE VIEW
	{
		VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		createInfo.image = image->image;
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		createInfo.format = format;
		createInfo.subresourceRange.aspectMask = aspect;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		VKA(vkCreateImageView(context->device, &createInfo, 0, &image->view));
	}
}

void Vulkan::uploadDataToImage(const TextureArray& uploadInfo) {
	// UPLOAD WITH STAGING BUFFER
	VulkanQueue* queue = &context->graphicsQueue;
	VkCommandPool commandPool;
	VkCommandBuffer commandBuffer;
	VulkanBuffer stagingBuffer;

	// CREATE AND MAP THE BUFFER
	createBuffer(&stagingBuffer.buffer, &stagingBuffer.memory, uploadInfo.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	void* mapped;
	VKA(vkMapMemory(context->device, stagingBuffer.memory, 0, uploadInfo.size, 0, &mapped));
	memcpy(mapped, uploadInfo.data, uploadInfo.size);
	VK(vkUnmapMemory(context->device, stagingBuffer.memory));

	// CREATE COMMAND QUEUE
	{
		VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		createInfo.queueFamilyIndex = queue->familyIndex;
		VKA(vkCreateCommandPool(context->device, &createInfo, 0, &commandPool));
	}

	// CREATE COMMAND BUFFER ALLOCATE
	{
		VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.commandPool = commandPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = 1;
		VKA(vkAllocateCommandBuffers(context->device, &allocateInfo, &commandBuffer));
	}

	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VKA(vkBeginCommandBuffer(commandBuffer, &beginInfo));

	// IMAGE BARRIER 1
	{
		VkImageMemoryBarrier imageBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.image = uploadInfo.images[uploadInfo.arrayLayer]->image;
		imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBarrier.subresourceRange.levelCount = 1;
		imageBarrier.subresourceRange.baseArrayLayer = 0;
		imageBarrier.subresourceRange.layerCount = uploadInfo.layerCount;
		imageBarrier.srcAccessMask = 0;
		imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 0, 0, 1, &imageBarrier);
	}

	VkBufferImageCopy region = {};
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.layerCount = 1;
	region.imageExtent = { uploadInfo.width, uploadInfo.height, 1 };
	VK(vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.buffer, uploadInfo.images[uploadInfo.arrayLayer]->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region));

	// IMAGE BARRIER 2
	{
		VkImageMemoryBarrier imageBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrier.newLayout = uploadInfo.finalLayout;
		imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.image = uploadInfo.images[uploadInfo.arrayLayer]->image;
		imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBarrier.subresourceRange.levelCount = 1;
		imageBarrier.subresourceRange.baseArrayLayer = 0;
		imageBarrier.subresourceRange.layerCount = uploadInfo.layerCount;
		imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageBarrier.dstAccessMask = VK_ACCESS_NONE;
		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, 0, 0, 0, 1, &imageBarrier);
	}


	VKA(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	VKA(vkQueueSubmit(queue->queue, 1, &submitInfo, VK_NULL_HANDLE));
	VKA(vkQueueWaitIdle(queue->queue));

	VK(vkDestroyCommandPool(context->device, commandPool, 0));
	cleanupBuffer(&stagingBuffer.buffer, &stagingBuffer.memory);
}

void Vulkan::cleanupImage(VulkanImage* image) {
	VK(vkDestroyImageView(context->device, image->view, 0));
	VK(vkDestroyImage(context->device, image->image, 0));
	VK(vkFreeMemory(context->device, image->memory, 0));
}

void Vulkan::uploadChunkMesh(Chunk* chunk) {
	VkDeviceSize vertexBufferSize = sizeof(chunk->getVertices()[0]) * chunk->getVertices().size();
	VkDeviceSize indexBufferSize = sizeof(chunk->getIndices()[0]) * chunk->getIndices().size();

	if (vertexBufferSize == 0) {
		vertexBufferSize = 1;
		LOG_INFO("VERTEX BUFFER SIZE IS 0");
	}
	if (indexBufferSize == 0) {
		indexBufferSize = 1;
		LOG_INFO("INDEX BUFFER SIZE IS 0");
	}

	createBuffer(&chunk->vertexBuffer, &chunk->vertexBufferMemory, vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	void* vertexData;
	vkMapMemory(context->device, chunk->vertexBufferMemory, 0, vertexBufferSize, 0, &vertexData);
	memcpy(vertexData, chunk->getVertices().data(), (size_t)vertexBufferSize);
	vkUnmapMemory(context->device, chunk->vertexBufferMemory);


	createBuffer(&chunk->indexBuffer, &chunk->indexBufferMemory, indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	void* indexData;
	vkMapMemory(context->device, chunk->indexBufferMemory, 0, indexBufferSize, 0, &indexData);
	memcpy(indexData, chunk->getIndices().data(), (size_t)indexBufferSize);
	vkUnmapMemory(context->device, chunk->indexBufferMemory);
}