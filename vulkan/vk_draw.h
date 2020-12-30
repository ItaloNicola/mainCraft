#ifndef VK_DRAW_H
#define VK_DRAW_H

#include <vulkan/vulkan.h>
#include "vk_types.h"


int
create_sync_objects(VkDevice logical_device, struct vk_draw_sync *sync, uint32_t images_count);

void
sync_objects_cleanup(VkDevice logical_device, struct vk_draw_sync *sync);

#endif //VK_DRAW_H
