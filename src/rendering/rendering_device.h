#ifndef RENDERING_DEVICE_H
#define RENDERING_DEVICE_H

#include <GLFW/glfw3.h>
#include <cstdint>
#include <imgui.h>

#include <string>
#include <unordered_map>

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

class RenderingDevice {
	VulkanContext *_context;
	uint32_t _currentFrame = 0;

	Camera *_camera;

	VmaAllocator _allocator;

	VkCommandBuffer _commandBuffers[MAX_FRAMES_IN_FLIGHT];

	VkDescriptorPool _descriptorPool;
	VkDescriptorSetLayout _globalSetLayout;
	VkDescriptorSetLayout _textureSetLayout;

	AllocatedBuffer _uniformBuffers[MAX_FRAMES_IN_FLIGHT];
	VmaAllocationInfo _uniformAllocInfos[MAX_FRAMES_IN_FLIGHT];
	VkDescriptorSet _uniformSets[MAX_FRAMES_IN_FLIGHT];

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

	void _uploadMesh(Mesh &p_mesh);
	Material *_createMaterial(const std::string &p_name, VkPipeline p_pipeline, VkPipelineLayout p_pipelineLayout);

	Mesh *_getMesh(const std::string &p_name);
	Material *_getMaterial(const std::string &p_name);
	Texture *_getTexture(const std::string &p_name);

	void _drawObjects(VkCommandBuffer p_commandBuffer, RenderObject *p_renderObjects, uint32_t p_count);

	void _updateUniformBuffer(uint32_t p_currentFrame);

	AllocatedBuffer _createBuffer(VkDeviceSize p_size, VkBufferUsageFlags p_usage, VmaAllocationInfo &p_allocInfo);
	void _copyBuffer(VkBuffer &p_srcBuffer, VkBuffer &p_dstBuffer, VkDeviceSize p_size);

	Texture _createTexture(uint32_t p_width, uint32_t p_height, VkFormat p_format, const std::vector<uint8_t> &p_data);
	void _generateMipmaps(VkImage p_image, VkFormat p_imageFormat, int32_t p_texWidth, int32_t p_texHeight, uint32_t p_mipLevels);

	AllocatedImage _createImage(uint32_t p_width, uint32_t p_height, uint32_t p_mipmaps, VkSampleCountFlagBits p_numSamples, VkFormat p_format, VkImageUsageFlags p_usage);
	VkImageView _createImageView(VkImage p_image, VkFormat p_format, VkImageAspectFlags p_aspectFlags, uint32_t p_mipmaps);

	VkCommandBuffer _beginSingleTimeCommands();
	void _endSingleTimeCommands(VkCommandBuffer p_commandBuffer);

public:
	void setCamera(Camera *p_camera);

	void windowCreate(GLFWwindow *p_window, uint32_t p_width, uint32_t p_height);
	void windowResize(uint32_t p_width, uint32_t p_height);

	void initImGui(GLFWwindow *p_window);

	void draw();
	void waitIdle();

	RenderingDevice(bool p_validationLayers);
	~RenderingDevice();
};

#endif // !RENDERING_DEVICE_H
