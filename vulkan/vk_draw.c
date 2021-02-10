#define GLM_FORCE_RADIANS
#include <cglm/project.h>
#include <cglm/cglm.h>
#include <string.h>
#include <stdlib.h>

#include "vk_resource_manager.h"
#include "game_objects.h"
#include "vk_draw.h"
#include "utils.h"

int
create_sync_objects(VkDevice logical_device, struct vk_draw_sync *sync, uint32_t images_count)
{
	VkResult result;
	int i;

	sync->images_in_flight = calloc(sizeof(VkFence), images_count);
	if (!sync->images_in_flight) {
		print_error("Failed to allocate images_in_fligth vector");
		return -1;
	}

	VkSemaphoreCreateInfo semaphore_info = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};

	VkFenceCreateInfo fence_info = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		/* Create a fence that is already 'unlock', in other words
		 * this fence start will not block anything at beggining */
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	for (i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		result = vkCreateSemaphore(logical_device, &semaphore_info, NULL, &sync->image_available_semaphore[i]);
		if (result != VK_SUCCESS) {
			print_error("Failed to create semaphores!");
			break;
		}

		result = vkCreateSemaphore(logical_device, &semaphore_info, NULL, &sync->render_finished_semaphore[i]);
		if (result != VK_SUCCESS) {
			print_error("Failed to create semaphores!");
			break;
		}

		result = vkCreateFence(logical_device, &fence_info, NULL, &sync->in_flight_fences[i]);
		if (result != VK_SUCCESS) {
			print_error("Failed to create fences!");
			break;
		}
	}

	if (i != MAX_FRAMES_IN_FLIGHT) {
		sync_objects_cleanup(logical_device, sync);
		return -1;
	}

	return 0;
}

void
sync_objects_cleanup(VkDevice logical_device, struct vk_draw_sync *sync)
{
	int i;

	for (i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (sync->render_finished_semaphore[i])
			vkDestroySemaphore(logical_device, sync->render_finished_semaphore[i], NULL);
		if (sync->image_available_semaphore[i])
			vkDestroySemaphore(logical_device, sync->image_available_semaphore[i], NULL);
		if (sync->in_flight_fences[i])
			vkDestroyFence(logical_device, sync->in_flight_fences[i], NULL);
	}

	free(sync->images_in_flight);
}

void
update_view_projection(const VmaAllocator mem_allocator, struct view_projection *camera, uint32_t current_image)
{
	mat4 view_proj;
	void* data;

	glm_mat4_mulN((mat4 *[]){ &camera->proj, &camera->view }, 2, view_proj);

	vmaMapMemory(mem_allocator, camera->buffers_memory[current_image], &data);
	memcpy(data, &view_proj, sizeof(mat4));
	vmaUnmapMemory(mem_allocator, camera->buffers_memory[current_image]);
}

int
acquire_swapchain_image(struct vk_program *program, uint8_t current_frame, uint32_t *imageIndex)
{
	struct vk_device *dev = &program->device;
	const VkDevice logical_device = dev->logical_device;
	const struct vk_draw_sync *sync = &dev->draw_sync;
	VkSemaphore image_available_semaphore = sync->image_available_semaphore[current_frame];
	VkFence in_flight_fences = sync->in_flight_fences[current_frame];
	VkResult result;
	int ret;

	vkWaitForFences(logical_device, 1, &in_flight_fences, VK_TRUE, UINT64_MAX);

	result = vkAcquireNextImageKHR(logical_device, dev->swapchain.handle, UINT64_MAX,
								   image_available_semaphore, VK_NULL_HANDLE, imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		ret = recreate_render_and_presentation_infra(program);
		if (ret == 0)
			return VK_ERROR_OUT_OF_DATE_KHR;
		return -1;
	}else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		print_error("failed to acquire swap chain image!");
		return -1;
	}

	if (sync->images_in_flight[*imageIndex] != VK_NULL_HANDLE)
		vkWaitForFences(logical_device, 1, &sync->images_in_flight[*imageIndex], VK_TRUE, UINT64_MAX);

	sync->images_in_flight[*imageIndex] = in_flight_fences;

	result = vkResetFences(logical_device, 1, &in_flight_fences);
	if (result != VK_SUCCESS) {
		print_error("Failed to reset fences!");
		return -1;
	}

	return 0;
}

int
draw_frame(struct vk_program *program, uint8_t current_frame, uint32_t imageIndex)
{
	struct vk_device *dev = &program->device;
	const struct vk_draw_sync *sync = &dev->draw_sync;
	VkSemaphore image_available_semaphore = sync->image_available_semaphore[current_frame];
	VkSemaphore render_finished_semaphore = sync->render_finished_semaphore[current_frame];
	VkFence in_flight_fences = sync->in_flight_fences[current_frame];
	const VkQueue *queues = dev->cmd_submission.queue_handles;
	bool *framebuffer_resized = &dev->swapchain.framebuffer_resized;
	VkCommandBuffer **cmd_buffers = dev->cmd_submission.cmd_buffers;
	VkResult result;

	/* The swapchain that will be used in present_info */
	VkSwapchainKHR swapchains[] = { dev->swapchain.handle };
	/* All bellow used in submit_info to submit the graphics command buffer */
	VkSemaphore waitSemaphores[] = { image_available_semaphore };
	/* Specifies to which semaphore a signal will be emited to after the rendering*/
	VkSemaphore signalSemaphores[] = { render_finished_semaphore };
	/* Which stages of the pipeline the submit will wait */
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = array_size(waitSemaphores),
		.pWaitSemaphores = waitSemaphores,
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmd_buffers[graphics][imageIndex],
		.signalSemaphoreCount = array_size(signalSemaphores),
		.pSignalSemaphores = signalSemaphores
	};

	result = vkQueueSubmit(queues[graphics], 1, &submitInfo, in_flight_fences);
	if (result != VK_SUCCESS) {
		print_error("Failed to submit draw command buffer!");
		return -1;
	}

	VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = array_size(signalSemaphores),
		.pWaitSemaphores = signalSemaphores,
		.swapchainCount = array_size(swapchains),
		.pSwapchains = swapchains,
		.pImageIndices = &imageIndex
	};

	result = vkQueuePresentKHR(queues[present], &presentInfo);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || *framebuffer_resized) {
		*framebuffer_resized = false;
		if (recreate_render_and_presentation_infra(program))
			return -1;
	} else if (result != VK_SUCCESS) {
		print_error("Failed to present frame!");
		return -1;
	}

	return 0;
}

