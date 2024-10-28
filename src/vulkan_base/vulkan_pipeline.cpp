#include "../vulkan_base.h"

VkShaderModule Vulkan::createShaderModule(const char* shaderFilename) {
	VkShaderModule result = {};

	// READ SHADER FILE
	FILE* file = fopen(shaderFilename, "rb");
	if (!file) {
		LOG_ERROR("Shader not found: ", shaderFilename);
		return result;
	}
	fseek(file, 0, SEEK_END);
	long fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);
	assert((fileSize & 0x03) == 0); // spv 4 bytes per instruction
	uint8_t* buffer = new uint8_t[fileSize];
	fread(buffer, 1, fileSize, file);

	// CREATE SHADER MODULE
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = fileSize;
	createInfo.pCode = (uint32_t*)buffer;
	VKA(vkCreateShaderModule(context->device, &createInfo, 0, &result));

	// DELETE BUFFER AND CLOSE FILE
	delete[] buffer;
	fclose(file);

	return result;
}

bool Vulkan::createPipeline(const char* vertexShaderFilename, const char* fragmentShaderFilename, VkVertexInputBindingDescription* binding) {

	// CREATE SHADER MODULES
	VkShaderModule vertexShaderModule = createShaderModule(vertexShaderFilename);
	VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderFilename);

	// 2 SHADER PIPELINES
	VkPipelineShaderStageCreateInfo shaderStages[2];

	// VERTEX SHADER
	shaderStages[0] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = vertexShaderModule;
	shaderStages[0].pName = "main";

	// FRAGMENT SHADER
	shaderStages[1] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = fragmentShaderModule;
	shaderStages[1].pName = "main";



	// VERTEX INPUT
	VkPipelineVertexInputStateCreateInfo vertexInputState = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

	// VERTEX INPUT STATE
	vertexInputState.vertexBindingDescriptionCount = binding ? 1 : 0;
	vertexInputState.pVertexBindingDescriptions = binding;
	vertexInputState.vertexAttributeDescriptionCount = ARRAY_COUNT(vertexAttributeDescriptions);
	vertexInputState.pVertexAttributeDescriptions = vertexAttributeDescriptions;

	// INPUT ASSEMBLY
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// VIEWPORT
	VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1; // MINIMAP?

	// RASTERISATION
	VkPipelineRasterizationStateCreateInfo rasterizationState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	//rasterizationState.depthClampEnable = VK_FALSE;
	//rasterizationState.rasterizerDiscardEnable = VK_FALSE;
	//rasterizationState.polygonMode = VK_POLYGON_MODE_FILL; //VK_POLYGON_MODE_FILL
	rasterizationState.lineWidth = 1.0f;
	////rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

	//rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;

	// MULTISAMPLE
	VkPipelineMultisampleStateCreateInfo multisampleState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// DEPTH STENCIL STATE
	VkPipelineDepthStencilStateCreateInfo depthStencilState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	depthStencilState.depthTestEnable = VK_TRUE;
	depthStencilState.depthWriteEnable = VK_TRUE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_GREATER; // GREATER OR EQUAL ?
	depthStencilState.minDepthBounds = 0.0f;
	depthStencilState.maxDepthBounds = 1.0f;

	// COLOR BLEND
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	VkPipelineColorBlendStateCreateInfo colorBlendState = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &colorBlendAttachment;

	// DYNAMIC STATE
	VkPipelineDynamicStateCreateInfo dynamicState = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
	VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	dynamicState.dynamicStateCount = ARRAY_COUNT(dynamicStates);
	dynamicState.pDynamicStates = dynamicStates;



	// CREATE PIPELINE LAYOUT
	{
		VkPipelineLayoutCreateInfo createInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
		createInfo.setLayoutCount = 1;
		createInfo.pSetLayouts = &descriptorSetLayout;
		VKA(vkCreatePipelineLayout(context->device, &createInfo, 0, &pipeline.pipelineLayout));
	}

	// CREATE GRAPHIC PIPELINES
	{
		VkGraphicsPipelineCreateInfo createInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
		createInfo.stageCount = ARRAY_COUNT(shaderStages);
		createInfo.pStages = shaderStages;
		createInfo.pVertexInputState = &vertexInputState;
		createInfo.pInputAssemblyState = &inputAssemblyState;
		createInfo.pViewportState = &viewportState;
		createInfo.pRasterizationState = &rasterizationState;
		createInfo.pMultisampleState = &multisampleState;
		createInfo.pDepthStencilState = &depthStencilState;
		createInfo.pColorBlendState = &colorBlendState;
		createInfo.pDynamicState = &dynamicState;
		createInfo.layout = pipeline.pipelineLayout;
		createInfo.renderPass = renderPass;
		createInfo.subpass = 0;


		VKA(vkCreateGraphicsPipelines(context->device, 0, 1, &createInfo, 0, &pipeline.pipeline));
	}

	// MODULE CAN BE DESTROYED AFTER PIPELINE CREATION
	VK(vkDestroyShaderModule(context->device, vertexShaderModule, 0));
	VK(vkDestroyShaderModule(context->device, fragmentShaderModule, 0));
	return true;
}

void Vulkan::cleanupPipeline() {
	VK(vkDestroyPipeline(context->device, pipeline.pipeline, 0));
	VK(vkDestroyPipelineLayout(context->device, pipeline.pipelineLayout, 0));
}