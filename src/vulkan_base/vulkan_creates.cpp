#include "../vulkan_base.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

void Vulkan::createSampler() {

	VkSamplerCreateInfo createInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
	createInfo.magFilter = VK_FILTER_NEAREST; // VK_FILTER_NEAREST
	createInfo.minFilter = VK_FILTER_NEAREST; // VK_FILTER_NEAREST
	createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT; // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
	createInfo.addressModeV = createInfo.addressModeU;
	createInfo.addressModeW = createInfo.addressModeU;
	createInfo.anisotropyEnable = VK_TRUE;
	createInfo.maxAnisotropy = context->physicalDeviceProperties.limits.maxSamplerAnisotropy; // CHANGE MAYBE LATER IDK PERFORMANCE
	createInfo.compareEnable = VK_FALSE;
	createInfo.mipLodBias = -0.5f; // MAYBE 0.0f
	createInfo.minLod = 0.0f;
	createInfo.maxLod = mipmapLevels;

	VKA(vkCreateSampler(context->device, &createInfo, 0, &sampler));
}

void Vulkan::createTextureArray() {
	int width = 0, height = 0, channels = 0;
	uint8_t* data = nullptr;

	uint32_t layerCount = paths.size();  // Each path represents a texture, so each texture is a layer

	// Load the first image to get the width, height, and channels
	data = stbi_load(paths[0], &width, &height, &channels, 4);  // Load as RGBA
	if (!data) {
		LOG_ERROR("Could not load image data for the first texture!");
		return;
	}
	stbi_image_free(data);  // Free the temporary data, we just wanted the size

	// Create the texture array image (with the correct number of layers)
	textureArray.images.resize(layerCount);
	for (int i = 0; i < layerCount; i++) {
		textureArray.images[i] = new VulkanImage;
		createImage(textureArray.images[i], width, height, VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, layerCount);
	}
	

	// Loop through each image and upload it as a layer in the array
	for (uint32_t layer = 0; layer < layerCount; ++layer) {
		// Load the image data for this layer
		data = stbi_load(paths[layer], &width, &height, &channels, 4);
		if (!data) {
			LOG_ERROR("Could not load image data for layer ", layer);
			continue;
		}

		// Prepare the upload info for this layer
		textureArray.data = data;
		textureArray.size = width * height * 4;  // Each pixel is 4 bytes (RGBA)
		textureArray.width = width;
		textureArray.height = height;
		textureArray.layerCount = layerCount;  // Total layers in the array
		textureArray.arrayLayer = layer;  // Upload to the current layer
		textureArray.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; //VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		textureArray.dstAccessMask = VK_ACCESS_SHADER_READ_BIT; //VK_ACCESS_SHADER_READ_BIT //VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL

		// Upload the data for this layer
		uploadDataToImage(textureArray);

		// Free the loaded image data for this layer
		stbi_image_free(data);
	}
}

void Vulkan::createDescriptorPool() {
	VkDescriptorPoolSize poolSizes[] = {
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, FRAMES_IN_FLIGHT * 1000},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, FRAMES_IN_FLIGHT * 1000}
	};

	VkDescriptorPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	createInfo.maxSets = FRAMES_IN_FLIGHT * 1000;
	createInfo.poolSizeCount = ARRAY_COUNT(poolSizes);
	createInfo.pPoolSizes = poolSizes;
	VKA(vkCreateDescriptorPool(context->device, &createInfo, nullptr, &descriptorPool));
}

void Vulkan::createUniformBuffers() {
	
	uint64_t minUniformAlignment = context->physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
	maxUniformSize = context->physicalDeviceProperties.limits.maxUniformBufferRange;
	singleElementSize = ALIGN_UP_POW2(sizeof(UniformBufferObject), minUniformAlignment);
	for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		for (uint32_t j = 0; j < UNIFORM_BUFFER_COUNT; j++) {
			createBuffer(&uniformBuffers[j][i].buffer, &uniformBuffers[j][i].memory, maxUniformSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		}
		
	}
}

void Vulkan::createDescriptorSets() {

	VkDescriptorSetLayoutBinding bindings[] = {
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, textureArray.layerCount, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}, // &sampler
	};

	VkDescriptorSetLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	createInfo.bindingCount = ARRAY_COUNT(bindings);
	createInfo.pBindings = bindings;
	VKA(vkCreateDescriptorSetLayout(context->device, &createInfo, 0, &descriptorSetLayout));
	// BUFFER INFO
	for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; i++) {
		for (uint32_t j = 0; j < UNIFORM_BUFFER_COUNT; j++) {
			VkDescriptorSetAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
			allocateInfo.descriptorPool = descriptorPool;
			allocateInfo.descriptorSetCount = 1;
			allocateInfo.pSetLayouts = &descriptorSetLayout;
			VKA(vkAllocateDescriptorSets(context->device, &allocateInfo, &descriptorSets[j][i]));

			VkDescriptorBufferInfo bufferInfo = { uniformBuffers[j][i].buffer, 0, singleElementSize };

			std::vector<VkDescriptorImageInfo> imageInfos(textureArray.layerCount);
			for (uint32_t i = 0; i < textureArray.layerCount; i++) {
				imageInfos[i] = { sampler, textureArray.images[i]->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
			}

			VkWriteDescriptorSet descriptorWrites[2];

			descriptorWrites[0] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			descriptorWrites[0].dstSet = descriptorSets[j][i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			descriptorWrites[1] = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
			descriptorWrites[1].dstSet = descriptorSets[j][i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorCount = textureArray.layerCount; //1
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].pImageInfo = imageInfos.data();
			VK(vkUpdateDescriptorSets(context->device, ARRAY_COUNT(descriptorWrites), descriptorWrites, 0, nullptr));
		}
	}
}

void Vulkan::createVextexInputAttributes() {
	
	// ATTRUBUTES
	// POSITION ATTRIBUTE
	vertexAttributeDescriptions[0].binding = 0;
	vertexAttributeDescriptions[0].location = 0;
	vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexAttributeDescriptions[0].offset = offsetof(Vertex, position);

	// NORMAL ATTRIBUTE
	vertexAttributeDescriptions[1].binding = 0;
	vertexAttributeDescriptions[1].location = 1;
	vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexAttributeDescriptions[1].offset = offsetof(Vertex, normal);

	// TEXCOORD ATTRIBUTE
	vertexAttributeDescriptions[2].binding = 0;
	vertexAttributeDescriptions[2].location = 2;
	vertexAttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	vertexAttributeDescriptions[2].offset = offsetof(Vertex, texCoord);

	// TEX INDEX ATTRIBUTE
	vertexAttributeDescriptions[3].binding = 0;
	vertexAttributeDescriptions[3].location = 3;
	vertexAttributeDescriptions[3].format = VK_FORMAT_R32_SINT;
	vertexAttributeDescriptions[3].offset = offsetof(Vertex, texIndex);

	// BINDING DESCRIPTION
	vertexInputBinding.binding = 0;
	vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	vertexInputBinding.stride = sizeof(Vertex);

}

void Vulkan::createFencesAndSemaphores() {

	// CREATE FENCES
	for (uint32_t i = 0; i < ARRAY_COUNT(fences); i++) {
		VkFenceCreateInfo createInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		VKA(vkCreateFence(context->device, &createInfo, 0, &fences[i]));
	}

	// SEMAPHORES FOR SYNCHRONISATION (GPU -> GPU)
	for (uint32_t i = 0; i < ARRAY_COUNT(acquireSemaphores); i++) {
		VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VKA(vkCreateSemaphore(context->device, &createInfo, 0, &acquireSemaphores[i]));
		VKA(vkCreateSemaphore(context->device, &createInfo, 0, &releaseSemaphores[i]));

	}

}

void Vulkan::createAndAllocateCommands() {
	// CREATE COMMAN POOLS
	for (uint32_t i = 0; i < ARRAY_COUNT(commandPools); i++) {
		VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		createInfo.queueFamilyIndex = context->graphicsQueue.familyIndex;
		VKA(vkCreateCommandPool(context->device, &createInfo, 0, &commandPools[i]));
	}

	// ALLOCATE COMMAND BUFFERS
	for (uint32_t i = 0; i < ARRAY_COUNT(commandPools); i++) {
		VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.commandPool = commandPools[i];
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = 1;
		VKA(vkAllocateCommandBuffers(context->device, &allocateInfo, &commandBuffers[i]));
	}
}