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
	bool _useValidation;

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

	VkImage _depthImage;
	VkDeviceMemory _depthImageMemory;
	VkImageView _depthImageView;

	SyncObject _syncObjects[MAX_FRAMES_IN_FLIGHT];

	// instance
	void _createInstance(bool useValidation = false);
	bool _checkValidationLayerSupport();
	std::vector<const char *> _getRequiredExtensions(bool useValidation);

	// physical device
	VkPhysicalDevice _pickPhysicalDevice(VkSurfaceKHR surface);
	bool _isDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
	bool _checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice);

	QueueFamilyIndices _findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
	SwapChainSupportDetails _querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

	// device
	VkDevice _createDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

	// swapchain
	void _createSwapChain(Window *pWindow);
	void _cleanupSwapChain(Window *pWindow);
	void _recreateSwapChain(Window *pWindow);

	VkImage _createImage(uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkDeviceMemory *pMemory);
	VkImageView _createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

	uint32_t _findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkFormat _findDepthFormat();
	VkFormat _findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	void _createCommandPool();
	void _createSyncObjects();

public:
	void windowCreate(GLFWwindow *pWindow, uint32_t width, uint32_t height);
	void windowResize(uint32_t width, uint32_t height);

	void recreateSwapchain();

	void submit(uint32_t currentFrame, uint32_t imageIndex, VkCommandBuffer commandBuffer);

	VkInstance getInstance() { return _instance; }
	VkPhysicalDevice getPhysicalDevice() { return _physicalDevice; }
	VkDevice getDevice() { return _device; }

	VkQueue getGraphicsQueue() { return _graphicsQueue; }
	VkQueue getPresentQueue() { return _presentQueue; }

	uint32_t getGraphicsQueueFamily() { return _graphicsQueueFamily; }

	VkRenderPass getRenderPass() { return _window.renderPass; }
	VkSwapchainKHR getSwapchain() { return _window.swapchain; }
	VkExtent2D getSwapchainExtent() { return _window.swapchainExtent; }
	VkFramebuffer getFramebuffer(uint32_t currentFrame) { return _window.swapchainImages[currentFrame].framebuffer; }

	VkCommandPool getCommandPool() { return _commandPool; }

	SyncObject getSyncObject(uint32_t index) { return _syncObjects[index]; }

	VulkanContext(bool useValidation);
	~VulkanContext();
};

#endif // !VULKAN_CONTEXT_H
