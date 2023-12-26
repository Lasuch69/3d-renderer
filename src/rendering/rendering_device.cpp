#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include "../loader.h"
#include "rendering_device.h"
#include "shaders/material.glsl.gen.h"
#include "vulkan_context.h"

void RenderingDevice::_initAllocator() {
	VmaAllocatorCreateInfo allocatorCreateInfo = {};
	allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_0;
	allocatorCreateInfo.instance = _context->getInstance();
	allocatorCreateInfo.physicalDevice = _context->getPhysicalDevice();
	allocatorCreateInfo.device = _context->getDevice();

	if (vmaCreateAllocator(&allocatorCreateInfo, &_allocator) != VK_SUCCESS) {
		throw std::runtime_error("failed to create allocator!");
	}
}

void RenderingDevice::_initCommands() {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = _context->getCommandPool();
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkAllocateCommandBuffers(_context->getDevice(), &allocInfo, &_commandBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}
	}
}

void RenderingDevice::_initDescriptors() {
	std::array<VkDescriptorPoolSize, 2> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	// texture
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) + 1;

	if (vkCreateDescriptorPool(_context->getDevice(), &poolInfo, nullptr, &_descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}

	// global set layout

	VkDescriptorSetLayoutBinding uboBinding{};
	uboBinding.binding = 0;
	uboBinding.descriptorCount = 1;
	uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboBinding.pImmutableSamplers = nullptr;
	uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo uboLayoutInfo{};
	uboLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	uboLayoutInfo.bindingCount = 1;
	uboLayoutInfo.pBindings = &uboBinding;

	if (vkCreateDescriptorSetLayout(_context->getDevice(), &uboLayoutInfo, nullptr, &_globalSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create UBO set layout!");
	}

	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, _globalSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = _descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> uniformSets{};

	if (vkAllocateDescriptorSets(_context->getDevice(), &allocInfo, uniformSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate uniform set!");
	}

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		_uniformBuffers[i] = _createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, _uniformAllocInfos[i]);
		_uniformSets[i] = uniformSets[i];

		VkDescriptorBufferInfo uniformBufferInfo{};
		uniformBufferInfo.buffer = _uniformBuffers[i].buffer;
		uniformBufferInfo.offset = 0;
		uniformBufferInfo.range = sizeof(UniformBufferObject);

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = _uniformSets[i];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &uniformBufferInfo;

		vkUpdateDescriptorSets(_context->getDevice(), 1, &descriptorWrite, 0, nullptr);
	}

	// texture set layout

	VkDescriptorSetLayoutBinding textureBinding{};
	textureBinding.binding = 0;
	textureBinding.descriptorCount = 1;
	textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	textureBinding.pImmutableSamplers = nullptr;
	textureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo textureLayoutInfo{};
	textureLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	textureLayoutInfo.bindingCount = 1;
	textureLayoutInfo.pBindings = &textureBinding;

	if (vkCreateDescriptorSetLayout(_context->getDevice(), &textureLayoutInfo, nullptr, &_textureSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture set layout!");
	}
}

void RenderingDevice::_initPipelines() {
	MaterialShaderRD shader;

	std::vector<uint32_t> spirv = shader.getVertexCode();

	// create shader module
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = spirv.size() * sizeof(uint32_t);
	createInfo.pCode = spirv.data();

	VkShaderModule vertexModule;
	if (vkCreateShaderModule(_context->getDevice(), &createInfo, nullptr, &vertexModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create vertex module!");
	}

	spirv = shader.getFragmentCode();

	// create shader module
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = spirv.size() * sizeof(uint32_t);
	createInfo.pCode = spirv.data();

	VkShaderModule fragmentModule;
	if (vkCreateShaderModule(_context->getDevice(), &createInfo, nullptr, &fragmentModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create fragment module!");
	}

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertexModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragmentModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = _context->getMsaaSamples();

	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

	VkDescriptorSetLayout setLayouts[] = { _globalSetLayout, _textureSetLayout };

	pipelineLayoutInfo.setLayoutCount = 2;
	pipelineLayoutInfo.pSetLayouts = setLayouts;

	VkPushConstantRange pushConstant{};
	pushConstant.offset = 0;
	pushConstant.size = sizeof(MeshPushConstants);
	pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

	VkPipelineLayout pipelineLayout;
	if (vkCreatePipelineLayout(_context->getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = _context->getRenderPass();
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	VkPipeline pipeline;
	if (vkCreateGraphicsPipelines(_context->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(_context->getDevice(), fragmentModule, nullptr);
	vkDestroyShaderModule(_context->getDevice(), vertexModule, nullptr);

	_createMaterial("main", pipeline, pipelineLayout);
}

void RenderingDevice::_initScene() {
	Material *main = _getMaterial("main");

	RenderObject cube;

	cube.mesh = _getMesh("cube");
	cube.material = main;
	cube.transformMatrix = glm::mat4{ 1.0f };

	_renderObjects.push_back(cube);

	RenderObject sphere;
	sphere.mesh = _getMesh("sphere");
	sphere.material = main;
	sphere.transformMatrix = glm::mat4{
		0.5, 0, 0, 0,
		0, 0.5, 0, 0,
		0, 0, 0.5, 0,
		-1.0, -1.0, 0.5, 1
	};

	_renderObjects.push_back(sphere);

	Texture *texture = _getTexture("texture");

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = _descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &_textureSetLayout;

	if (vkAllocateDescriptorSets(_context->getDevice(), &allocInfo, &main->textureSet) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image set!");
	}

	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = texture->view;
	imageInfo.sampler = texture->sampler;

	VkWriteDescriptorSet descriptorWrite{};
	descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite.dstSet = main->textureSet;
	descriptorWrite.dstBinding = 0;
	descriptorWrite.dstArrayElement = 0;
	descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrite.descriptorCount = 1;
	descriptorWrite.pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(_context->getDevice(), 1, &descriptorWrite, 0, nullptr);
}

void RenderingDevice::_loadMeshes() {
	Mesh cube = Loader::load_mesh("models/cube.obj");
	Mesh sphere = Loader::load_mesh("models/sphere.obj");

	_uploadMesh(cube);
	_uploadMesh(sphere);

	_meshes["cube"] = cube;
	_meshes["sphere"] = sphere;
}

void RenderingDevice::_loadTextures() {
	Image image = Loader::load_image("textures/raw_plank_wall_diff_1k.png");

	Texture texture = _createTexture(image.width, image.height, image.format, image.data);
	_textures["texture"] = texture;
}

void RenderingDevice::_uploadMesh(Mesh &p_mesh) {
	// vertex
	VkDeviceSize vertexBufferSize = sizeof(p_mesh.vertices[0]) * p_mesh.vertices.size();

	// allocate buffer
	VmaAllocationInfo vertexAllocInfo;
	p_mesh.vertexBuffer = _createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexAllocInfo);

	// transfer data
	VmaAllocationInfo stagingAllocInfo;
	AllocatedBuffer stagingBuffer = _createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingAllocInfo);

	memcpy(stagingAllocInfo.pMappedData, p_mesh.vertices.data(), (size_t)vertexBufferSize);
	vmaFlushAllocation(_allocator, stagingBuffer.allocation, 0, VK_WHOLE_SIZE);
	_copyBuffer(stagingBuffer.buffer, p_mesh.vertexBuffer.buffer, vertexBufferSize);

	vmaDestroyBuffer(_allocator, stagingBuffer.buffer, stagingBuffer.allocation);

	// index
	VkDeviceSize indexBufferSize = sizeof(p_mesh.indices[0]) * p_mesh.indices.size();

	// allocate buffer
	VmaAllocationInfo indexAllocInfo;
	p_mesh.indexBuffer = _createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexAllocInfo);

	// transfer data
	stagingAllocInfo = {};
	stagingBuffer = _createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingAllocInfo);

	memcpy(stagingAllocInfo.pMappedData, p_mesh.indices.data(), (size_t)indexBufferSize);
	vmaFlushAllocation(_allocator, stagingBuffer.allocation, 0, VK_WHOLE_SIZE);
	_copyBuffer(stagingBuffer.buffer, p_mesh.indexBuffer.buffer, indexBufferSize);

	vmaDestroyBuffer(_allocator, stagingBuffer.buffer, stagingBuffer.allocation);
}

Material *RenderingDevice::_createMaterial(const std::string &p_name, VkPipeline p_pipeline, VkPipelineLayout p_pipelineLayout) {
	Material mat;

	mat.pipeline = p_pipeline;
	mat.pipelineLayout = p_pipelineLayout;

	_materials[p_name] = mat;
	return &_materials[p_name];
}

Mesh *RenderingDevice::_getMesh(const std::string &p_name) {
	// search for the object, and return nullptr if not found
	auto it = _meshes.find(p_name);
	if (it == _meshes.end()) {
		return nullptr;
	} else {
		return &(*it).second;
	}
}

Material *RenderingDevice::_getMaterial(const std::string &p_name) {
	// search for the object, and return nullptr if not found
	auto it = _materials.find(p_name);
	if (it == _materials.end()) {
		return nullptr;
	} else {
		return &(*it).second;
	}
}

Texture *RenderingDevice::_getTexture(const std::string &p_name) {
	// search for the object, and return nullptr if not found
	auto it = _textures.find(p_name);
	if (it == _textures.end()) {
		return nullptr;
	} else {
		return &(*it).second;
	}
}

void RenderingDevice::_drawObjects(VkCommandBuffer p_commandBuffer, RenderObject *p_renderObjects, uint32_t p_count) {
	Mesh *lastMesh = nullptr;
	Material *lastMaterial = nullptr;

	for (int i = 0; i < p_count; i++) {
		RenderObject object = p_renderObjects[i];

		if (object.material != lastMaterial) {
			vkCmdBindPipeline(p_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);

			VkExtent2D extent = _context->getSwapchainExtent();

			VkViewport viewport{};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = (float)extent.width;
			viewport.height = (float)extent.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(p_commandBuffer, 0, 1, &viewport);

			VkRect2D scissor{};
			scissor.offset = { 0, 0 };
			scissor.extent = extent;
			vkCmdSetScissor(p_commandBuffer, 0, 1, &scissor);

			lastMaterial = object.material;
		}

		MeshPushConstants constants;
		constants.model = object.transformMatrix;

		// upload the mesh to the gpu via pushconstants
		vkCmdPushConstants(p_commandBuffer, object.material->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

		// only bind the mesh if its a different one from last bind
		if (object.mesh != lastMesh) {
			// bind the mesh vertex buffer with offset 0
			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(p_commandBuffer, 0, 1, &object.mesh->vertexBuffer.buffer, &offset);

			// bind descriptors
			vkCmdBindDescriptorSets(p_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 0, 1, &_uniformSets[i], 0, nullptr);
			vkCmdBindDescriptorSets(p_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 1, 1, &object.material->textureSet, 0, nullptr);

			// bind index buffer
			vkCmdBindIndexBuffer(p_commandBuffer, object.mesh->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

			lastMesh = object.mesh;
		}

		vkCmdDrawIndexed(p_commandBuffer, static_cast<uint32_t>(object.mesh->indices.size()), 1, 0, 0, 0);
	}
}

void RenderingDevice::_updateUniformBuffer(uint32_t p_currentFrame) {
	VkExtent2D extent = _context->getSwapchainExtent();

	glm::mat4 view = glm::lookAt(glm::vec3(3.0f, 3.0f, 3.0f), glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)extent.width / (float)extent.height, 0.1f, 100.0f);
	projection[1][1] *= -1;

	UniformBufferObject ubo{};
	ubo.view = view;
	ubo.proj = projection;

	memcpy(_uniformAllocInfos[_currentFrame].pMappedData, &ubo, sizeof(ubo));
}

AllocatedBuffer RenderingDevice::_createBuffer(VkDeviceSize p_size, VkBufferUsageFlags p_usage, VmaAllocationInfo &p_allocInfo) {
	VkBufferCreateInfo bufCreateInfo{};
	bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufCreateInfo.size = p_size;
	bufCreateInfo.usage = p_usage;

	VmaAllocationCreateInfo allocCreateInfo{};
	allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

	AllocatedBuffer buffer;
	if (vmaCreateBuffer(_allocator, &bufCreateInfo, &allocCreateInfo, &buffer.buffer, &buffer.allocation, &p_allocInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer!");
	}

	return buffer;
}

void RenderingDevice::_copyBuffer(VkBuffer &p_srcBuffer, VkBuffer &p_dstBuffer, VkDeviceSize p_size) {
	VkCommandBuffer commandBuffer = _beginSingleTimeCommands();

	VkBufferCopy bufCopy{};
	bufCopy.srcOffset = 0;
	bufCopy.dstOffset = 0;
	bufCopy.size = p_size;

	vkCmdCopyBuffer(commandBuffer, p_srcBuffer, p_dstBuffer, 1, &bufCopy);

	_endSingleTimeCommands(commandBuffer);
}

Texture RenderingDevice::_createTexture(uint32_t p_width, uint32_t p_height, VkFormat p_format, const std::vector<uint8_t> &p_data) {
	uint8_t mipmaps = static_cast<uint32_t>(std::floor(std::log2(std::max(p_width, p_height)))) + 1;

	AllocatedImage textureImage = _createImage(p_width, p_height, mipmaps, VK_SAMPLE_COUNT_1_BIT, p_format, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

	// create staging buffer
	VkDeviceSize size = p_width * p_height * 4;

	VmaAllocationInfo stagingAllocInfo;
	AllocatedBuffer stagingBuffer = _createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingAllocInfo);
	memcpy(stagingAllocInfo.pMappedData, p_data.data(), (size_t)size);
	vmaFlushAllocation(_allocator, stagingBuffer.allocation, 0, VK_WHOLE_SIZE);

	// transition image layout
	VkCommandBuffer commandBuffer = _beginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = textureImage.image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipmaps;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

	vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

	_endSingleTimeCommands(commandBuffer);

	// copy buffer to image
	commandBuffer = _beginSingleTimeCommands();

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;
	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = {
		p_width,
		p_height,
		1
	};

	vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.buffer, textureImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	_endSingleTimeCommands(commandBuffer);

	// generate mipmaps
	_generateMipmaps(textureImage.image, p_format, p_width, p_height, mipmaps);

	// destroy staging buffer
	vmaDestroyBuffer(_allocator, stagingBuffer.buffer, stagingBuffer.allocation);

	// image view
	VkImageView textureImageView = _createImageView(textureImage.image, p_format, VK_IMAGE_ASPECT_COLOR_BIT, mipmaps);

	// sampler
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(_context->getPhysicalDevice(), &properties);

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.minLod = 0.0f; // optional
	samplerInfo.maxLod = static_cast<float>(mipmaps);
	samplerInfo.mipLodBias = 0.0f; // optional

	VkSampler textureSampler;
	if (vkCreateSampler(_context->getDevice(), &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}

	return Texture{ textureImage, textureImageView, textureSampler };
}

void RenderingDevice::_generateMipmaps(VkImage p_image, VkFormat p_imageFormat, int32_t p_texWidth, int32_t p_texHeight, uint32_t p_mipLevels) {
	// check if image format supports linear blitting
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(_context->getPhysicalDevice(), p_imageFormat, &formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		throw std::runtime_error("texture image format does not support linear blitting!");
	}

	VkCommandBuffer commandBuffer = _beginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = p_image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = p_texWidth;
	int32_t mipHeight = p_texHeight;

	for (uint32_t i = 1; i < p_mipLevels; i++) {
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(commandBuffer,
				p_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				p_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

		if (mipWidth > 1)
			mipWidth /= 2;
		if (mipHeight > 1)
			mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = p_mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

	_endSingleTimeCommands(commandBuffer);
}

AllocatedImage RenderingDevice::_createImage(uint32_t p_width, uint32_t p_height, uint32_t p_mipmaps, VkSampleCountFlagBits p_numSamples, VkFormat p_format, VkImageUsageFlags p_usage) {
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = p_width;
	imageInfo.extent.height = p_height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = p_mipmaps;
	imageInfo.arrayLayers = 1;
	imageInfo.format = p_format;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = p_usage;
	imageInfo.samples = p_numSamples;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	allocCreateInfo.priority = 1.0f;

	AllocatedImage allocatedImage;
	if (vmaCreateImage(_allocator, &imageInfo, &allocCreateInfo, &allocatedImage.image, &allocatedImage.allocation, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	return allocatedImage;
}

VkImageView RenderingDevice::_createImageView(VkImage p_image, VkFormat p_format, VkImageAspectFlags p_aspectFlags, uint32_t p_mipmaps) {
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = p_image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = p_format;
	viewInfo.subresourceRange.aspectMask = p_aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = p_mipmaps;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(_context->getDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}

VkCommandBuffer RenderingDevice::_beginSingleTimeCommands() {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = _context->getCommandPool();
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(_context->getDevice(), &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void RenderingDevice::_endSingleTimeCommands(VkCommandBuffer p_commandBuffer) {
	vkEndCommandBuffer(p_commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &p_commandBuffer;

	vkQueueSubmit(_context->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(_context->getGraphicsQueue());

	vkFreeCommandBuffers(_context->getDevice(), _context->getCommandPool(), 1, &p_commandBuffer);
}

void RenderingDevice::windowCreate(GLFWwindow *p_window, uint32_t p_width, uint32_t p_height) {
	_context->windowCreate(p_window, p_width, p_height);

	_initAllocator();
	std::cout << "Allocator initialized!\n";

	_initCommands();
	std::cout << "Commands initialized!\n";

	_initDescriptors();
	std::cout << "Descriptors initialized!\n";

	_initPipelines();
	std::cout << "Pipelines initialized!\n";

	_loadMeshes();
	std::cout << "Meshes loaded!\n";

	_loadTextures();
	std::cout << "Textures loaded!\n";

	_initScene();
	std::cout << "Scene initialized!\n";
}

void RenderingDevice::windowResize(uint32_t p_width, uint32_t p_height) {
	_context->windowResize(p_width, p_height);
}

void RenderingDevice::draw() {
	SyncObject sync = _context->getSyncObject(_currentFrame);
	VkCommandBuffer commandBuffer = _commandBuffers[_currentFrame];

	vkWaitForFences(_context->getDevice(), 1, &sync.renderFence, VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(_context->getDevice(), _context->getSwapchain(), UINT64_MAX, sync.presentSemaphore, VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		_context->recreateSwapchain();
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	_updateUniformBuffer(_currentFrame);

	vkResetFences(_context->getDevice(), 1, &sync.renderFence);

	vkResetCommandBuffer(commandBuffer, 0);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = _context->getRenderPass();
	renderPassInfo.framebuffer = _context->getFramebuffer(imageIndex);
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = _context->getSwapchainExtent();

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	_drawObjects(commandBuffer, _renderObjects.data(), _renderObjects.size());

	vkCmdEndRenderPass(commandBuffer);

	vkEndCommandBuffer(commandBuffer);

	_context->submit(_currentFrame, imageIndex, commandBuffer);

	_currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void RenderingDevice::waitIdle() {
	vkDeviceWaitIdle(_context->getDevice());
}

RenderingDevice::RenderingDevice(bool p_validationLayers) {
	_context = new VulkanContext(p_validationLayers);
}

RenderingDevice::~RenderingDevice() {
	free(_context);
}
