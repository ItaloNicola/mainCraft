#ifndef VK_IMAGE_VIEW_H
#define VK_IMAGE_VIEW_H

#include "vk_types.h"

VkImageView
create_image_view(VkDevice logical_device, VkImage image, VkFormat format,
				  uint32_t mip_levels, VkImageAspectFlags aspect_flags);

void
image_views_cleanup(VkDevice logical_device, VkImageView *image_views, uint32_t images_count);

int
create_image(struct vk_device *dev, uint32_t width, uint32_t height, uint32_t mip_levels,
			 VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
			 VkMemoryPropertyFlags properties, VkImage* image, VmaAllocation *image_memory);

int
transition_image_layout(struct vk_cmd_submission *cmd_sub, VkImage image, uint32_t mip_levels,
						VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout);

int
copy_buffer_to_image(struct vk_cmd_submission *cmd_sub, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

int
create_gpu_image(struct vk_device *dev, VmaAllocation staging_buffer_memory, VkBuffer staging_buffer,
				 void *staging_buffer_data, int tex_width, int tex_height, uint32_t mip_levels,
				 VmaAllocation *texture_image_memory, VkImage *texture_image);

#endif //VK_IMAGE_VIEW_H
