#define VMA_IMPLEMENTATION
#include "VulkanMemoryAllocator/vk_mem_alloc.h"
#include "vk_mem_alloc_wrapper.h"

extern "C" {

VkResult
init_vulkan_memory_allocator(struct vk_program *program)
{
	struct vk_device *dev = &program->device;

	VmaAllocatorCreateInfo allocator_info = {
		.physicalDevice = dev->physical_device,
		.device = dev->logical_device,
		.instance = program->instance,
		.vulkanApiVersion = VK_API_VERSION_1_0
	};

	return vmaCreateAllocator(&allocator_info, &dev->mem_allocator);
}

}
