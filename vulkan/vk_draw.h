#ifndef VK_DRAW_H
#define VK_DRAW_H

#include <vulkan/vulkan.h>
#include "vk_types.h"


int
create_sync_objects(VkDevice logical_device, struct vk_draw_sync *sync, uint32_t images_count);

void
sync_objects_cleanup(VkDevice logical_device, struct vk_draw_sync *sync);

void
update_view_projection(const VmaAllocator mem_allocator, struct view_projection *camera, uint32_t current_image);

int
acquire_swapchain_image(struct vk_program *program, uint8_t current_frame, uint32_t *imageIndex);

int
draw_frame(struct vk_program *program, uint8_t current_frame, uint32_t imageIndex);

#endif //VK_DRAW_H
