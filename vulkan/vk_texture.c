#include <string.h>
#include <stdio.h>

#include "VulkanMemoryAllocator/vk_mem_alloc.h"
#include "vk_texture.h"
#include "vk_buffer.h"
#include "vk_image.h"
#include "utils.h"

#define TEX_DIR "assets/textures/"

void
destroy_texture_images(struct vk_device *dev, VmaAllocation *texture_image_memory,
					   VkImage *texture_image, uint32_t images_count)
{
	int i;

	for (i = 0; i < images_count; i++)
		vmaDestroyImage(dev->mem_allocator, texture_image[i], texture_image_memory[i]);

	free(texture_image_memory);
	free(texture_image);
}

uint64_t
calculate_staging_buffer_size(FILE *image_files[], uint32_t images_count)
{
	int tex_width[images_count], tex_height[images_count];
	VkDeviceSize staging_buffer_size = 0;
	uint64_t current_image_size;
	int tex_channel, i, ret;

	for (i = 0; i < images_count; i++) {
		ret = stbi_info_from_file(image_files[i], &tex_width[i], &tex_height[i], &tex_channel);
		if (!ret)
			return 0;

		current_image_size = tex_width[i] * tex_height[i] * 4;
		if (staging_buffer_size < current_image_size)
			staging_buffer_size = current_image_size;
	}

	return staging_buffer_size;
}

int
create_texture_images(struct vk_device *dev, char *image_names[], uint32_t images_count,
					  VkImage **texture_image, VmaAllocation **texture_image_memory)
{
	VmaAllocation *local_texture_images_memory, staging_buffer_memory;
	int tex_width, tex_height, tex_channels, i, ret = -1;
	VkDeviceSize staging_buffer_size;
	FILE *image_files[images_count];
	VkImage *local_texture_images;
	VkBuffer staging_buffer;
	VkResult result;
	stbi_uc* pixels;
	void* data;

	memset(image_files, 0, sizeof(FILE *) * images_count);

	for (i = 0; i < images_count; i++) {
		image_files[i] = fopen(image_names[i], "r");
		if (!image_files[i]) {
			pprint_error("Failed to open the '%s' texture file!", image_names[i]);
			goto close_files;
		}
	}

	staging_buffer_size = calculate_staging_buffer_size(image_files, images_count);
	if (staging_buffer_size == 0) {
		print_error("Failed to retrieve image info about texture file!");
		goto close_files;
	}

	ret = create_buffer(dev, staging_buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
						VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory);
	if (ret)
		goto close_files;

	result = vmaMapMemory(dev->mem_allocator, staging_buffer_memory, &data);
	if (result != VK_SUCCESS) {
		print_error("Failed to map buffer to system memory!");
		goto destoy_staging_buffer;
	}

	local_texture_images = malloc(sizeof(VkImage) * images_count);
	if (!local_texture_images) {
		print_error("Failed to allocate texture images vector!");
		goto unmap_staging_buffer;
	}

	local_texture_images_memory = malloc(sizeof(VmaAllocation) * images_count);
	if (!local_texture_images_memory) {
		print_error("Failed to allocate texture images memory vector!");
		free(local_texture_images);
		goto unmap_staging_buffer;
	}

	for (i = 0; i < images_count; i++) {
		pixels = stbi_load_from_file(image_files[i], &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
		if (!pixels) {
			print_error("failed to load texture image!");
			break;
		}

		memcpy(data, pixels, tex_width * tex_height * 4);

		stbi_image_free(pixels);

		ret = create_gpu_image(dev, staging_buffer_memory, staging_buffer, data, tex_width, tex_height,
							   &local_texture_images_memory[i], &local_texture_images[i]);
		if (ret)
			break;
	}

	if (i != images_count) {
		destroy_texture_images(dev, local_texture_images_memory, local_texture_images, i);
		goto unmap_staging_buffer;
	}

	*texture_image = local_texture_images;
	*texture_image_memory = local_texture_images_memory;

	ret = 0;

unmap_staging_buffer:
	vmaUnmapMemory(dev->mem_allocator, staging_buffer_memory);
destoy_staging_buffer:
	vmaDestroyBuffer(dev->mem_allocator, staging_buffer, staging_buffer_memory);
close_files:
	for (i = 0; i < images_count && image_files[i]; i++)
		fclose(image_files[i]);

	return ret;
}

void
destroy_texture_image_views(VkDevice logical_device, VkImageView *texture_images_view, uint32_t images_count)
{
	int i;

	for (i = 0; i < images_count; i++)
		vkDestroyImageView(logical_device, texture_images_view[i], NULL);

	free(texture_images_view);
}

/* Create a ImageView to our textures(from createTextureImage).
 * As the swapchain imageView we cannot access the content of texture
 * directly, we need a imageView to access it.
 * */
int
create_texture_image_views(VkDevice logical_device, VkImage *texture_image,
						   VkImageView **texture_images_view, uint32_t images_count)
{
	VkImageView *local_image_view;
	int i;

	local_image_view = calloc(images_count, sizeof(VkImageView));
	if (!local_image_view) {
		print_error("Failed to alloc textures image views!");
		goto return_error;
	}

	for (i = 0; i < images_count; i++) {
		local_image_view[i] = create_image_view(logical_device, texture_image[i],
												VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
		if (local_image_view[i] == VK_NULL_HANDLE) {
			print_error("Failed to create texture image view!");
			break;
		}
	}

	if (i != images_count)
		goto destroy_texture_views;

	*texture_images_view = local_image_view;

	return 0;

destroy_texture_views:
	destroy_texture_image_views(logical_device, local_image_view, i);
return_error:
	return -1;
}

VkSampler
create_texture_sampler(VkDevice logical_device, VkPhysicalDeviceProperties *device_properties)
{
	VkSampler texture_sampler;
	VkResult result;

	VkSamplerCreateInfo sampler_info = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.maxAnisotropy = device_properties->limits.maxSamplerAnisotropy,
		.anisotropyEnable = VK_TRUE,
		.unnormalizedCoordinates = VK_FALSE,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.mipLodBias = 0.0f,
		.minLod = 0.0f,
		.maxLod = 0.0f
	};

	result = vkCreateSampler(logical_device, &sampler_info, NULL, &texture_sampler);
	if (result != VK_SUCCESS) {
		print_error("Failed to create texture sampler!");
		return VK_NULL_HANDLE;
	}

	return texture_sampler;
}

int
load_cube_textures(struct vk_device *dev)
{
	struct vk_vertex_object *cube = &dev->game_objs.cube;
	int ret;

	char *textures_names[] = {
		TEX_DIR "stone_bricks.png", TEX_DIR "bricks.png", TEX_DIR "grass_block_top.png",
		TEX_DIR "sand.png",	TEX_DIR "coarse_dirt.png", TEX_DIR "grass_block_side.png"
	};

	ret = create_texture_images(dev, textures_names, array_size(textures_names),
								&cube->texture_images, &cube->texture_images_memory);

	if (!ret)
		cube->texture_count = array_size(textures_names);

	return ret;
}

int
load_all_textures(struct vk_device *dev)
{
	return load_cube_textures(dev);
}
