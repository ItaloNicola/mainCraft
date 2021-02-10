#include "vk_command_buffer.h"
#include "vk_buffer.h"
#include "vk_image.h"
#include "utils.h"

VkImageView
create_image_view(VkDevice logical_device, VkImage image, VkFormat format,
				  uint32_t mip_levels, VkImageAspectFlags aspect_flags)
{
	VkImageView image_view;
	VkResult result;

	VkImageViewCreateInfo view_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = format,
		.components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
		.subresourceRange.aspectMask = aspect_flags,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.levelCount = mip_levels,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1
	};

	result = vkCreateImageView(logical_device, &view_info, NULL, &image_view);
	if (result != VK_SUCCESS) {
		print_error("Failed to create texture image view!");
		return VK_NULL_HANDLE;
	}

	return image_view;
}

void
image_views_cleanup(VkDevice logical_device, VkImageView *image_views, uint32_t images_count)
{
	int i;

	for (i = 0; i < images_count && image_views[i]; i++)
		vkDestroyImageView(logical_device, image_views[i], NULL);

	free(image_views);
}

int
create_image(struct vk_device *dev, uint32_t width, uint32_t height, uint32_t mip_levels,
			 VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
			 VkMemoryPropertyFlags properties, VkImage* image, VmaAllocation *image_memory)
{
	VkResult result;

	VkImageCreateInfo image_info = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.extent.width = width,
		.extent.height = height,
		.extent.depth = 1,
		.mipLevels = mip_levels,
		.arrayLayers = 1,
		.format = format,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.usage = usage,
		.samples = VK_SAMPLE_COUNT_1_BIT,
	};

	if (dev->cmd_submission.cmd_buffers_count[transfer] > 0)
		/* It will be used by graphics and transfer command buffers */
		image_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
	else
		image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VmaAllocationCreateInfo alloc_info = {
		.requiredFlags = properties
	};

	result = vmaCreateImage(dev->mem_allocator, &image_info, &alloc_info, image, image_memory, NULL);
	if (result != VK_SUCCESS) {
		print_error("failed to create image!");
		return -1;
	}

	return 0;
}

int
transition_image_layout(struct vk_cmd_submission *cmd_sub, VkImage image, uint32_t mip_levels,
						VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout)
{
	VkPipelineStageFlags src_stage;
	VkPipelineStageFlags dst_stage;
	VkCommandBuffer cmd_buffer;
	VkResult result;
	VkQueue queue;

	VkImageMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.oldLayout = old_layout,
		.newLayout = new_layout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.subresourceRange.baseMipLevel = 0,
		.subresourceRange.levelCount = mip_levels,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1,
		.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
	};

	if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else {
		print_error("Unsupported layout transition!");
		return -1;
	}

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

	vkCmdPipelineBarrier(cmd_buffer, src_stage, dst_stage, 0, 0, NULL, 0, NULL, 1, &barrier);

	result = end_single_time_commands(cmd_buffer, queue);
	if (result != VK_SUCCESS)
		return -1;

	return 0;
}

int
copy_buffer_to_image(struct vk_cmd_submission *cmd_sub, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
	VkCommandBuffer cmd_buffer;
	VkResult result;
	VkQueue queue;

	VkBufferImageCopy region = {
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.imageSubresource.mipLevel = 0,
		.imageSubresource.baseArrayLayer = 0,
		.imageSubresource.layerCount = 1,
		.imageOffset = { 0, 0, 0 },
		.imageExtent = { width, height, 1 }
	};

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

	vkCmdCopyBufferToImage(cmd_buffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	result = end_single_time_commands(cmd_buffer, queue);
	if (result != VK_SUCCESS)
		return -1;

	return 0;
}

int
generate_mipmaps(VkCommandBuffer cmd_buffer, VkQueue queue, VkImage image, VkFormat image_format,
				 int32_t tex_width, int32_t tex_height, uint32_t mip_levels)
{
	int32_t mip_width = tex_width, mip_height = tex_height, i;
	VkResult result;

	result = begin_single_time_commands(cmd_buffer);
	if (result != VK_SUCCESS)
		return -1;

	VkImageMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.image = image,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.subresourceRange.baseArrayLayer = 0,
		.subresourceRange.layerCount = 1,
		.subresourceRange.levelCount = 1
	};

	for (i = 1; i < mip_levels; i++) {
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.subresourceRange.baseMipLevel = i - 1;

		vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
							 VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

		VkImageBlit blit = {
			.srcOffsets[0] = { 0, 0, 0 },
			.srcOffsets[1] = { mip_width, mip_height, 1 },
			.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.srcSubresource.mipLevel = i - 1,
			.srcSubresource.baseArrayLayer = 0,
			.srcSubresource.layerCount = 1,
			.dstOffsets[0] = { 0, 0, 0 },
			.dstOffsets[1] = { mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1 },
			.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.dstSubresource.mipLevel = i,
			.dstSubresource.baseArrayLayer = 0,
			.dstSubresource.layerCount = 1
		};

		vkCmdBlitImage(cmd_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,
					   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
							 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

		if (mip_width > 1)
			mip_width /= 2;

		if (mip_height > 1)
			mip_height /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mip_levels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
						 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);

	result = end_single_time_commands(cmd_buffer, queue);
	if (result != VK_SUCCESS)
		return -1;

	return 0;
}

int
create_gpu_image(struct vk_device *dev, VmaAllocation staging_buffer_memory, VkBuffer staging_buffer,
				 void *staging_buffer_data, int tex_width, int tex_height, uint32_t mip_levels,
				 VmaAllocation *texture_image_memory, VkImage *texture_image)
{
	struct vk_cmd_submission *cmd_sub = &dev->cmd_submission;
	VmaAllocation local_texture_image_memory;
	VkImage local_texture_image;
	int ret;

	ret = create_image(dev, tex_width, tex_height, mip_levels, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
					   VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
					   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &local_texture_image, &local_texture_image_memory);
	if (ret)
		goto return_error;

	/* The following steps do:
	 * 1) The transition from a "stbi layout"(vkCmdCopyBufferToImage) to a
	 *    transfer optimized layout.
	 * 2) Copy the the image with this new layout to the a image buffer.
	 * 3) Transition to a layout that is readable to the shader.
	 * The step (1) is necessary because in the step (2) we are using the
	 * vkCmdCopyBufferToImage function. And the step (3) is necessary
	 * to shader access the texture.
	 * */
	ret = transition_image_layout(&dev->cmd_submission, local_texture_image, mip_levels, VK_FORMAT_R8G8B8A8_SRGB,
								  VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	if (ret)
		goto destroy_image;

	ret = copy_buffer_to_image(&dev->cmd_submission, staging_buffer, local_texture_image, tex_width, tex_height);
	if (ret)
		goto destroy_image;

	ret = generate_mipmaps(cmd_sub->cmd_buffers[graphics][0], cmd_sub->queue_handles[graphics],
						   local_texture_image, VK_FORMAT_R8G8B8A8_SRGB, tex_width, tex_height, mip_levels);
	if (ret)
		goto destroy_image;

	*texture_image_memory = local_texture_image_memory;
	*texture_image = local_texture_image;

	return 0;

destroy_image:
	vmaDestroyImage(dev->mem_allocator, local_texture_image, local_texture_image_memory);
return_error:
	return ret;
}
