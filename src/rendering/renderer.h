#ifndef RENDERER_H
#define RENDERER_H

#include <cstdint>

#include <imgui.h>

#include "camera.h"
#include "types.h"
#include "vertex.h"
#include "vulkan_context.h"

struct UniformBufferObject {
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct MeshPushConstants {
	glm::vec4 data;
	glm::mat4 model;
};

struct Mesh {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	AllocatedBuffer vertexBuffer;
	AllocatedBuffer indexBuffer;

	bool initialized = false;
};

struct Texture {
	AllocatedImage image;
	VkImageView view;
	VkSampler sampler;
};

struct Material {
	VkDescriptorSet textureSet;

	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
};

class Renderer {
	VulkanContext *_context;
	uint32_t _currentFrame = 0;

	Camera *_camera;

	VmaAllocator _allocator;

	VkCommandBuffer _commandBuffers[MAX_FRAMES_IN_FLIGHT];

	VkDescriptorPool _descriptorPool;

	VkDescriptorSetLayout _uniformSetLayout;
	VkDescriptorSetLayout _subpassSetLayout;
	VkDescriptorSetLayout _textureSetLayout;

	AllocatedBuffer _uniformBuffers[MAX_FRAMES_IN_FLIGHT];
	VmaAllocationInfo _uniformAllocInfos[MAX_FRAMES_IN_FLIGHT];
	VkDescriptorSet _uniformSets[MAX_FRAMES_IN_FLIGHT];

	Material _material;

	VkDescriptorSet _subpassSet;
	Material _tonemapping;

	typedef struct {
		VkCommandBuffer commandBuffer;
		uint32_t imageIndex;
	} RenderHandle;

	RenderHandle *_renderHandle = nullptr;

	void _initAllocator();
	void _initCommands();
	void _initDescriptors();
	void _initPipelines();

	void _uploadMesh(Mesh *pMesh);

	void _updateUniformBuffer(uint32_t currentFrame);

	void _writeImageSet(VkDescriptorSet dstSet, VkImageView imageView, VkSampler sampler, VkDescriptorType descriptorType);

	AllocatedBuffer _createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationInfo &allocInfo);
	void _copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	Texture _createTexture(uint32_t width, uint32_t height, VkFormat format, const std::vector<uint8_t> &data);
	bool _generateMipmaps(int32_t width, int32_t height, VkFormat format, uint32_t mipmaps, VkImage image);

	AllocatedImage _createImage(uint32_t width, uint32_t height, VkFormat format, uint32_t mipmaps, VkImageUsageFlags usage);
	VkImageView _createImageView(VkImage image, VkFormat format, uint32_t mipmaps, VkImageAspectFlags aspectFlags);

	VkPipelineLayout _createPipelineLayout(VkDescriptorSetLayout *pSetLayouts, uint32_t layoutCount, VkPushConstantRange *pPushConstants, uint32_t constantCount);
	VkPipeline _createPipeline(VkPipelineLayout layout, VkShaderModule vertex, VkShaderModule fragment, uint32_t subpass);

	VkCommandBuffer _beginSingleTimeCommands();
	void _endSingleTimeCommands(VkCommandBuffer commandBuffer);

public:
	VkInstance getInstance();
	Camera *getCamera();

	void windowInit(VkSurfaceKHR surface, uint32_t width, uint32_t height);
	void windowResize(uint32_t width, uint32_t height);

	void initImGui();

	Mesh meshCreate(std::vector<Vertex> vertices, std::vector<uint32_t> indices);
	Texture textureCreate(uint32_t width, uint32_t height, VkFormat format, const std::vector<uint8_t> &data);

	void drawBegin();
	void drawMesh(Mesh *pMesh, const glm::mat4 &transform);
	void drawEnd();

	void waitIdle();

	Renderer(std::vector<const char *> extensions, bool useValidation);
	~Renderer();
};

#endif // !RENDERER_H
