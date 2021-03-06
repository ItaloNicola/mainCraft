#ifndef VK_SWAP_CHAIN_H
#define VK_SWAP_CHAIN_H

#include "vk_types.h"

int
create_swapchain(struct vk_device *device, VkSurfaceKHR surface, GLFWwindow *window);

int
create_swapchain_image_views(VkDevice logical_device, struct vk_swapchain *swapchain);

#endif //VK_SWAP_CHAIN_H
