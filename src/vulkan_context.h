#ifndef VULKAN_CONTEXT_H
#define VULKAN_CONTEXT_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <vulkan/vulkan.h>

#include <vma/vk_mem_alloc.h>

#include <optional>
#include <string>
#include <vector>

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char *> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char *> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct UniformBufferObject {
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}

	bool operator==(const Vertex &other) const {
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
};

namespace std {
template <>
struct hash<Vertex> {
	size_t operator()(Vertex const &vertex) const {
		return ((hash<glm::vec3>()(vertex.pos) ^
						(hash<glm::vec3>()(vertex.color) << 1)) >>
					   1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
	}
};
} // namespace std

class VulkanContext {
private:
	VmaAllocator allocator;

	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;

	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;

	VkQueue graphicsQueue;
	VkQueue presentQueue;

	typedef struct {
		VkImage image;
		VkImageView view;
		VkFramebuffer framebuffer;
	} SwapChainImageResource;

	struct Window {
		std::vector<SwapChainImageResource> swapchainImages;
		VkSwapchainKHR swapchain = VK_NULL_HANDLE;
		VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
		VkSurfaceKHR surface = VK_NULL_HANDLE;
		VkRenderPass renderPass = VK_NULL_HANDLE;
		int width = 0;
		int height = 0;
		VkExtent2D swapchainExtent;
	};

	Window window;
	VkFormat format;
	VkColorSpaceKHR colorSpace;
	uint32_t swapchainImageCount = 0;

	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_4_BIT;

	VkImage colorImage;
	VmaAllocation colorImageAlloc;
	VkImageView colorImageView;

	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	VkCommandPool commandPool;

	VkImage depthImage;
	VmaAllocation depthImageAlloc;
	VkImageView depthImageView;

	uint32_t mipLevels;
	VkImage textureImage;
	VmaAllocation textureImageAlloc;
	VkImageView textureImageView;
	VkSampler textureSampler;

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	VkBuffer vertexBuffer;
	VmaAllocation vertexAlloc;
	VkBuffer indexBuffer;
	VmaAllocation indexAlloc;

	std::vector<VkBuffer> uniformBuffers;
	std::vector<VmaAllocation> uniformAllocs;
	std::vector<VmaAllocationInfo> uniformAllocInfos;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorSet> descriptorSets;

	std::vector<VkCommandBuffer> commandBuffers;

	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	uint32_t currentFrame = 0;

	bool framebufferResized = false;

	void createInstance(std::vector<const char *> p_extensions);

	void pickPhysicalDevice();
	void createLogicalDevice();

	void createAllocator();

	void createSwapChain(Window *p_window);
	void cleanupSwapChain(Window *p_window);
	void recreateSwapChain(Window *p_window);

	void createDescriptorSetLayout();
	void createGraphicsPipeline();
	void createCommandPool();
	void createTextureImage();
	void loadModel();
	void createVertexBuffer();
	void createIndexBuffer();
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();
	void createCommandBuffers();
	void createSyncObjects();

	bool checkValidationLayerSupport();

	bool isDeviceSuitable(VkPhysicalDevice device);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device);

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, int width, int height);

	VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	VkFormat findDepthFormat();

	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer &buf, VmaAllocation &alloc, VmaAllocationInfo &allocInfo);
	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
	void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image, VmaAllocation &imageAlloc);
	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

	void updateUniformBuffer(uint32_t currentImage);

	VkShaderModule createShaderModule(const std::vector<char> &code);
	static std::vector<char> readFile(const std::string &filename);

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
			void *pUserData);

public:
	void windowCreate(int p_width, int p_height, VkSurfaceKHR &p_surface);
	void windowResize(int p_width, int p_height);

	void init();
	void drawFrame();

	VkInstance getInstance() { return instance; }
	VkDevice getDevice() { return device; }

	VulkanContext() {}
	VulkanContext(std::vector<const char *> p_extensions);

	~VulkanContext();
};

#endif // !VULKAN_CONTEXT_H
