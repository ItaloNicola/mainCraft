#include <stdlib.h>
#include <stdio.h>

#include "vk_resource_manager.h"
#include "player_view.h"
#include "vk_window.h"
#include "vk_draw.h"
#include "types.h"

int
vk_main_loop(struct vk_program *program)
{
	GLFWwindow *window = program->game_window.window;
	struct vk_device *dev = &program->device;
	struct view_projection *camera = &dev->game_objs.camera;
	struct game_data *game = &program->game;
	uint8_t current_frame = 0;
	int ret = 0;

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		update_position_and_view(window, &program->game_window.input, game, camera->view, -1.0f);

		if (draw_frame(program, &current_frame)) {
			ret = -1;
			break;
		}
	}

	vkDeviceWaitIdle(dev->logical_device);

	return ret;
}

int
run_vk(const int argc, char *const *argv)
{
	int exit_status = EXIT_FAILURE;
	struct vk_program program = { };
	struct glfw_callback_data callback_data = {
		.input = &program.game_window.input,
		.window_resized = &program.device.swapchain.framebuffer_resized
	};

	if (vk_init_window(&program, &callback_data))
		goto exit_program;

	if (init_vk(&program))
		goto destroy_window;

	if (vk_main_loop(&program))
		goto vk_cleanup;

	exit_status = EXIT_SUCCESS;

vk_cleanup:
	vk_cleanup(&program);
destroy_window:
	vk_destroy_window(&program.game_window);
exit_program:
	return exit_status;
}

