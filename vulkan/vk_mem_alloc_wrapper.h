#ifndef VK_MEM_ALLOC_H
#define VK_MEM_ALLOC_H

#include "vk_types.h"

#ifdef __cplusplus
extern "C" {
#endif

VkResult
init_vulkan_memory_allocator(struct vk_program *program);

#ifdef __cplusplus
}
#endif

#endif //VK_MEM_ALLOC_H
