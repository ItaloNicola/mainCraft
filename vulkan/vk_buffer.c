#include <string.h>

#include "vk_command_buffer.h"
#include "vk_buffer.h"
#include "utils.h"


int
create_buffer(struct vk_device *dev, VkDeviceSize size, VkBufferUsageFlags usage,
			  VkMemoryPropertyFlags properties, VkBuffer *buffer, VmaAllocation *buffer_memory)
{
	VkResult result;

	VkBufferCreateInfo buffer_info = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};

	VmaAllocationCreateInfo alloc_info = {
		.requiredFlags = properties
	};

	result = vmaCreateBuffer(dev->mem_allocator, &buffer_info, &alloc_info, buffer, buffer_memory, NULL);
	if (result != VK_SUCCESS) {
		print_error("Failed to create buffer!");
		return -1;
	}

	return 0;
}

int
copy_buffer(struct vk_cmd_submission *cmd_sub, VkBuffer src_buffer, VkBuffer dst_buffer, VkDeviceSize size)
{
	VkCommandBuffer cmd_buffer;
	VkResult result;
	VkQueue queue;

	if (cmd_sub->cmd_buffers_count[transfer] > 0) {
		cmd_buffer = cmd_sub->cmd_buffers[transfer][0];
		queue = cmd_sub->queue_handles[transfer];
	} else {
		cmd_buffer = cmd_sub->cmd_buffers[graphics][0];
		queue = cmd_sub->queue_handles[graphics];
	}

	result = begin_single_time_commands(cmd_buffer);
	if (result != VK_SUCCESS)
		return -1;

	VkBufferCopy copy_region = {
		.size = size
	};

	vkCmdCopyBuffer(cmd_buffer, src_buffer, dst_buffer, 1, &copy_region);

	result = end_single_time_commands(cmd_buffer, queue);
	if (result != VK_SUCCESS)
		return -1;

	return 0;
}

int
create_gpu_buffer(struct vk_device *dev, VmaAllocation *buffer_memory, VkBuffer *buffer,
				  void *buffer_data, VkDeviceSize buffer_size, VkBufferUsageFlags usage)
{
	VmaAllocation staging_buffer_memory, local_buffer_memory;
	VkBuffer staging_buffer, local_buffer;
	VkResult result;
	void *data;
	int ret;

	ret = create_buffer(dev, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					    &staging_buffer, &staging_buffer_memory);
	if (ret)
		goto return_error;

	result = vmaMapMemory(dev->mem_allocator, staging_buffer_memory, &data);
	if (result != VK_SUCCESS) {
		print_error("Failed to map buffer to system memory!");
		goto destoy_staging_buffer;
	}

	memcpy(data, buffer_data, (size_t) buffer_size);
	vmaUnmapMemory(dev->mem_allocator, staging_buffer_memory);

	ret = create_buffer(dev, buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
					   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &local_buffer, &local_buffer_memory);
	if (ret)
		goto destoy_staging_buffer;

	if (copy_buffer(&dev->cmd_submission, staging_buffer, local_buffer, buffer_size)) {
		vmaDestroyBuffer(dev->mem_allocator, local_buffer, local_buffer_memory);
		goto destoy_staging_buffer;
	}

	*buffer = local_buffer;
	*buffer_memory = local_buffer_memory;

	ret = 0;

destoy_staging_buffer:
	vmaDestroyBuffer(dev->mem_allocator, staging_buffer, staging_buffer_memory);
return_error:
	return ret;
}

