#ifndef VULKAN_CONTEXT_H
#define VULKAN_CONTEXT_H

#include <cstdint>
#include <optional>
#include <vector>

#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>

const int MAX_FRAMES_IN_FLIGHT = 2;

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

struct SyncObject {
	VkSemaphore presentSemaphore;
	VkSemaphore renderSemaphore;
	VkFence renderFence;
};

class VulkanContext {
private:
	bool _validationLayers = false;

	VkInstance _instance;

	VkDebugUtilsMessengerEXT _debugMessenger;

	VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
	VkDevice _device;

	VkQueue _graphicsQueue;
	VkQueue _presentQueue;

	uint32_t _graphicsQueueFamily;

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

	VkFormat _colorFormat;
	VkFormat _depthFormat;

	VkColorSpaceKHR _colorSpace;
	uint32_t _swapchainImageCount = 0;

	VkImage _depthImage;
	VkDeviceMemory _depthImageMemory;
	VkImageView _depthImageView;

	SyncObject _syncObjects[MAX_FRAMES_IN_FLIGHT];

	// instance
	void _createInstance();
	bool _checkValidationLayerSupport();
	std::vector<const char *> _getRequiredExtensions();

	// physical device
	VkPhysicalDevice _pickPhysicalDevice(VkSurfaceKHR p_surface);
	bool _isDeviceSuitable(VkPhysicalDevice p_physicalDevice, VkSurfaceKHR p_surface);
	bool _checkDeviceExtensionSupport(VkPhysicalDevice p_physicalDevice);

	QueueFamilyIndices _findQueueFamilies(VkPhysicalDevice p_physicalDevice, VkSurfaceKHR p_surface);
	SwapChainSupportDetails _querySwapChainSupport(VkPhysicalDevice p_physicalDevice, VkSurfaceKHR p_surface);

	// device
	VkDevice _createDevice(VkPhysicalDevice p_physicalDevice, VkSurfaceKHR p_surface);

	// swapchain
	void _createSwapChain(Window *p_window);
	void _cleanupSwapChain(Window *p_window);
	void _recreateSwapChain(Window *p_window);

	VkImage _createImage(uint32_t p_width, uint32_t p_height, VkSampleCountFlagBits p_numSamples, VkFormat p_format, VkImageUsageFlags p_usage, VkDeviceMemory &p_memory);
	VkImageView _createImageView(VkImage p_image, VkFormat p_format, VkImageAspectFlags p_aspectFlags);

	uint32_t _findMemoryType(uint32_t p_typeFilter, VkMemoryPropertyFlags p_properties);
	VkFormat _findDepthFormat();
	VkFormat _findSupportedFormat(const std::vector<VkFormat> &p_candidates, VkImageTiling p_tiling, VkFormatFeatureFlags p_features);

	void _createCommandPool();
	void _createSyncObjects();

public:
	void windowCreate(GLFWwindow *p_window, uint32_t p_width, uint32_t p_height);
	void windowResize(uint32_t p_width, uint32_t p_height);

	void recreateSwapchain();

	void submit(uint32_t p_currentFrame, uint32_t p_imageIndex, VkCommandBuffer p_commandBuffer);

	VkInstance getInstance() { return _instance; }
	VkPhysicalDevice getPhysicalDevice() { return _physicalDevice; }
	VkDevice getDevice() { return _device; }

	VkQueue getGraphicsQueue() { return _graphicsQueue; }
	VkQueue getPresentQueue() { return _presentQueue; }

	uint32_t getGraphicsQueueFamily() { return _graphicsQueueFamily; }

	VkRenderPass getRenderPass() { return _window.renderPass; }
	VkSwapchainKHR getSwapchain() { return _window.swapchain; }
	VkExtent2D getSwapchainExtent() { return _window.swapchainExtent; }
	VkFramebuffer getFramebuffer(uint32_t p_currentFrame) { return _window.swapchainImages[p_currentFrame].framebuffer; }

	VkCommandPool getCommandPool() { return _commandPool; }

	SyncObject getSyncObject(uint32_t p_index) { return _syncObjects[p_index]; }

	VulkanContext(bool p_validationLayers);
	~VulkanContext();
};

#endif // !VULKAN_CONTEXT_H
