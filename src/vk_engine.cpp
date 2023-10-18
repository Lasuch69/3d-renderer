#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <stb_image.h>

#include "vk_engine.h"

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

void VulkanEngine::resizeWindow(int width, int height) {
	if (width == _window.width && height == _window.height) {
		return;
	}

	_window.width = width;
	_window.height = height;
	_window.resized = true;
}

void VulkanEngine::init() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	_glfwWindow = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

	glfwSetWindowUserPointer(_glfwWindow, this);
	glfwSetFramebufferSizeCallback(_glfwWindow, windowResizedCallback);

	std::cout << "Window initialized!\n";

	initVulkan();
	std::cout << "Vulkan ready!\n";

	initCommands();
	std::cout << "Commands ready!\n";

	initSyncObjects();
	std::cout << "Sync objects ready!\n";

	initDescriptors();
	std::cout << "Descriptors initialized!\n";

	initPipelines();
	std::cout << "Pipelines initialized!\n";

	loadMeshes();
	std::cout << "Meshes loaded!\n";

	loadTextures();
	std::cout << "Textures loaded!\n";

	initScene();
	std::cout << "Scene initialized!\n";

	initImGui();
	std::cout << "ImGui initialized!\n";

	_initialized = true;
}

void VulkanEngine::run() {
	while (!glfwWindowShouldClose(_glfwWindow)) {
		glfwPollEvents();

		int state = glfwGetKey(_glfwWindow, GLFW_KEY_F6);
		if (state != _keyState) {
			_keyState = state;
			if (state == GLFW_PRESS)
				_drawDebugMenu = !_drawDebugMenu;
		}

		if (_drawDebugMenu) {
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			{
				static bool show_demo_window = false;
				static bool show_another_window = false;

				static float clear_color[] = { 0.0f, 0.0f, 0.0f };

				static float f = 0.0f;
				static int counter = 0;

				ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

				ImGui::Text("This is some useful text."); // Display some text (you can use a format strings too)
				ImGui::Checkbox("Demo Window", &show_demo_window); // Edit bools storing our window open/close state
				ImGui::Checkbox("Another Window", &show_another_window);

				ImGui::SliderFloat("float", &f, 0.0f, 1.0f); // Edit 1 float using a slider from 0.0f to 1.0f
				ImGui::ColorEdit3("clear color", (float *)&clear_color); // Edit 3 floats representing a color

				if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
					counter++;
				ImGui::SameLine();
				ImGui::Text("counter = %d", counter);

				ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / _io->Framerate, _io->Framerate);
				ImGui::End();
			}

			ImGui::Render();
		}

		draw();
	}

	vkDeviceWaitIdle(_device);
}

void VulkanEngine::cleanup() {
	if (_initialized) {
		// Clean ImGui stuff
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

		vmaDestroyAllocator(_allocator);

		cleanupSwapChain(&_window);

		vkDestroyCommandPool(_device, _commandPool, nullptr);

		vkDestroyDevice(_device, nullptr);

		if (enableValidationLayers) {
			DestroyDebugUtilsMessengerEXT(_instance, _debugMessenger, nullptr);
		}

		vkDestroySurfaceKHR(_instance, _window.surface, nullptr);

		vkDestroyInstance(_instance, nullptr);

		glfwDestroyWindow(_glfwWindow);
		glfwTerminate();
	}
}

void VulkanEngine::initVulkan() {
	createInstance();

	VkSurfaceKHR surface;
	glfwCreateWindowSurface(_instance, _glfwWindow, nullptr, &surface);

	int width, height = 0;
	glfwGetFramebufferSize(_glfwWindow, &width, &height);

	_window = {};
	_window.surface = surface;
	_window.width = width;
	_window.height = height;

	// Physical Device

	std::cout << "Physical Device...\n";

	VkPhysicalDevice chosenDevice = pickPhysicalDevice(surface);

	if (chosenDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}

	_physicalDevice = chosenDevice;

	// Device

	std::cout << "Device...\n";

	QueueFamilyIndices indices = findQueueFamilies(_physicalDevice, surface);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	} else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(_physicalDevice, &createInfo, nullptr, &_device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	_graphicsQueueFamily = indices.graphicsFamily.value();
	vkGetDeviceQueue(_device, indices.graphicsFamily.value(), 0, &_graphicsQueue);
	vkGetDeviceQueue(_device, indices.presentFamily.value(), 0, &_presentQueue);

	// Allocator

	std::cout << "Allocator...\n";

	VmaAllocatorCreateInfo allocatorCreateInfo = {};
	allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_0;
	allocatorCreateInfo.physicalDevice = _physicalDevice;
	allocatorCreateInfo.device = _device;
	allocatorCreateInfo.instance = _instance;

	if (vmaCreateAllocator(&allocatorCreateInfo, &_allocator) != VK_SUCCESS) {
		throw std::runtime_error("failed to create allocator!");
	}

	// Swapchain

	std::cout << "Swapchain...\n";
	createSwapChain(&_window);
}

void VulkanEngine::initCommands() {
	VkCommandPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	createInfo.queueFamilyIndex = _graphicsQueueFamily;
	createInfo.pNext = nullptr;

	if (vkCreateCommandPool(_device, &createInfo, nullptr, &_commandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = _commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkAllocateCommandBuffers(_device, &allocInfo, &_frameData[i].commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}
	}
}

void VulkanEngine::initSyncObjects() {
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_frameData[i].presentSemaphore) != VK_SUCCESS ||
				vkCreateSemaphore(_device, &semaphoreInfo, nullptr, &_frameData[i].renderSemaphore) != VK_SUCCESS ||
				vkCreateFence(_device, &fenceInfo, nullptr, &_frameData[i].renderFence) != VK_SUCCESS) {
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}
}

void VulkanEngine::initDescriptors() {
	std::array<VkDescriptorPoolSize, 3> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = 1;

	// ImGui
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[2].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) + 2;

	if (vkCreateDescriptorPool(_device, &poolInfo, nullptr, &_descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}

	// Global set layout

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

	if (vkCreateDescriptorSetLayout(_device, &uboLayoutInfo, nullptr, &_globalSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create UBO set layout!");
	}

	std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, _globalSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = _descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
	allocInfo.pSetLayouts = layouts.data();

	std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> uniformSets{};

	if (vkAllocateDescriptorSets(_device, &allocInfo, uniformSets.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate uniform set!");
	}

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		_frameData[i].uniformBuffer = createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, _frameData[i].uniformAllocInfo);
		_frameData[i].uniformSet = uniformSets[i];

		VkDescriptorBufferInfo uniformBufferInfo{};
		uniformBufferInfo.buffer = _frameData[i].uniformBuffer.buffer;
		uniformBufferInfo.offset = 0;
		uniformBufferInfo.range = sizeof(UniformBufferObject);

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = _frameData[i].uniformSet;
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &uniformBufferInfo;

		vkUpdateDescriptorSets(_device, 1, &descriptorWrite, 0, nullptr);
	}

	// Texture set layout

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

	if (vkCreateDescriptorSetLayout(_device, &textureLayoutInfo, nullptr, &_textureSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture set layout!");
	}
}

void VulkanEngine::initPipelines() {
	VkShaderModule vertShaderModule = loadShaderModule("shaders/vert.spv");
	VkShaderModule fragShaderModule = loadShaderModule("shaders/frag.spv");

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
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
	multisampling.rasterizationSamples = _msaaSamples;

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
	if (vkCreatePipelineLayout(_device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
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
	pipelineInfo.renderPass = _window.renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	VkPipeline pipeline;
	if (vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	vkDestroyShaderModule(_device, fragShaderModule, nullptr);
	vkDestroyShaderModule(_device, vertShaderModule, nullptr);

	createMaterial("main", pipeline, pipelineLayout);
}

void VulkanEngine::initScene() {
	Material *main = getMaterial("main");

	RenderObject cube;

	cube.mesh = getMesh("cube");
	cube.material = main;
	cube.transformMatrix = glm::mat4{ 1.0f };

	_renderObjects.push_back(cube);

	RenderObject sphere;
	sphere.mesh = getMesh("sphere");
	sphere.material = main;
	sphere.transformMatrix = glm::mat4{
		0.5, 0, 0, 0,
		0, 0.5, 0, 0,
		0, 0, 0.5, 0,
		-1.0, -1.0, 0.5, 1
	};

	_renderObjects.push_back(sphere);

	Texture *texture = getTexture("texture");

	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = _descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &_textureSetLayout;

	if (vkAllocateDescriptorSets(_device, &allocInfo, &main->textureSet) != VK_SUCCESS) {
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

	vkUpdateDescriptorSets(_device, 1, &descriptorWrite, 0, nullptr);
}

static void check_vk_result(VkResult err) {
	if (err == 0)
		return;
	fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
	if (err < 0)
		abort();
}

void VulkanEngine::initImGui() {
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	_io = &ImGui::GetIO();
	_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	_io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForVulkan(_glfwWindow, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = _instance;
	init_info.PhysicalDevice = _physicalDevice;
	init_info.Device = _device;
	init_info.QueueFamily = _graphicsQueueFamily;
	init_info.Queue = _graphicsQueue;
	init_info.PipelineCache = nullptr;
	init_info.DescriptorPool = _descriptorPool;
	init_info.Subpass = 0;
	init_info.MinImageCount = 2;
	init_info.ImageCount = 2;
	init_info.MSAASamples = _msaaSamples;
	init_info.Allocator = nullptr;
	init_info.CheckVkResultFn = check_vk_result;
	ImGui_ImplVulkan_Init(&init_info, _window.renderPass);

	VkCommandBuffer commandBuffer = beginSingleTimeCommands();
	ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
	endSingleTimeCommands(commandBuffer);

	ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void VulkanEngine::loadMeshes() {
	Mesh cube;
	cube.load("models/cube.obj");

	Mesh sphere;
	sphere.load("models/sphere.obj");

	uploadMesh(cube);
	uploadMesh(sphere);

	_meshes["cube"] = cube;
	_meshes["sphere"] = sphere;
}

void VulkanEngine::loadTextures() {
	int texWidth, texHeight, texChannels;
	stbi_uc *pixels = stbi_load("textures/raw_plank_wall_diff_1k.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	uint32_t size = texWidth * texHeight * 4;

	std::vector<uint8_t> data(size);
	for (int i = 0; i < size; i++) {
		data[i] = pixels[i];
	}

	Texture texture = createTexture(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, data);

	_textures["texture"] = texture;
}

void VulkanEngine::uploadMesh(Mesh &mesh) {
	// Vertex
	VkDeviceSize vertexBufferSize = sizeof(mesh.vertices[0]) * mesh.vertices.size();

	// allocate buffer
	VmaAllocationInfo vertexAllocInfo;
	mesh.vertexBuffer = createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexAllocInfo);

	// transfer data
	VmaAllocationInfo stagingAllocInfo;
	AllocatedBuffer stagingBuffer = createBuffer(vertexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingAllocInfo);

	memcpy(stagingAllocInfo.pMappedData, mesh.vertices.data(), (size_t)vertexBufferSize);
	vmaFlushAllocation(_allocator, stagingBuffer.allocation, 0, VK_WHOLE_SIZE);
	copyBuffer(stagingBuffer.buffer, mesh.vertexBuffer.buffer, vertexBufferSize);

	vmaDestroyBuffer(_allocator, stagingBuffer.buffer, stagingBuffer.allocation);

	// Index
	VkDeviceSize indexBufferSize = sizeof(mesh.indices[0]) * mesh.indices.size();

	// allocate buffer
	VmaAllocationInfo indexAllocInfo;
	mesh.indexBuffer = createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexAllocInfo);

	// transfer data
	stagingAllocInfo = {};
	stagingBuffer = createBuffer(indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingAllocInfo);

	memcpy(stagingAllocInfo.pMappedData, mesh.indices.data(), (size_t)indexBufferSize);
	vmaFlushAllocation(_allocator, stagingBuffer.allocation, 0, VK_WHOLE_SIZE);
	copyBuffer(stagingBuffer.buffer, mesh.indexBuffer.buffer, indexBufferSize);

	vmaDestroyBuffer(_allocator, stagingBuffer.buffer, stagingBuffer.allocation);
}

Material *VulkanEngine::createMaterial(const std::string &name, VkPipeline pipeline, VkPipelineLayout pipelineLayout) {
	Material mat;

	mat.pipeline = pipeline;
	mat.pipelineLayout = pipelineLayout;

	_materials[name] = mat;
	return &_materials[name];
}

Mesh *VulkanEngine::getMesh(const std::string &name) {
	// search for the object, and return nullptr if not found
	auto it = _meshes.find(name);
	if (it == _meshes.end()) {
		return nullptr;
	} else {
		return &(*it).second;
	}
}

Material *VulkanEngine::getMaterial(const std::string &name) {
	// search for the object, and return nullptr if not found
	auto it = _materials.find(name);
	if (it == _materials.end()) {
		return nullptr;
	} else {
		return &(*it).second;
	}
}

Texture *VulkanEngine::getTexture(const std::string &name) {
	// search for the object, and return nullptr if not found
	auto it = _textures.find(name);
	if (it == _textures.end()) {
		return nullptr;
	} else {
		return &(*it).second;
	}
}

void VulkanEngine::drawObjects(VkCommandBuffer commandBuffer, RenderObject *renderObjects, uint32_t count) {
	Mesh *lastMesh = nullptr;
	Material *lastMaterial = nullptr;

	for (int i = 0; i < count; i++) {
		RenderObject object = renderObjects[i];

		if (object.material != lastMaterial) {
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);

			VkViewport viewport{};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = (float)_window.swapchainExtent.width;
			viewport.height = (float)_window.swapchainExtent.height;
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

			VkRect2D scissor{};
			scissor.offset = { 0, 0 };
			scissor.extent = _window.swapchainExtent;
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

			lastMaterial = object.material;
		}

		MeshPushConstants constants;
		constants.model = object.transformMatrix;

		// upload the mesh to the gpu via pushconstants
		vkCmdPushConstants(commandBuffer, object.material->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

		// only bind the mesh if its a different one from last bind
		if (object.mesh != lastMesh) {
			// bind the mesh vertex buffer with offset 0
			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &object.mesh->vertexBuffer.buffer, &offset);

			// bind descriptors
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 0, 1, &_frameData[_currentFrame].uniformSet, 0, nullptr);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 1, 1, &object.material->textureSet, 0, nullptr);

			// bind index buffer
			vkCmdBindIndexBuffer(commandBuffer, object.mesh->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);

			lastMesh = object.mesh;
		}

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(object.mesh->indices.size()), 1, 0, 0, 0);
	}
}

void VulkanEngine::draw() {
	int i = _currentFrame;
	vkWaitForFences(_device, 1, &_frameData[i].renderFence, VK_TRUE, UINT64_MAX);

	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(_device, _window.swapchain, UINT64_MAX, _frameData[i].presentSemaphore, VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		recreateSwapChain(&_window);
	} else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	updateUniformBuffer(i);

	vkResetFences(_device, 1, &_frameData[i].renderFence);

	vkResetCommandBuffer(_frameData[i].commandBuffer, 0);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(_frameData[i].commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = _window.renderPass;
	renderPassInfo.framebuffer = _window.swapchainImages[imageIndex].framebuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = _window.swapchainExtent;

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(_frameData[i].commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	drawObjects(_frameData[i].commandBuffer, _renderObjects.data(), _renderObjects.size());

	if (_drawDebugMenu) {
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), _frameData[i].commandBuffer);
	}

	vkCmdEndRenderPass(_frameData[i].commandBuffer);

	vkEndCommandBuffer(_frameData[i].commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { _frameData[i].presentSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_frameData[i].commandBuffer;

	VkSemaphore signalSemaphores[] = { _frameData[i].renderSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(_graphicsQueue, 1, &submitInfo, _frameData[i].renderFence) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { _window.swapchain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = &imageIndex;

	result = vkQueuePresentKHR(_presentQueue, &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _window.resized) {
		_window.resized = false;
		recreateSwapChain(&_window);
	} else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	_currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanEngine::updateUniformBuffer(uint32_t currentFrame) {
	glm::vec3 camPos = { 3.f, 3.f, 2.f };

	glm::mat4 view = glm::lookAt(camPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)_window.swapchainExtent.width / (float)_window.swapchainExtent.height, 0.1f, 100.0f);
	projection[1][1] *= -1;

	UniformBufferObject ubo{};
	ubo.view = view;
	ubo.proj = projection;

	memcpy(_frameData[currentFrame].uniformAllocInfo.pMappedData, &ubo, sizeof(ubo));
}

void VulkanEngine::createInstance() {
	if (enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested, but not available!");
	}

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	std::vector<const char *> extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		debugCreateInfo = {};
		debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugCreateInfo.pfnUserCallback = debugCallback;

		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
	} else {
		createInfo.enabledLayerCount = 0;

		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance!");
	}

	if (!enableValidationLayers) {
		return;
	}

	if (CreateDebugUtilsMessengerEXT(_instance, &debugCreateInfo, nullptr, &_debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

bool VulkanEngine::checkValidationLayerSupport() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char *layerName : validationLayers) {
		bool layerFound = false;

		for (const auto &layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

std::vector<const char *> VulkanEngine::getRequiredExtensions() {
	uint32_t glfwExtensionCount = 0;
	const char **glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

VkPhysicalDevice VulkanEngine::pickPhysicalDevice(const VkSurfaceKHR &surface) {
	VkPhysicalDevice chosenDevice = VK_NULL_HANDLE;

	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);

	if (deviceCount == 0) {
		return chosenDevice;
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

	for (const VkPhysicalDevice &device : devices) {
		if (isDeviceSuitable(device, surface)) {
			chosenDevice = device;
			break;
		}
	}

	return chosenDevice;
}

bool VulkanEngine::isDeviceSuitable(VkPhysicalDevice physicalDevice, const VkSurfaceKHR &surface) {
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);

	bool extensionsSupported = checkDeviceExtensionSupport(physicalDevice);

	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice, surface);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

	return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

bool VulkanEngine::checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto &extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

void VulkanEngine::createSwapChain(Window *p_window) {
	SwapChainSupportDetails swapchainSupport = querySwapChainSupport(_physicalDevice, p_window->surface);

	VkSurfaceCapabilitiesKHR capabilities = swapchainSupport.capabilities;

	VkSurfaceFormatKHR surfaceFormat = swapchainSupport.formats[0];
	for (const VkSurfaceFormatKHR &availableFormat : swapchainSupport.formats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			surfaceFormat = availableFormat;
		}
	}

	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (const VkPresentModeKHR &availablePresentMode : swapchainSupport.presentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			presentMode = availablePresentMode;
		}
	}

	VkExtent2D extent;
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		extent = capabilities.currentExtent;
	} else {
		VkExtent2D actualExtent = {
			static_cast<uint32_t>(p_window->width),
			static_cast<uint32_t>(p_window->height)
		};

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		extent = actualExtent;
	}

	uint32_t imageCount = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
		imageCount = capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = p_window->surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = findQueueFamilies(_physicalDevice, p_window->surface);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	if (vkCreateSwapchainKHR(_device, &createInfo, nullptr, &p_window->swapchain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(_device, p_window->swapchain, &imageCount, nullptr);

	VkImage *swapchainImages = (VkImage *)malloc(imageCount * sizeof(VkImage));
	vkGetSwapchainImagesKHR(_device, p_window->swapchain, &imageCount, swapchainImages);

	p_window->swapchainImages.resize(imageCount);

	for (uint32_t i = 0; i < imageCount; i++) {
		p_window->swapchainImages[i].image = swapchainImages[i];
		p_window->swapchainImages[i].view = createImageView(swapchainImages[i], surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}

	free(swapchainImages);

	// Resources

	_colorImage = createImageResource(extent.width, extent.height, _msaaSamples, surfaceFormat.format, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	_colorImageView = createImageView(_colorImage.image, surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT, 1);

	_depthImage = createImageResource(extent.width, extent.height, _msaaSamples, _depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
	_depthImageView = createImageView(_depthImage.image, _depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

	// Framebuffers

	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = surfaceFormat.format;
	colorAttachment.samples = _msaaSamples;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = _depthFormat;
	depthAttachment.samples = _msaaSamples;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription colorAttachmentResolve{};
	colorAttachmentResolve.format = surfaceFormat.format;
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentResolveRef{};
	colorAttachmentResolveRef.attachment = 2;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	subpass.pResolveAttachments = &colorAttachmentResolveRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(_device, &renderPassInfo, nullptr, &p_window->renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}

	for (size_t i = 0; i < imageCount; i++) {
		std::array<VkImageView, 3> attachments = {
			_colorImageView,
			_depthImageView,
			p_window->swapchainImages[i].view
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = p_window->renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = extent.width;
		framebufferInfo.height = extent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(_device, &framebufferInfo, nullptr, &p_window->swapchainImages[i].framebuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}

	p_window->swapchainExtent = extent;
	_swapchainImageCount = imageCount;

	_colorFormat = surfaceFormat.format;
	_colorSpace = surfaceFormat.colorSpace;
}

void VulkanEngine::cleanupSwapChain(Window *p_window) {
	vkDestroyImageView(_device, _colorImageView, nullptr);
	vmaDestroyImage(_allocator, _colorImage.image, _colorImage.allocation);

	vkDestroyImageView(_device, _depthImageView, nullptr);
	vmaDestroyImage(_allocator, _depthImage.image, _depthImage.allocation);

	for (uint32_t i = 0; i < p_window->swapchainImages.size(); i++) {
		vkDestroyFramebuffer(_device, p_window->swapchainImages[i].framebuffer, nullptr);
		vkDestroyImageView(_device, p_window->swapchainImages[i].view, nullptr);
	}

	p_window->swapchainImages.clear();
	vkDestroySwapchainKHR(_device, p_window->swapchain, nullptr);
}

void VulkanEngine::recreateSwapChain(Window *p_window) {
	int width = p_window->width;
	int height = p_window->height;

	while (width == 0 || height == 0) {
		width = p_window->width;
		height = p_window->height;
	}

	vkDeviceWaitIdle(_device);

	cleanupSwapChain(p_window);
	createSwapChain(p_window);
}

VkShaderModule VulkanEngine::loadShaderModule(const std::string &path) {
	std::ifstream file(path, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = buffer.size();
	createInfo.pCode = reinterpret_cast<const uint32_t *>(buffer.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}

	return shaderModule;
}

QueueFamilyIndices VulkanEngine::findQueueFamilies(VkPhysicalDevice physicalDevice, const VkSurfaceKHR &surface) {
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto &queueFamily : queueFamilies) {
		if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			indices.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

		if (presentSupport) {
			indices.presentFamily = i;
		}

		if (indices.isComplete()) {
			break;
		}

		i++;
	}

	return indices;
}

SwapChainSupportDetails VulkanEngine::querySwapChainSupport(VkPhysicalDevice physicalDevice, const VkSurfaceKHR &surface) {
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) {
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}

AllocatedBuffer VulkanEngine::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationInfo &allocInfo) {
	VkBufferCreateInfo bufCreateInfo{};
	bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufCreateInfo.size = size;
	bufCreateInfo.usage = usage;

	VmaAllocationCreateInfo allocCreateInfo{};
	allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
	allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

	AllocatedBuffer buffer;
	if (vmaCreateBuffer(_allocator, &bufCreateInfo, &allocCreateInfo, &buffer.buffer, &buffer.allocation, &allocInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer!");
	}

	return buffer;
}

void VulkanEngine::copyBuffer(VkBuffer &srcBuffer, VkBuffer &dstBuffer, VkDeviceSize size) {
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkBufferCopy bufCopy{};
	bufCopy.srcOffset = 0;
	bufCopy.dstOffset = 0;
	bufCopy.size = size;

	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &bufCopy);

	endSingleTimeCommands(commandBuffer);
}

Texture VulkanEngine::createTexture(uint32_t width, uint32_t height, VkFormat format, const std::vector<uint8_t> &data) {
	uint8_t mipmaps = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

	AllocatedImage textureImage = createImage(width, height, mipmaps, VK_SAMPLE_COUNT_1_BIT, format, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

	// Create staging buffer
	VkDeviceSize size = width * height * 4;

	VmaAllocationInfo stagingAllocInfo;
	AllocatedBuffer stagingBuffer = createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingAllocInfo);
	memcpy(stagingAllocInfo.pMappedData, data.data(), (size_t)size);
	vmaFlushAllocation(_allocator, stagingBuffer.allocation, 0, VK_WHOLE_SIZE);

	// Transition image layout
	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

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

	endSingleTimeCommands(commandBuffer);

	// Copy buffer to image
	commandBuffer = beginSingleTimeCommands();

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

	endSingleTimeCommands(commandBuffer);

	// Generate mipmaps
	generateMipmaps(textureImage.image, format, width, height, mipmaps);

	// Destroy staging buffer
	vmaDestroyBuffer(_allocator, stagingBuffer.buffer, stagingBuffer.allocation);

	// Image view
	VkImageView textureImageView = createImageView(textureImage.image, format, VK_IMAGE_ASPECT_COLOR_BIT, mipmaps);

	// Sampler
	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(_physicalDevice, &properties);

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
	samplerInfo.minLod = 0.0f; // Optional
	samplerInfo.maxLod = static_cast<float>(mipmaps);
	samplerInfo.mipLodBias = 0.0f; // Optional

	VkSampler textureSampler;
	if (vkCreateSampler(_device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture sampler!");
	}

	return Texture{ textureImage, textureImageView, textureSampler };
}

void VulkanEngine::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
	// Check if image format supports linear blitting
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(_physicalDevice, imageFormat, &formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		throw std::runtime_error("texture image format does not support linear blitting!");
	}

	VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;

	for (uint32_t i = 1; i < mipLevels; i++) {
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

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

	endSingleTimeCommands(commandBuffer);
}

AllocatedImage VulkanEngine::createImageResource(uint32_t width, uint32_t height, VkSampleCountFlagBits numSamples, VkFormat format, VkImageUsageFlags usage) {
	return createImage(width, height, 1, numSamples, format, usage);
}

AllocatedImage VulkanEngine::createImage(uint32_t width, uint32_t height, uint32_t mipmaps, VkSampleCountFlagBits numSamples, VkFormat format, VkImageUsageFlags usage) {
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
	imageInfo.samples = numSamples;
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

VkImageView VulkanEngine::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipmaps) {
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
	if (vkCreateImageView(_device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}

VkCommandBuffer VulkanEngine::beginSingleTimeCommands() {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = _commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(_device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void VulkanEngine::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(_graphicsQueue);

	vkFreeCommandBuffers(_device, _commandPool, 1, &commandBuffer);
}

void VulkanEngine::windowResizedCallback(GLFWwindow *window, int width, int height) {
	VulkanEngine *app = reinterpret_cast<VulkanEngine *>(glfwGetWindowUserPointer(window));

	glfwGetFramebufferSize(window, &width, &height);
	app->resizeWindow(width, height);
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanEngine::debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
		void *pUserData) {
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}
