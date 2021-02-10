#ifndef VK_GPU_OBJECTS_H
#define VK_GPU_OBJECTS_H

#include "game_objects.h"
#include "vk_types.h"

int
create_vertex_buffer(struct vk_device *dev, struct vk_vertex_object *vertex_object);

int
create_index_buffer(struct vk_device *dev, struct vk_vertex_object *vertex_object);

int
create_vp_ubo_buffers(struct vk_device *dev, struct view_projection *vp);

int
create_cubes_position_buffers(struct vk_device *dev, struct vk_vertex_object *vertex_object, uint32_t swapchain_images_count);

int
generate_terrain_buffer(struct vk_device *dev, fnl_state *noise, struct vk_vertex_object *cube);

void
destroy_buffer_vector(struct vk_device *dev, VkBuffer *buffers, VmaAllocation *buffers_memory, uint32_t buffer_count);

#endif //VK_GPU_OBJECTS_H
