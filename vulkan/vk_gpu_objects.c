#include <string.h>

#include "vk_gpu_objects.h"
#include "vk_constants.h"
#include "vk_buffer.h"
#include "terrain.h"
#include "utils.h"

int
create_vertex_buffer(struct vk_device *dev, struct vk_vertex_object *vertex_object)
{
	VkDeviceSize size = sizeof(struct vertex) * vertex_object->vertices_count;

	return create_gpu_buffer(dev, &vertex_object->vertex_buffer_memory, &vertex_object->vertex_buffer,
							 vertex_object->vertices, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
}

int
create_index_buffer(struct vk_device *dev, struct vk_vertex_object *vertex_object)
{
	VkDeviceSize size = sizeof(uint16_t) * vertex_object->indices_count;

	return create_gpu_buffer(dev, &vertex_object->index_buffer_memory, &vertex_object->index_buffer,
							 vertex_object->indices, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
}

int
create_cubes_position_buffers(struct vk_device *dev, struct vk_vertex_object *vertex_object, uint32_t swapchain_images_count)
{
	VmaAllocation *local_buffer_memory;
	VkBuffer *local_buffer;
	int i, ret;

	local_buffer = malloc(sizeof(VkBuffer) * swapchain_images_count);
	if (!local_buffer) {
		print_error("Failed to allocate cubes position buffers vector");
		goto return_error;
	}

	local_buffer_memory = malloc(sizeof(VmaAllocation) * swapchain_images_count);
	if (!local_buffer_memory) {
		print_error("Failed to allocate cubes position buffers vector");
		goto free_local_buffer;
	}

	for (i = 0; i < swapchain_images_count; i++) {
		ret = create_buffer(dev, CUBES_POSITION_BUFFER_SIZE,
							VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
							VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &local_buffer[i], &local_buffer_memory[i]);
		if (ret)
			break;
	}

	if (i != swapchain_images_count) {
		print_error("Failed to create cube position gpu buffer!");
		destroy_buffer_vector(dev, local_buffer, local_buffer_memory, i);
		goto free_local_buffer_memory;
	}

	vertex_object->position_buffer = local_buffer;
	vertex_object->position_buffer_memory = local_buffer_memory;
	vertex_object->position_buffer_count = swapchain_images_count;

	return 0;

free_local_buffer_memory:
	free(local_buffer_memory);
free_local_buffer:
	free(local_buffer);
return_error:
	return -1;
}

int
generate_terrain_buffer(struct vk_device *dev, fnl_state *noise, struct vk_vertex_object *cube)
{
	VkResult result;
	void *data;

	result = vmaMapMemory(dev->mem_allocator, cube->staging_position_buffer_memory, &data);
	if (result != VK_SUCCESS) {
		print_error("Failed to map the position staging buffer");
		return -1;
	}

	generate_terrain(data, noise, -140, -140, 140, 140);

	vmaUnmapMemory(dev->mem_allocator, cube->staging_position_buffer_memory);

	cube->position_count = 280 * 280;

	return 0;
}

int
create_vp_ubo_buffers(struct vk_device *dev, struct view_projection *vp)
{
	struct vk_swapchain *swapchain = &dev->swapchain;
	VkDeviceSize buffer_size = sizeof(mat4);
	VmaAllocation *local_buffer_memory;
	VkBuffer *local_buffer;
	size_t i, ret;

	local_buffer = malloc(sizeof(VkBuffer) * swapchain->images_count);
	if (!local_buffer) {
		print_error("Failed to allocate view-projection uniform buffer");
		return -1;
	}

	local_buffer_memory = malloc(sizeof(VmaAllocation) * swapchain->images_count);
	if (!local_buffer_memory) {
		print_error("Failed to allocate view-projection uniform buffer memory");
		free(vp->buffers);
		return -1;
	}

	/* We need create several(equal to the swap chains) because we have
	 * several swapchain buffers on the fly at same time and we need
	 * a uniform buffer with View Projection to each frame.
	 * */
	for (i = 0; i < swapchain->images_count; i++) {
		ret = create_buffer(dev, buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
							VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
							&local_buffer[i], &local_buffer_memory[i]);
		if (ret)
			break;
	}

	if (i != swapchain->images_count) {
		destroy_buffer_vector(dev, vp->buffers, vp->buffers_memory, i);
		return -1;
	}

	vp->buffers = local_buffer;
	vp->buffers_memory = local_buffer_memory;
	vp->buffer_count = swapchain->images_count;
	return 0;
}

void
destroy_buffer_vector(struct vk_device *dev, VkBuffer *buffers, VmaAllocation *buffers_memory, uint32_t buffer_count)
{
	int i;

	for (i = 0; i < buffer_count; i++)
		vmaDestroyBuffer(dev->mem_allocator, buffers[i], buffers_memory[i]);

	free(buffers);
	free(buffers_memory);
}
