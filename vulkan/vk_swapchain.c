#include <stdlib.h>
#include <stdio.h>

#include "vk_swapchain.h"
#include "vk_image.h"
#include "utils.h"


VkSurfaceFormatKHR
chooseSwapSurfaceFormat(const struct surface_support *swap_chain_support) {
	uint32_t i, formats_count = swap_chain_support->formats_count;
	VkSurfaceFormatKHR *available_formats = swap_chain_support->formats;

	for (i = 0; i < formats_count; i++)
		if (available_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
			available_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return available_formats[i];

	return available_formats[0];
}

VkPresentModeKHR
chooseSwapPresentMode(const struct surface_support *swap_chain_support)
{
	//const VkPresentModeKHR *available_present_modes = swap_chain_support->present_modes;
	//uint32_t i, present_modes_count = swap_chain_support->present_modes_count;

	//for (i = 0; i < present_modes_count; i++)
	//	if (availablePresentMode[i] == VK_PRESENT_MODE_MAILBOX_KHR)
	//		return availablePresentMode;

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D
chooseSwapExtent(const VkSurfaceCapabilitiesKHR capabilities, GLFWwindow *window)
{
	VkExtent2D max_image_extent = capabilities.maxImageExtent;
	VkExtent2D min_image_extent = capabilities.minImageExtent;
	VkExtent2D actual_extent;
	int width, height;

	if (capabilities.currentExtent.width != UINT32_MAX)
		return capabilities.currentExtent;

	glfwGetFramebufferSize(window, &width, &height);

	actual_extent.width = max(min_image_extent.width, min(max_image_extent.width, width));
	actual_extent.height = max(min_image_extent.height, min(max_image_extent.height, height));

	return actual_extent;
}

int
create_swapchain(struct vk_device *device, VkSurfaceKHR surface, GLFWwindow *window)
{
	VkSurfaceCapabilitiesKHR swapchain_capabilities = device->swapchain.support.capabilities;
	struct swapchain_info *swapchain_state = &device->swapchain.state;
	struct vk_cmd_submission *cmd_sub = &device->cmd_submission;
	VkSharingMode image_sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
	uint32_t queue_family_indices[queues_count];
	uint32_t family_count = 0;
	uint32_t image_count;
	VkResult result;
	VkImage* images;

	swapchain_state->present_mode = chooseSwapPresentMode(&device->swapchain.support);
	swapchain_state->surface_format = chooseSwapSurfaceFormat(&device->swapchain.support);
	swapchain_state->extent = chooseSwapExtent(swapchain_capabilities, window);

	image_count = swapchain_capabilities.minImageCount + 1;
	if (swapchain_capabilities.maxImageCount > 0 && image_count > swapchain_capabilities.maxImageCount)
		image_count = swapchain_capabilities.maxImageCount;

	queue_family_indices[family_count++] = cmd_sub->family_indices[graphics];

	if (cmd_sub->family_indices[present] != cmd_sub->family_indices[graphics]) {
		queue_family_indices[family_count++] = cmd_sub->family_indices[present];
		image_sharing_mode = VK_SHARING_MODE_CONCURRENT;
	}

	if (cmd_sub->queue_count[transfer]) {
		queue_family_indices[family_count++] = cmd_sub->family_indices[transfer];
		image_sharing_mode = VK_SHARING_MODE_CONCURRENT;
	}

	VkSwapchainCreateInfoKHR create_info = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = surface,
		.minImageCount = image_count,
		.imageFormat = swapchain_state->surface_format.format,
		.imageColorSpace = swapchain_state->surface_format.colorSpace,
		.imageExtent = swapchain_state->extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		.preTransform = swapchain_capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = swapchain_state->present_mode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE,
		.pQueueFamilyIndices = queue_family_indices,
		.queueFamilyIndexCount = family_count,
		.imageSharingMode = image_sharing_mode
	};

	result = vkCreateSwapchainKHR(device->logical_device, &create_info, NULL, &device->swapchain.handle);
	if (result != VK_SUCCESS) {
		print_error("Failed to create swap chain!");
		goto return_error;
	}

	result = vkGetSwapchainImagesKHR(device->logical_device, device->swapchain.handle, &image_count, NULL);
	if (result != VK_SUCCESS) {
		print_error("Failed to enumerate the swap chain images!");
		goto destroy_swapchain;
	}

	images = malloc(sizeof(VkImage) * image_count);
	if (!images) {
		print_error("Failed while allocating swapchain images vector!");
		goto destroy_swapchain;
	}

	result = vkGetSwapchainImagesKHR(device->logical_device, device->swapchain.handle, &image_count, images);
	if (result != VK_SUCCESS) {
		print_error("Failed retrieve swap chain images!");
		goto free_images;
	}

	device->swapchain.images = images;
	device->swapchain.images_count = image_count;

	return 0;

free_images:
	free(images);
destroy_swapchain:
	vkDestroySwapchainKHR(device->logical_device, device->swapchain.handle, NULL);
return_error:
	return -1;
}

int
create_swapchain_image_views(VkDevice logical_device, struct vk_swapchain *swapchain)
{
	int32_t i;

	swapchain->image_views = malloc(sizeof(VkImageView) * swapchain->images_count);
	if (!swapchain->image_views) {
		print_error("Failed while allocating swapchain image views vector!");
		return -1;
	}

	for (i = 0; i < swapchain->images_count; i++) {
		swapchain->image_views[i] = create_image_view(logical_device, swapchain->images[i],
													  swapchain->state.surface_format.format,
													  1, VK_IMAGE_ASPECT_COLOR_BIT);
		if (!swapchain->image_views[i]) {
			pprint_error("Failed to create %d/%u image view!", i, swapchain->images_count);
			break;
		}
	}

	if (i != swapchain->images_count) {
		image_views_cleanup(logical_device, swapchain->image_views, swapchain->images_count);
		return -1;
	}

	return 0;
}

