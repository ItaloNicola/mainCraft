#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "vk_logical_device.h"
#include "vk_constants.h"
#include "utils.h"


int
find_queue_families(VkPhysicalDevice physical_device, struct vk_cmd_submission *cmd_sub, VkSurfaceKHR surface)
{
	uint32_t i, queue_queue_count = 0;
	VkQueueFamilyProperties *queue_family;
	VkBool32 present_support = false;
	VkResult result;
	int ret = -1;

	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_queue_count, NULL);

	queue_family = malloc(sizeof(VkQueueFamilyProperties) * queues_count);
	if (!queue_family) {
		print_error("Failed to allocate queue_family vector!");
		goto return_error;
	}

	vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_queue_count, queue_family);

	for (i = 0; i < queue_queue_count; i++) {
		result = vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
		if (result != VK_SUCCESS) {
			print_error("Failed retrieve device surface support!");
			goto free_queue_family;
		}
		/* Verify if our gpu has a presentation family_indices queue
		 * Notice this queue is a extention, so in the vulkan core
		 * this isn't necessary
		 * */
		if (present_support && queue_family[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			cmd_sub->family_indices[present]= i;
			cmd_sub->queue_count[present] = queue_family[i].queueCount;
		} if (queue_family[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
			cmd_sub->family_indices[graphics] = i;
			cmd_sub->queue_count[graphics] = queue_family[i].queueCount;
		} else if (queue_family[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
			cmd_sub->family_indices[transfer] = i;
			cmd_sub->queue_count[transfer] = queue_family[i].queueCount;
		} else if (queue_family[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
			cmd_sub->family_indices[compute] = i;
			cmd_sub->queue_count[compute] = queue_family[i].queueCount;
		} else if (queue_family[i].queueFlags & VK_QUEUE_PROTECTED_BIT) {
			cmd_sub->family_indices[protectedBit] = i;
			cmd_sub->queue_count[protectedBit] = queue_family[i].queueCount;
		} else if (queue_family[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
			cmd_sub->family_indices[sparseBindingBit] = i;
			cmd_sub->queue_count[sparseBindingBit] = queue_family[i].queueCount;
		}
	}

	ret = 0;

free_queue_family:
	free(queue_family);
return_error:
	return ret;
}

bool
check_device_extension_support(VkPhysicalDevice physical_device)
{
	VkExtensionProperties *available_extensions;
	uint32_t extension_count, i, j, available_in_device = 0;
	VkResult result;
	bool ret = true;

	result = vkEnumerateDeviceExtensionProperties(physical_device, NULL, &extension_count, NULL);
	if (result != VK_SUCCESS) {
		print_error("Failed to enumerate device extentions");
		ret = false;
		goto return_value;
	}

	available_extensions = malloc(sizeof(VkExtensionProperties) * extension_count);
	if (!available_extensions) {
		print_error("Failed while probing device extentions");
		ret = false;
		goto return_value;
	}

	result = vkEnumerateDeviceExtensionProperties(physical_device, NULL, &extension_count, available_extensions);
	if (result != VK_SUCCESS) {
		print_error("Failed to retrieve device extentions");
		ret = false;
		goto free_available_extentions;
	}

	for (i = 0; i < extension_count; i++)
		for (j = 0; j < array_size(device_extensions); j++)
			if (!strcmp(available_extensions[i].extensionName, device_extensions[j]))
					available_in_device++;

	/* Check if all the necessary extentions are available in the device */
	if (available_in_device != array_size(device_extensions))
		ret = false;

free_available_extentions:
	free(available_extensions);
return_value:
	return ret;
}

int
query_surface_support(VkPhysicalDevice physical_device, VkSurfaceKHR surface, struct surface_support *surface_support)
{
	uint32_t formats_count, present_mode_count;
	surface_support->formats_count = surface_support->present_modes_count = 0;
	VkResult result;

	result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_support->capabilities);
	if (result != VK_SUCCESS) {
		print_error("Failed get device surface capabilities!");
		return -1;
	}

	result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formats_count, NULL);
	if (result != VK_SUCCESS) {
		print_error("Failed get device surface formats!");
		return -1;
	}

	if (formats_count) {
		surface_support->formats = malloc(sizeof(VkSurfaceFormatKHR) * formats_count);
		if (!surface_support->formats) {
			print_error("Failed while recovering physical Device surface formats!");
			return -1;
		}
		result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &formats_count, surface_support->formats);
		if (result != VK_SUCCESS) {
			print_error("Failed get device surface formats!");
			return -1;
		}
		surface_support->formats_count = formats_count;
	}

	result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count, NULL);
	if (result != VK_SUCCESS) {
		print_error("Failed get device surface present modes!");
		return -1;
	}

	if (present_mode_count) {
		surface_support->present_modes = malloc(sizeof(VkPresentModeKHR) * present_mode_count);
		if (!surface_support->present_modes) {
			print_error("Failed allocated the present modes vector!");
			return -1;
		}
		result = vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &present_mode_count,
														   surface_support->present_modes);
		if (result != VK_SUCCESS) {
			print_error("Failed get device surface present modes!");
			return -1;
		}
		surface_support->present_modes_count = present_mode_count;
	}

	return 0;
}

void
surface_support_cleanup(struct surface_support *surface_support)
{
	if (surface_support->formats_count)
		free(surface_support->formats);
	if (surface_support->present_modes_count)
		free(surface_support->present_modes);

	surface_support->formats_count = surface_support->present_modes_count = 0;
	surface_support->formats = NULL;
	surface_support->present_modes = NULL;
}

VkFormat
find_supported_format(VkPhysicalDevice physical_device, const VkFormat candidates[],
					  uint32_t condidates_count, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	int i;

	for (i = 0; i < condidates_count; i++) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physical_device, candidates[i], &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
			return candidates[i];
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
			return candidates[i];
	}

	return VK_FORMAT_UNDEFINED;
}

VkFormat
find_depth_format(VkPhysicalDevice physical_device)
{
	/* depth_buffer_formats is in vk_constants.c */
	return find_supported_format(physical_device, depth_buffer_formats, array_size(depth_buffer_formats),
								 VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool
is_device_suitable(VkPhysicalDevice physical_device, struct vk_device *picked_device, VkSurfaceKHR surface)
{
	VkPhysicalDeviceProperties *device_properties = &picked_device->device_properties.device_properties;
	VkPhysicalDeviceFeatures supported_features;
	struct vk_cmd_submission cmd_sub = { };
	struct surface_support surface_support = { };
	VkFormatProperties format_properties;
	bool extensions_supported;
	VkFormat depth_format;

	extensions_supported = check_device_extension_support(physical_device);
	if (!extensions_supported)
		goto return_false;

	if (query_surface_support(physical_device, surface, &surface_support))
		goto return_false;

	if (!surface_support.formats_count || !surface_support.present_modes_count)
		goto surface_support_cleanup;

	if (find_queue_families(physical_device, &cmd_sub, surface))
		goto surface_support_cleanup;

	if (!cmd_sub.queue_count[graphics] || !cmd_sub.queue_count[present])
		goto surface_support_cleanup;

	depth_format = find_depth_format(physical_device);
	if (depth_format == VK_FORMAT_UNDEFINED)
		goto surface_support_cleanup;

	vkGetPhysicalDeviceFeatures(physical_device, &supported_features);
	if(!supported_features.samplerAnisotropy)
		goto surface_support_cleanup;

    vkGetPhysicalDeviceFormatProperties(physical_device, VK_FORMAT_R8G8B8A8_SRGB, &format_properties);
	if(!(format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
		goto surface_support_cleanup;

	vkGetPhysicalDeviceProperties(physical_device, device_properties);
	picked_device->physical_device = physical_device;
	picked_device->cmd_submission = cmd_sub;
	picked_device->swapchain.support = surface_support;
	picked_device->device_properties.supported_features = supported_features;
	picked_device->render.depth_format = depth_format;

	return true;

surface_support_cleanup:
	surface_support_cleanup(&surface_support);
return_false:
	return false;
}

int
pick_physical_device(VkInstance instance, struct vk_device *device, VkSurfaceKHR surface)
{
	uint32_t device_count = 0, i;
	VkPhysicalDevice *devices;
	VkResult result;
	int ret = -1;

	vkEnumeratePhysicalDevices(instance, &device_count, NULL);
	if (device_count == 0) {
		print_error("Failed to find GPUs with Vulkan support!");
		goto return_error;
	}

	devices = malloc(sizeof(VkPhysicalDevice) * device_count);
	if (!devices) {
		print_error("Failed while allocating physical devices vector!");
		goto return_error;
	}

	result = vkEnumeratePhysicalDevices(instance, &device_count, devices);
	if (result != VK_SUCCESS) {
		print_error("Failed to retrieve physical devices!");
		goto free_device_vector;
	}

	for (i = 0; i < device_count; i++)
		if (is_device_suitable(devices[i], device, surface)) {
			ret = 0;
			break;
		}

	if (ret)
		print_error("Failed to find a suitable GPU!");

free_device_vector:
	free(devices);
return_error:
	return ret;
}

int
create_logical_device(struct vk_device *device)
{
	uint64_t unique_queue_families[queues_count], i, unique_family_count = 0;
	VkDeviceQueueCreateInfo queue_create_infos[queues_count];
	struct vk_cmd_submission *cmd_sub = &device->cmd_submission;
	uint32_t *family_indices = cmd_sub->family_indices;
	VkQueue *queue_handles = cmd_sub->queue_handles;
	const float queue_priority = 1.0f;
	VkResult result;

	/* Because we already verified it in `find_queue_families()` we don't need check
	 * if this GPU have a Graphics/Present queue
	 */
	unique_queue_families[unique_family_count++] = family_indices[graphics];
	if (family_indices[present] != family_indices[graphics])
		unique_queue_families[unique_family_count++] = family_indices[present];
	if (cmd_sub->queue_count[transfer])
		unique_queue_families[unique_family_count++] = family_indices[transfer];

	for (i = 0; i < unique_family_count; i++) {
		VkDeviceQueueCreateInfo queue_create_info = {
			.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
			.queueFamilyIndex = unique_queue_families[i],
			// If we want mutiple queues we need change this
			.queueCount = 1,
			.pQueuePriorities = &queue_priority,
		};
		queue_create_infos[i] = queue_create_info;
	}

	VkPhysicalDeviceFeatures device_features = {
		.samplerAnisotropy = VK_TRUE
	};

	VkDeviceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pQueueCreateInfos = queue_create_infos,
		.queueCreateInfoCount = unique_family_count,
		.enabledExtensionCount = array_size(device_extensions),
		.ppEnabledExtensionNames = device_extensions,
		.pEnabledFeatures = &device_features
	};

	if (enable_validation_layers) {
		create_info.enabledLayerCount = array_size(validation_layers);
		create_info.ppEnabledLayerNames = validation_layers;
	}

	result = vkCreateDevice(device->physical_device , &create_info, NULL, &device->logical_device);
	if (result != VK_SUCCESS) {
		print_error("Failed to create logical device!");
		return -1;
	}

	// And the retrieve the queues
	vkGetDeviceQueue(device->logical_device, family_indices[graphics], 0, &queue_handles[graphics]);
	vkGetDeviceQueue(device->logical_device, family_indices[present], 0, &queue_handles[present]);
	if (cmd_sub->queue_count[transfer]) {
		vkGetDeviceQueue(device->logical_device, family_indices[transfer], 0, &queue_handles[transfer]);
		cmd_sub->queue_count[transfer] = 1;
	}

	cmd_sub->queue_count[graphics] = 1;
	cmd_sub->queue_count[present] = 1;

	return 0;
}
