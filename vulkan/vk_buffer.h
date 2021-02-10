#ifndef VK_BUFFER_H
#define VK_BUFFER_H

#include <vulkan/vulkan.h>

#include "vk_types.h"


int
copy_buffer(struct vk_cmd_submission *cmd_sub, VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);

int
create_gpu_buffer(struct vk_device *dev, VmaAllocation *buffer_memory, VkBuffer *buffer,
				  void *buffer_data, VkDeviceSize buffer_size, VkBufferUsageFlags usage);

int
create_buffer(struct vk_device *dev, VkDeviceSize size, VkBufferUsageFlags usage,
			  VkMemoryPropertyFlags properties, VkBuffer *buffer, VmaAllocation *buffer_memory);

int
copy_buffer(struct vk_cmd_submission *cmd_sub, VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size);

#endif //VK_BUFFER_H
