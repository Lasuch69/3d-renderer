#ifndef RENDERER_H
#define RENDERER_H

#include <cstdint>
#include <string>
#include <unordered_map>

#include <GLFW/glfw3.h>
#include <imgui.h>

#include "../camera.h"
#include "mesh.h"
#include "vulkan_context.h"

struct UniformBufferObject {
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct MeshPushConstants {
	glm::vec4 data;
	glm::mat4 model;
};

struct Texture {
	AllocatedImage image;
	VkImageView view;
	VkSampler sampler;
};

struct Material {
	VkDescriptorSet textureSet;
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
};

struct RenderObject {
	Mesh *mesh;
	Material *material;

	glm::mat4 transformMatrix;
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

	VkDescriptorSet _subpassSet;
	Material _tonemapping;

	// Objects
	std::vector<RenderObject> _renderObjects;

	std::unordered_map<std::string, Mesh> _meshes;
	std::unordered_map<std::string, Material> _materials;
	std::unordered_map<std::string, Texture> _textures;

	void _initAllocator();
	void _initCommands();
	void _initDescriptors();
	void _initPipelines();
	void _initScene();

	void _loadMeshes();
	void _loadTextures();

	void _uploadMesh(Mesh &mesh);
	Material *_createMaterial(const std::string &name, VkPipeline pipeline, VkPipelineLayout pipelineLayout);

	Mesh *_getMesh(const std::string &name);
	Material *_getMaterial(const std::string &name);
	Texture *_getTexture(const std::string &name);

	void _drawObjects(VkCommandBuffer commandBuffer, RenderObject *pRenderObjects, uint32_t count);

	void _updateUniformBuffer(uint32_t currentFrame);
	void _updateSubpassSet();

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
	void setCamera(Camera *pCamera);

	void windowCreate(GLFWwindow *pWindow, uint32_t width, uint32_t height);
	void windowResize(uint32_t width, uint32_t height);

	void initImGui(GLFWwindow *pWindow);

	void draw();
	void waitIdle();

	Renderer(bool useValidation);
	~Renderer();
};

#endif // !RENDERER_H
