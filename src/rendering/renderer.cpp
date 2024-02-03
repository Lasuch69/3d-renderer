#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "renderer.h"

#include "shaders/material.glsl.gen.h"
#include "shaders/tonemapping.glsl.gen.h"

VkShaderModule createShaderModule(VkDevice device, const std::vector<uint32_t> &spirv) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = spirv.size() * sizeof(uint32_t);
	createInfo.pCode = spirv.data();

	VkShaderModule shaderModule;
	VK_CHECK(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule), "Failed to create shader module!");

	return shaderModule;
}

void Renderer::_initAllocator() {
	VmaAllocatorCreateInfo allocatorCreateInfo = {};
	allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_0;
	allocatorCreateInfo.instance = _context->getInstance();
	allocatorCreateInfo.physicalDevice = _context->getPhysicalDevice();
	allocatorCreateInfo.device = _context->getDevice();

	VK_CHECK(vmaCreateAllocator(&allocatorCreateInfo, &_allocator), "Failed to create allocator!");
}

void Renderer::_initCommands() {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = _context->getCommandPool();
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VK_CHECK(vkAllocateCommandBuffers(_context->getDevice(), &allocInfo, &_commandBuffers[i]), "Failed to allocate command buffers!");
	}
}

void Renderer::_initDescriptors() {
	std::array<VkDescriptorPoolSize, 4> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	// subpass
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	poolSizes[1].descriptorCount = 1;

	// texture
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[2].descriptorCount = 1;

	// ImGui
	poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[3].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) + 3;

	VK_CHECK(vkCreateDescriptorPool(_context->getDevice(), &poolInfo, nullptr, &_descriptorPool), "Failed to create descriptor pool!");

	// uniform set layout

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

	VK_CHECK(vkCreateDescriptorSetLayout(_context->getDevice(), &uboLayoutInfo, nullptr, &_uniformSetLayout), "Failed to create uniform buffer object set layout!");

	{
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, _uniformSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = _descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();

		std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> uniformSets{};

		VK_CHECK(vkAllocateDescriptorSets(_context->getDevice(), &allocInfo, uniformSets.data()), "Failed to allocate uniform set!");

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
	}

	// subpass set layout

	VkDescriptorSetLayoutBinding subpassBinding{};
	subpassBinding.binding = 0;
	subpassBinding.descriptorCount = 1;
	subpassBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
	subpassBinding.pImmutableSamplers = nullptr;
	subpassBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo subpassLayoutInfo{};
	subpassLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	subpassLayoutInfo.bindingCount = 1;
	subpassLayoutInfo.pBindings = &subpassBinding;

	VK_CHECK(vkCreateDescriptorSetLayout(_context->getDevice(), &subpassLayoutInfo, nullptr, &_subpassSetLayout), "Failed to create subpass set layout!");

	{
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = _descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &_subpassSetLayout;

		VK_CHECK(vkAllocateDescriptorSets(_context->getDevice(), &allocInfo, &_subpassSet), "Failed to allocate subpass set!");

		_writeImageSet(_subpassSet, _context->getColorImageView(), VK_NULL_HANDLE, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
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

	VK_CHECK(vkCreateDescriptorSetLayout(_context->getDevice(), &textureLayoutInfo, nullptr, &_textureSetLayout), "Failed to create texture set layout!");
}

// ImGui error check
static void vkCheckResult(VkResult err) {
	if (err == 0)
		return;

	fprintf(stderr, "ERROR: %d\n", err);

	if (err < 0)
		abort();
}

void Renderer::initImGui(GLFWwindow *pWindow) {
	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForVulkan(pWindow, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = _context->getInstance();
	init_info.PhysicalDevice = _context->getPhysicalDevice();
	init_info.Device = _context->getDevice();
	init_info.QueueFamily = _context->getGraphicsQueueFamily();
	init_info.Queue = _context->getGraphicsQueue();
	init_info.PipelineCache = nullptr;
	init_info.DescriptorPool = _descriptorPool;
	init_info.Subpass = 0;
	init_info.MinImageCount = 2;
	init_info.ImageCount = 2;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.Allocator = nullptr;
	init_info.CheckVkResultFn = vkCheckResult;
	ImGui_ImplVulkan_Init(&init_info, _context->getRenderPass());

	VkCommandBuffer commandBuffer = _beginSingleTimeCommands();
	ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
	_endSingleTimeCommands(commandBuffer);

	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void Renderer::_initPipelines() {
	{
		MaterialShaderRD shader;

		VkShaderModule vertexModule = createShaderModule(_context->getDevice(), shader.getVertexCode());
		VkShaderModule fragmentModule = createShaderModule(_context->getDevice(), shader.getFragmentCode());

		VkDescriptorSetLayout setLayouts[] = { _uniformSetLayout, _textureSetLayout };

		VkPushConstantRange pushConstant{};
		pushConstant.offset = 0;
		pushConstant.size = sizeof(MeshPushConstants);
		pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkPipelineLayout pipelineLayout = _createPipelineLayout(setLayouts, 2, &pushConstant, 1);
		VkPipeline pipeline = _createPipeline(pipelineLayout, vertexModule, fragmentModule, 0);

		vkDestroyShaderModule(_context->getDevice(), fragmentModule, nullptr);
		vkDestroyShaderModule(_context->getDevice(), vertexModule, nullptr);

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = _descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &_textureSetLayout;

		VkDescriptorSet textureSet;
		VK_CHECK(vkAllocateDescriptorSets(_context->getDevice(), &allocInfo, &textureSet), "Failed to allocate texture set!");

		_material = { textureSet, pipelineLayout, pipeline };
	}

	{
		TonemappingShaderRD shader;

		VkShaderModule vertexModule = createShaderModule(_context->getDevice(), shader.getVertexCode());
		VkShaderModule fragmentModulee = createShaderModule(_context->getDevice(), shader.getFragmentCode());

		VkPipelineLayout pipelineLayout = _createPipelineLayout(&_subpassSetLayout, 1, nullptr, 0);
		VkPipeline pipeline = _createPipeline(pipelineLayout, vertexModule, fragmentModulee, 1);

		vkDestroyShaderModule(_context->getDevice(), fragmentModulee, nullptr);
		vkDestroyShaderModule(_context->getDevice(), vertexModule, nullptr);

		_tonemapping = { VK_NULL_HANDLE, pipelineLayout, pipeline };
	}
}

void Renderer::_uploadMesh(Mesh *pMesh) {
	// vertex
	VkDeviceSize vertexBufferSize = sizeof(pMesh->vertices[0]) * pMesh->vertices.size();

	// allocate buffer
	VmaAllocationInfo vertexAllocInfo;
	pMesh->vertexBuffer = _createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexAllocInfo);

	// transfer data
	VmaAllocationInfo stagingAllocInfo;
	AllocatedBuffer stagingBuffer = _createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingAllocInfo);

	memcpy(stagingAllocInfo.pMappedData, pMesh->vertices.data(), (size_t)vertexBufferSize);
	vmaFlushAllocation(_allocator, stagingBuffer.allocation, 0, VK_WHOLE_SIZE);
	_copyBuffer(stagingBuffer.buffer, pMesh->vertexBuffer.buffer, vertexBufferSize);

	vmaDestroyBuffer(_allocator, stagingBuffer.buffer, stagingBuffer.allocation);

	// index
	VkDeviceSize indexBufferSize = sizeof(pMesh->indices[0]) * pMesh->indices.size();

	// allocate buffer
	VmaAllocationInfo indexAllocInfo;
	pMesh->indexBuffer = _createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexAllocInfo);

	// transfer data
	stagingAllocInfo = {};
	stagingBuffer = _createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingAllocInfo);

	memcpy(stagingAllocInfo.pMappedData, pMesh->indices.data(), (size_t)indexBufferSize);
	vmaFlushAllocation(_allocator, stagingBuffer.allocation, 0, VK_WHOLE_SIZE);
	_copyBuffer(stagingBuffer.buffer, pMesh->indexBuffer.buffer, indexBufferSize);

	vmaDestroyBuffer(_allocator, stagingBuffer.buffer, stagingBuffer.allocation);
}

void Renderer::_updateUniformBuffer(uint32_t p_currentFrame) {
	VkExtent2D extent = _context->getSwapchainExtent();
	float aspect = (float)extent.width / (float)extent.height;

	UniformBufferObject ubo{};
	ubo.view = _camera->getViewMatrix();
	ubo.proj = _camera->getProjectionMatrix(aspect);

	memcpy(_uniformAllocInfos[_currentFrame].pMappedData, &ubo, sizeof(ubo));
}

void Renderer::_writeImageSet(VkDescriptorSet dstSet, VkImageView imageView, VkSampler sampler, VkDescriptorType descriptorType) {
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = imageView;
	imageInfo.sampler = sampler;

	VkWriteDescriptorSet writeDescriptorSet = {};
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.dstSet = dstSet;
	writeDescriptorSet.dstBinding = 0;
	writeDescriptorSet.dstArrayElement = 0;
	writeDescriptorSet.descriptorType = descriptorType;
	writeDescriptorSet.descriptorCount = 1;
	writeDescriptorSet.pImageInfo = &imageInfo;

	vkUpdateDescriptorSets(_context->getDevice(), 1, &writeDescriptorSet, 0, nullptr);
}

AllocatedBuffer Renderer::_createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationInfo &allocInfo) {
	VkBufferCreateInfo bufCreateInfo{};
	bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufCreateInfo.size = size;
	bufCreateInfo.usage = usage;

	VmaAllocationCreateInfo allocCreateInfo{};
	allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

	AllocatedBuffer buffer;
	VK_CHECK(vmaCreateBuffer(_allocator, &bufCreateInfo, &allocCreateInfo, &buffer.buffer, &buffer.allocation, &allocInfo), "Failed to allocate buffer!");

	return buffer;
}

void Renderer::_copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	VkCommandBuffer commandBuffer = _beginSingleTimeCommands();

	VkBufferCopy bufCopy{};
	bufCopy.srcOffset = 0;
	bufCopy.dstOffset = 0;
	bufCopy.size = size;

	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &bufCopy);

	_endSingleTimeCommands(commandBuffer);
}

Texture Renderer::_createTexture(uint32_t width, uint32_t height, VkFormat format, const std::vector<uint8_t> &data) {
	uint8_t mipmaps = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

	AllocatedImage textureImage = _createImage(width, height, format, mipmaps, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

	// create staging buffer
	VkDeviceSize size = width * height * 4;

	VmaAllocationInfo stagingAllocInfo;
	AllocatedBuffer stagingBuffer = _createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingAllocInfo);
	memcpy(stagingAllocInfo.pMappedData, data.data(), (size_t)size);
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
		width,
		height,
		1
	};

	vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.buffer, textureImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	_endSingleTimeCommands(commandBuffer);

	// generate mipmaps
	bool isMipmapValid = _generateMipmaps(width, height, format, mipmaps, textureImage.image);

	if (!isMipmapValid) {
		mipmaps = 1;
	}

	// destroy staging buffer
	vmaDestroyBuffer(_allocator, stagingBuffer.buffer, stagingBuffer.allocation);

	// image view
	VkImageView textureImageView = _createImageView(textureImage.image, format, mipmaps, VK_IMAGE_ASPECT_COLOR_BIT);

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
	VK_CHECK(vkCreateSampler(_context->getDevice(), &samplerInfo, nullptr, &textureSampler), "Failed to create texture sampler!");

	return Texture{ textureImage, textureImageView, textureSampler };
}

bool Renderer::_generateMipmaps(int32_t width, int32_t height, VkFormat format, uint32_t mipmaps, VkImage image) {
	// check if image format supports linear blitting
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(_context->getPhysicalDevice(), format, &formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		printf("Texture image format does not support linear blitting!");

		return false;
	}

	VkCommandBuffer commandBuffer = _beginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = width;
	int32_t mipHeight = height;

	for (uint32_t i = 1; i < mipmaps; i++) {
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
				image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
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

	barrier.subresourceRange.baseMipLevel = mipmaps - 1;
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

	return true;
}

AllocatedImage Renderer::_createImage(uint32_t width, uint32_t height, VkFormat format, uint32_t mipmaps, VkImageUsageFlags usage) {
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipmaps;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo allocCreateInfo = {};
	allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
	allocCreateInfo.priority = 1.0f;

	AllocatedImage allocatedImage;
	VK_CHECK(vmaCreateImage(_allocator, &imageInfo, &allocCreateInfo, &allocatedImage.image, &allocatedImage.allocation, nullptr), "Failed to create image!");

	return allocatedImage;
}

VkImageView Renderer::_createImageView(VkImage image, VkFormat format, uint32_t mipmaps, VkImageAspectFlags aspectFlags) {
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipmaps;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	VK_CHECK(vkCreateImageView(_context->getDevice(), &viewInfo, nullptr, &imageView), "Failed to create image view!");

	return imageView;
}

VkPipelineLayout Renderer::_createPipelineLayout(VkDescriptorSetLayout *pSetLayouts, uint32_t layoutCount, VkPushConstantRange *pPushConstants, uint32_t constantCount) {
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = layoutCount;
	pipelineLayoutInfo.pSetLayouts = pSetLayouts;
	pipelineLayoutInfo.pushConstantRangeCount = constantCount;
	pipelineLayoutInfo.pPushConstantRanges = pPushConstants;

	VkPipelineLayout pipelineLayout;
	VK_CHECK(vkCreatePipelineLayout(_context->getDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout), "Failed to create pipeline layout!");

	return pipelineLayout;
}

VkPipeline Renderer::_createPipeline(VkPipelineLayout layout, VkShaderModule vertex, VkShaderModule fragment, uint32_t subpass) {
	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertex;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragment;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	auto bindingDescription = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
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
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

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
	pipelineInfo.layout = layout;
	pipelineInfo.renderPass = _context->getRenderPass();
	pipelineInfo.subpass = subpass;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	VkPipeline pipeline;
	VK_CHECK(vkCreateGraphicsPipelines(_context->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline), "Failed to create graphics pipeline!");

	return pipeline;
}

VkCommandBuffer Renderer::_beginSingleTimeCommands() {
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

void Renderer::_endSingleTimeCommands(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(_context->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(_context->getGraphicsQueue());

	vkFreeCommandBuffers(_context->getDevice(), _context->getCommandPool(), 1, &commandBuffer);
}

void Renderer::_drawObjects(VkCommandBuffer commandBuffer) {
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _material.pipeline);

	VkExtent2D extent = _context->getSwapchainExtent();

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)extent.width;
	viewport.height = (float)extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = extent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	for (auto iter = _objects.begin(); iter != _objects.end(); ++iter) {
		Object object = iter->second;

		MeshPushConstants constants;
		constants.model = object.transform;

		// upload the mesh to the gpu via pushconstants
		vkCmdPushConstants(commandBuffer, _material.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

		// bind the mesh vertex buffer with offset 0
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &object.pMesh->vertexBuffer.buffer, &offset);

		// bind descriptors
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _material.pipelineLayout, 0, 1, &_uniformSets[_currentFrame], 0, nullptr);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _material.pipelineLayout, 1, 1, &_material.textureSet, 0, nullptr);

		// bind index buffer
		vkCmdBindIndexBuffer(commandBuffer, object.pMesh->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(object.pMesh->indices.size()), 1, 0, 0, 0);
	}
}

Camera *Renderer::getCamera() {
	return _camera;
}

void Renderer::windowCreate(GLFWwindow *pWindow, uint32_t width, uint32_t height) {
	_context->windowCreate(pWindow, width, height);

	_initAllocator();
	_initCommands();
	_initDescriptors();
	_initPipelines();
}

void Renderer::windowResize(uint32_t width, uint32_t height) {
	_context->windowResize(width, height);
}

RID Renderer::objectCreate() {
	RID rid = _objectIdx;
	_objectIdx += 1;

	_objects[rid] = {};
	return rid;
}

void Renderer::objectSetMesh(RID object, Mesh *pMesh) {
	auto iter = _objects.find(object);
	if (iter == _objects.end()) {
		return;
	}

	Object *pObject;
	pObject = &(*iter).second;
	pObject->pMesh = pMesh;
}

void Renderer::objectSetTexture(RID object, Texture *pTexture) {
	auto iter = _objects.find(object);
	if (iter == _objects.end()) {
		return;
	}

	Object *pObject;
	pObject = &(*iter).second;
	pObject->pTexture = pTexture;
}

void Renderer::objectSetTransform(RID object, const glm::mat4 &transform) {
	auto iter = _objects.find(object);
	if (iter == _objects.end()) {
		return;
	}

	Object *pObject;
	pObject = &(*iter).second;
	pObject->transform = transform;
}

void Renderer::objectFree(RID object) {
	_objects.erase(object);
}

Mesh Renderer::meshCreate(std::vector<Vertex> vertices, std::vector<uint32_t> indices) {
	Mesh mesh = {};

	mesh.vertices = vertices;
	mesh.indices = indices;

	_uploadMesh(&mesh);
	return mesh;
}

Texture Renderer::textureCreate(uint32_t width, uint32_t height, VkFormat format, const std::vector<uint8_t> &data) {
	Texture texture = _createTexture(width, height, format, data);

	// TODO: this does not belong here!
	_writeImageSet(_material.textureSet, texture.view, texture.sampler, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	return texture;
}

void Renderer::draw() {
	SyncObject sync = _context->getSyncObject(_currentFrame);
	VkCommandBuffer commandBuffer = _commandBuffers[_currentFrame];

	vkWaitForFences(_context->getDevice(), 1, &sync.renderFence, VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(_context->getDevice(), _context->getSwapchain(), UINT64_MAX, sync.presentSemaphore, VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		_context->recreateSwapchain();
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		printf("Failed to acquire swapchain image!");
	}

	// update subpass attachment
	vkDeviceWaitIdle(_context->getDevice());
	_writeImageSet(_subpassSet, _context->getColorImageView(), VK_NULL_HANDLE, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);

	_updateUniformBuffer(_currentFrame);

	vkResetFences(_context->getDevice(), 1, &sync.renderFence);

	vkResetCommandBuffer(commandBuffer, 0);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo), "Failed to begin recording command buffer!");

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = _context->getRenderPass();
	renderPassInfo.framebuffer = _context->getFramebuffer(imageIndex);
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = _context->getSwapchainExtent();

	std::array<VkClearValue, 3> clearValues{};
	clearValues[1].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[2].depthStencil = { 1.0f, 0 };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	_drawObjects(commandBuffer);

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

	// Tonemapping
	vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _tonemapping.pipeline);

	VkExtent2D extent = _context->getSwapchainExtent();

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)extent.width;
	viewport.height = (float)extent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = extent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _tonemapping.pipelineLayout, 0, 1, &_subpassSet, 0, nullptr);

	vkCmdDraw(commandBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(commandBuffer);

	vkEndCommandBuffer(commandBuffer);

	_context->submit(_currentFrame, imageIndex, commandBuffer);

	_currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::waitIdle() {
	vkDeviceWaitIdle(_context->getDevice());
}

Renderer::Renderer(bool useValidation) {
	_context = new VulkanContext(useValidation);
	_camera = new Camera();
}

Renderer::~Renderer() {
	free(_camera);
	free(_context);
}
