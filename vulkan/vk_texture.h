#ifndef VK_TEXTURE_H
#define VK_TEXTURE_H

#include <stb/stb_image.h>

#include "vk_types.h"

int
load_all_textures(struct vk_device *dev);

void
destroy_texture_images(struct vk_device *dev, VmaAllocation *texture_image_mem,
					   VkImage *texture_image, uint32_t images_count);
int
create_texture_image_views(VkDevice logical_device, VkImage *texture_image,
						   VkImageView **texture_images_view, uint32_t images_count);

void
destroy_texture_image_views(VkDevice logical_device, VkImageView *texture_images_view, uint32_t images_count);

VkSampler
create_texture_sampler(VkDevice logical_device, VkPhysicalDeviceProperties *device_properties);

#endif //VK_TEXTURE_H
