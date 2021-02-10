#ifndef VK_TYPES_H
#define VK_TYPES_H

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <cglm/cglm.h>
#include <stdbool.h>

#include "VulkanMemoryAllocator/vk_mem_alloc.h"
#include "vk_constants.h"
#include "types.h"


enum family_indices { graphics = 0, transfer, compute, protectedBit, sparseBindingBit, present, queues_count };

struct view_projection {
	VkBuffer *buffers;
	uint32_t buffer_count;
	VmaAllocation *buffers_memory;
	uint32_t buffer_memory_count;
	mat4 proj;
	mat4 view;
};

struct vk_vertex_object {
	/* Vertex resources */
	VkBuffer vertex_buffer;
	VmaAllocation vertex_buffer_memory;
	struct vertex *vertices;
	uint64_t vertices_count;
	/* Index resources */
	uint16_t *indices;
	uint64_t indices_count;
	VkBuffer index_buffer;
	VmaAllocation index_buffer_memory;
	/* Textures resources */
	VkImage *texture_images;
	VmaAllocation *texture_images_memory;
	VkImageView *texture_images_view;
	uint32_t texture_count;
	/* Model positions */
	VkBuffer *position_buffer;
	VmaAllocation *position_buffer_memory;
	VkBuffer staging_position_buffer;
	VmaAllocation staging_position_buffer_memory;
	uint32_t position_count;
	uint32_t position_buffer_count;
};

struct vk_game_objects {
	struct vk_vertex_object cube;
	struct view_projection camera;
};

struct vk_device_properties {
	VkPhysicalDeviceFeatures supported_features;
	VkPhysicalDeviceProperties device_properties;
};

struct vk_draw_sync {
	VkSemaphore image_available_semaphore[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore render_finished_semaphore[MAX_FRAMES_IN_FLIGHT];
	VkFence in_flight_fences[MAX_FRAMES_IN_FLIGHT];
	VkFence *images_in_flight;
};

struct vk_cmd_submission {
	VkCommandPool command_pools[queues_count];
	uint64_t pools_allocated;
	VkCommandBuffer *cmd_buffers[queues_count];
	uint32_t cmd_buffers_count[queues_count];
	uint32_t family_indices[queues_count];
	uint32_t queue_count[queues_count];
	VkQueue queue_handles[queues_count];
	/* Per family in use queue count*/
	uint32_t handles_count[queues_count];
	VkDescriptorPool descriptor_pool;
	VkDescriptorSet *descriptor_sets;
	uint32_t descriptors_count;
};

struct vk_render {
	VkRenderPass render_pass;
	VkPipeline graphics_pipeline;
	VkPipelineLayout pipeline_layout;;
	VkFramebuffer *swapChain_framebuffers;
	uint32_t framebuffer_count;
	VkDescriptorSetLayout descriptor_set_layout;
	VkSampler texture_sampler;
	VkImage depth_image;
	VmaAllocation depth_image_memory;
	VkImageView depth_image_view;
	VkFormat depth_format;
};

struct surface_support {
	VkSurfaceCapabilitiesKHR capabilities;
	VkSurfaceFormatKHR *formats;
	uint32_t formats_count;
	VkPresentModeKHR *present_modes;
	uint32_t present_modes_count;
};

struct swapchain_info {
	VkSurfaceFormatKHR surface_format;
	VkPresentModeKHR present_mode;
	VkExtent2D extent;
};

struct vk_swapchain {
	VkSwapchainKHR handle;
	VkImageView *image_views;
	VkImage* images;
	uint32_t images_count;
	struct swapchain_info state;
	struct surface_support support;
	bool framebuffer_resized;
};

struct vk_device {
	VkPhysicalDevice physical_device;
	VkDevice logical_device;
	VmaAllocator mem_allocator;
	struct vk_cmd_submission cmd_submission;
	struct vk_swapchain swapchain;
	struct vk_render render;
	struct vk_draw_sync draw_sync;
	struct vk_game_objects game_objs;
	struct vk_device_properties device_properties;
};

struct window {
	GLFWwindow *window;
	VkSurfaceKHR surface;
	struct input input;
};

struct vk_program {
	VkApplicationInfo app_info;
	VkInstance instance;
	struct vk_device device;
	struct window game_window;
	struct game_data game;
};

#endif //VK_TYPES_H
