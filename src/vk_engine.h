#ifndef VULKAN_CONTEXT_H
#define VULKAN_CONTEXT_H

#include "vk_mesh.h"

#include <GLFW/glfw3.h>

#include <optional>
#include <string>
#include <unordered_map>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const std::vector<const char *> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char *> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

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

struct MeshPushConstants {
	glm::vec4 data;
	glm::mat4 render_matrix;
};

struct Material {
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
};

struct RenderObject {
	Mesh *mesh;
	Material *material;

	glm::mat4 transformMatrix;
};

class VulkanEngine {
	// Objects
	std::vector<RenderObject> _renderObjects;

	std::unordered_map<std::string, Material> _materials;
	std::unordered_map<std::string, Mesh> _meshes;

	GLFWwindow *_glfwWindow = nullptr;

	VkInstance _instance;
	VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
	VkDevice _device;

	VmaAllocator _allocator;

	VkDebugUtilsMessengerEXT _debugMessenger;

	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;

	VkQueue _presentQueue;

	bool _initialized = false;

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
		VkExtent2D swapchainExtent;

		int width = 0;
		int height = 0;
		bool resized = false;
	};

	Window _window;

	VkCommandPool _commandPool;
	std::vector<VkCommandBuffer> _commandBuffers;

	VkSampleCountFlagBits _msaaSamples = VK_SAMPLE_COUNT_8_BIT;

	VkFormat _colorFormat;
	VkFormat _depthFormat = VK_FORMAT_D32_SFLOAT;

	VkColorSpaceKHR _colorSpace;
	uint32_t _swapchainImageCount = 0;

	AllocatedImage _colorAllocatedImage;
	VkImageView _colorImageView;

	AllocatedImage _depthAllocatedImage;
	VkImageView _depthImageView;

	std::vector<VkSemaphore> _presentSemaphores;
	std::vector<VkSemaphore> _renderSemaphores;
	std::vector<VkFence> _renderFences;

	uint32_t _currentFrame = 0;

	void initVulkan();
	void initCommands();
	void initSyncObjects();
	void initPipelines();
	void initScene();

	void loadMeshes();
	void uploadMesh(Mesh &mesh);

	Material *createMaterial(const std::string &name, VkPipeline pipeline, VkPipelineLayout pipelineLayout);

	Mesh *getMesh(const std::string &name);
	Material *getMaterial(const std::string &name);

	void drawObjects(VkCommandBuffer commandBuffer, RenderObject *renderObjects, uint32_t count);
	void draw();

	// Instance
	void createInstance();
	bool checkValidationLayerSupport();
	std::vector<const char *> getRequiredExtensions();

	// Physical device
	VkPhysicalDevice pickPhysicalDevice(const VkSurfaceKHR &surface);
	bool isDeviceSuitable(VkPhysicalDevice physicalDevice, const VkSurfaceKHR &surface);
	bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice);

	// Swapchain
	void createSwapChain(Window *p_window);
	void cleanupSwapChain(Window *p_window);
	void recreateSwapChain(Window *p_window);

	VkShaderModule loadShaderModule(const std::string &filename);

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice, const VkSurfaceKHR &surface);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice, const VkSurfaceKHR &surface);

	AllocatedBuffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaAllocationInfo &allocationInfo);
	void copyBuffer(VkBuffer &srcBuffer, VkBuffer &dstBuffer, VkDeviceSize size);

	AllocatedImage createImageResource(uint32_t width, uint32_t height, VkSampleCountFlagBits numSamples, VkFormat format, VkImageUsageFlags usage);

	AllocatedImage createImage(uint32_t width, uint32_t height, VkSampleCountFlagBits numSamples, VkFormat format, VkImageUsageFlags usage, VkMemoryPropertyFlags properties);
	VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

	VkCommandBuffer beginSingleTimeCommands();
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	static void windowResizedCallback(GLFWwindow *window, int width, int height);
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
			void *pUserData);

public:
	void resizeWindow(int width, int height);

	void init();
	void run();
	void cleanup();
};

#endif // !VULKAN_CONTEXT_H
