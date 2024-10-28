#include "../vulkan_base.h"

void Vulkan::renderVulkan() {
	static float time = 0.0f;
	time += 0.01f;

	uint32_t imageIndex = 0; // Swapchain Index
	static uint32_t frameIndex = 0; // For command pools, command buffers, fences, semaphores

	// WAIT UNTIL GPU IS DONE TO RESET THE COMMAND POOL
	// WAIT FOR FENCES
	VKA(vkWaitForFences(context->device, 1, &fences[frameIndex], VK_TRUE, UINT64_MAX));
	// RESET FENCE

	// CREATE RENDERABLE IMAGE
	VkResult result = VK(vkAcquireNextImageKHR(context->device, swapchain.swapchain, UINT64_MAX, acquireSemaphores[frameIndex], 0, &imageIndex));
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) { // SIZE DOESNT MATCH THE WINDOW
		// SWAPCHAIN IS OUT OF DATE
		recreateSwapchain();

		return;
	}
	else {
		VKA(vkResetFences(context->device, 1, &fences[frameIndex]));
		ASSERT_VULKAN(result);
	}

	// RESET COMMAND POOL
	VKA(vkResetCommandPool(context->device, commandPools[frameIndex], 0));

	// COMMAND BUFFER INFO
	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;


	// RENDER COMMANDS
	{

		// START COMMAND BUFFER
		VkCommandBuffer commandBuffer = commandBuffers[frameIndex];
		VKA(vkBeginCommandBuffer(commandBuffer, &beginInfo));

		// RESIZE SCALES WITH PIPELINE
		VkViewport viewport = { 0.0f, 0.0f, (float)swapchain.width, (float)swapchain.height, 0.0f, 1.0f };
		VkRect2D scissor = { {0, 0}, {swapchain.width, swapchain.height} };

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);



		// CLEAR VALUE (BACKGROUND COLOR)
		VkClearValue clearValues[2] = {
			{135 / 255.0f, 206 / 255.0f, 235 / 255.0f, 1.0f}, // BACKGROUND COLOR RGBA
			{0.0f, 0.0f}, // DEPTH BUFFER CLEAR VALUE (VLLT BEIM 1. 1.0f)
		};

		// BEGIN RENDER PASS INFO
		VkRenderPassBeginInfo beginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		beginInfo.renderPass = renderPass;
		beginInfo.framebuffer = framebuffers[imageIndex];
		beginInfo.renderArea = { {0, 0}, {swapchain.width, swapchain.height} };
		beginInfo.clearValueCount = ARRAY_COUNT(clearValues);
		beginInfo.pClearValues = clearValues;
		vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

		// BIND TO PIPELINE
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
		renderInCommand(commandBuffer, frameIndex); // RENDER HERE

		// END RENDER PASS
		vkCmdEndRenderPass(commandBuffer);

		// END COMMAND BUFFER
		VKA(vkEndCommandBuffer(commandBuffer));
	}

	// SEND COMMAND BUFFER TO THE GRAPHICS QUEUE
	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[frameIndex];
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &acquireSemaphores[frameIndex];
	VkPipelineStageFlags waitMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	submitInfo.pWaitDstStageMask = &waitMask;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &releaseSemaphores[frameIndex];
	VKA(vkQueueSubmit(context->graphicsQueue.queue, 1, &submitInfo, fences[frameIndex]));

	// PRESENT THE IMAGE WITH SWAPCHAIN
	VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain.swapchain;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &releaseSemaphores[frameIndex];
	result = VK(vkQueuePresentKHR(context->graphicsQueue.queue, &presentInfo));
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) { // SIZE DOESNT MATCH THE WINDOW
		// SWAPCHAIN IS OUT OF DATE
		recreateSwapchain();
	}
	else {
		ASSERT_VULKAN(result);
	}

	frameIndex = (frameIndex + 1) % FRAMES_IN_FLIGHT;
}

void Vulkan::renderInCommand(VkCommandBuffer commandBuffer, uint32_t frameIndex) {
	renderChunk(commandBuffer, frameIndex);
}

// CHANGE FRUSTUM FOR CHUNKS ???
// CHANGE WORLD GENERATION
// GENERATE BASIC TERRAIN
void Vulkan::renderChunk(VkCommandBuffer commandBuffer, uint32_t frameIndex) {
	uint32_t dynamicOffset = 0; // singleElementSize for each more chunk
	uint32_t currentModel = 0;

	VkDeviceMemory uboMemory = uniformBuffers[0][frameIndex].memory;
	VkDescriptorSet* descriptorSet = &descriptorSets[0][frameIndex];

	cameraManager.extractFrustum(cameraManager.camera.viewProj);

	for (auto& chunkPair : worldManager.getChunks()) {
		
		auto chunk = chunkPair.second;

		// FRUSTUM CULLING (DONT LOAD UNSEEN CHUNKS)
		if (cameraManager.isSphereInFrustum(cameraManager.frustum, chunk->chunkCenter, chunk->chunkRadius)) {
			if (!chunk->vertexAndIndexBufferUploaded) {
				uploadChunkMesh(chunk.get());
				chunk->vertexAndIndexBufferUploaded = true;
			}

			glm::mat4 modelViewProj = cameraManager.camera.viewProj * chunk->modelMatrix;
			glm::mat4 modelView = cameraManager.camera.view * chunk->modelMatrix;

			UniformBufferObject ubo = {};
			ubo.modelView = modelView;
			ubo.modelViewProj = modelViewProj;

			dynamicOffset = currentModel * singleElementSize;

			if (dynamicOffset + sizeof(UniformBufferObject) > maxUniformSize) {
				uboMemory = uniformBuffers[1][frameIndex].memory;
				descriptorSet = &descriptorSets[1][frameIndex];

				dynamicOffset = 0;
				currentModel = 0;
			}

			void* data;
			VK(vkMapMemory(context->device, uboMemory, dynamicOffset, sizeof(ubo), 0, &data));
			memcpy(data, &ubo, sizeof(ubo));
			VK(vkUnmapMemory(context->device, uboMemory));

			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &chunk->vertexBuffer, offsets);
			vkCmdBindIndexBuffer(commandBuffer, chunk->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipelineLayout, 0, 1, descriptorSet, 1, &dynamicOffset);
			vkCmdDrawIndexed(commandBuffer, chunk->getIndices().size(), 1, 0, 0, 0);

			currentModel++;
		}
		
	}
	
}

void Vulkan::updateVulkan(float delta) {
	cameraManager.updateCamera(delta, swapchain.width, swapchain.height);
	worldManager.generateChunksAround(cameraManager.camera.cameraPosition, viewDistance);
	worldManager.processChunkQueue(5); // Chunks per frame
	worldManager.unloadDistantChunks(cameraManager.camera.cameraPosition, viewDistance, context->device);
}