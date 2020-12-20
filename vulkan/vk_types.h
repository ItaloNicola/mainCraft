#ifndef VK_TYPES_H
#define VK_TYPES_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>


enum family_indices { graphics = 0, transfer, compute, protectedBit, sparseBindingBit, present, queues_count };

struct vk_queues {
	uint32_t family_indices[queues_count];
	uint32_t queue_count[queues_count];
	VkQueue handles[queues_count];
	/* Per family in use queue count*/
	uint32_t handles_count[queues_count];
};

struct vk_device {
	VkPhysicalDevice physical_device;
	VkDevice logical_device;
	struct vk_queues queues;
};

struct vk_program {
	GLFWwindow *window;
	VkSurfaceKHR surface;
	VkApplicationInfo app_info;
	VkInstance instance;
	struct vk_device device;
};

#endif //VK_TYPES_H
