#ifndef TYPES_H
#define TYPES_H

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

#endif // !TYPES_H
