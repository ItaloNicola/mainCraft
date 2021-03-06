#ifndef VK_CONSTANTS_H
#define VK_CONSTANTS_H

#include <vulkan/vulkan.h>


#ifdef ENABLE_VALIDATION_LAYERS
#define enable_validation_layers true
#else
#define enable_validation_layers false
#endif

#define MAX_FRAMES_IN_FLIGHT 2
#define CUBES_POSITION_BUFFER_SIZE 1048576 // 1 MB

extern const char *validation_layers[1];
extern const char *device_extensions[1];
extern const VkFormat depth_buffer_formats[3];

#endif //VK_CONSTANTS_H
