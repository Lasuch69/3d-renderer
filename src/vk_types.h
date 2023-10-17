#ifndef VK_TYPES_H
#define VK_TYPES_H

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

struct AllocatedBuffer {
	VmaAllocation allocation;
	VkBuffer buffer;
};

struct AllocatedImage {
	VmaAllocation allocation;
	VkImage image;
};

#endif // !VK_TYPES_H
