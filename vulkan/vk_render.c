#include <stdlib.h>

#include "vk_descriptors.h"
#include "vk_render.h"
#include "vk_image.h"
#include "utils.h"

VkRenderPass
create_render_pass(VkDevice logical_device, VkFormat depth_format, struct swapchain_info state)
{
	VkRenderPass render_pass;
	VkResult result;

	VkAttachmentDescription color_attachment = {
		.format = state.surface_format.format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	};

	VkAttachmentDescription depth_attachment = {
		.format = depth_format,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	VkAttachmentReference color_attachment_ref = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkAttachmentReference depth_attachment_ref = {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &color_attachment_ref,
		.pDepthStencilAttachment = &depth_attachment_ref
	};

	/* This subpass needs wait for the clear-color operation and the operation
	 * of write pixels in the framebuffer have to wait this subpass
	 * */
	VkSubpassDependency dependency = {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.srcAccessMask = 0,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
	};

	/* Currently we only have one of each ^^; (except the attachment)
	 * But we may have more in the future...
	 * */
	VkSubpassDescription subpasses[] = { subpass };
	VkSubpassDependency subpasses_dependencies[] = { dependency };
	VkAttachmentDescription attachments[] = { color_attachment, depth_attachment };

	VkRenderPassCreateInfo render_pass_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = array_size(attachments),
		.pAttachments = attachments,
		.subpassCount = array_size(subpasses),
		.pSubpasses = subpasses,
		.dependencyCount = array_size(subpasses_dependencies),
		.pDependencies = subpasses_dependencies
	};

	result = vkCreateRenderPass(logical_device, &render_pass_info, NULL, &render_pass);
	if (result != VK_SUCCESS) {
		print_error("Failed to create render pass!");
		return VK_NULL_HANDLE;
	}

	return render_pass;
}

VkShaderModule
create_shader_module(const VkDevice logical_device, const char *code, int64_t size) {
	VkShaderModule shader_module;
	VkResult result;

	VkShaderModuleCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = size,
		.pCode = (const uint32_t *) code
	};

	result = vkCreateShaderModule(logical_device, &create_info, NULL, &shader_module);
	if (result != VK_SUCCESS) {
		print_error("Failed to create shader module!");
		return VK_NULL_HANDLE;
	}

	return shader_module;
}

int
create_graphics_pipeline(const VkDevice logical_device, struct swapchain_info *swapchain_info, struct vk_render *render)
{
	static VkVertexInputAttributeDescription vertex_attribute_descriptions[3];
	static VkVertexInputBindingDescription vertex_binding_description[2];
	char *vert_shader_code, *frag_shader_code;
	int64_t vert_size, frag_size;
	VkPipeline pipeline;
	VkResult result;
	int ret = -1;

	vert_shader_code = read_file("shaders/vert.spv", &vert_size);
	if (!vert_shader_code)
		goto return_error;

	frag_shader_code = read_file("shaders/frag.spv", &frag_size);
	if (!frag_shader_code)
		goto destroy_vert_code;

	VkShaderModule vert_shader_module = create_shader_module(logical_device, vert_shader_code, vert_size);
	if (vert_shader_module == VK_NULL_HANDLE)
		goto destroy_frag_code;

	VkShaderModule frag_shader_module = create_shader_module(logical_device, frag_shader_code, frag_size);
	if (frag_shader_module == VK_NULL_HANDLE)
		goto destroy_vert_module;

	get_vertex_binding_description(0, vertex_binding_description);
	get_vertex_attribute_descriptions(0, 0, vertex_attribute_descriptions);
	get_vec3_binding_description(1, VK_VERTEX_INPUT_RATE_INSTANCE, &vertex_binding_description[1]);
	get_vec3_attribute_descriptions(1, 2, &vertex_attribute_descriptions[2]);

	VkPipelineVertexInputStateCreateInfo vertex_input_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = array_size(vertex_binding_description),
		.pVertexBindingDescriptions = vertex_binding_description,
		.vertexAttributeDescriptionCount = array_size(vertex_attribute_descriptions),
		.pVertexAttributeDescriptions = vertex_attribute_descriptions
	};

	VkPipelineShaderStageCreateInfo vert_shader_stage_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vert_shader_module,
		.pName = "main"
	};

	VkPipelineShaderStageCreateInfo frag_shader_stage_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = frag_shader_module,
		.pName = "main"
	};

	VkPipelineShaderStageCreateInfo shader_stages[] = {
		vert_shader_stage_info,
		frag_shader_stage_info
	};

	VkPipelineInputAssemblyStateCreateInfo input_assembly = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE
	};

	VkViewport viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.width = swapchain_info->extent.width,
		.height = swapchain_info->extent.height,
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	VkRect2D scissor = {
		.offset = {0, 0},
		.extent = swapchain_info->extent
	};

	/* Now just fill the struct with the two above */
	VkPipelineViewportStateCreateInfo viewport_state = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor
	};

	/* The rasterization node of the pipeline */
	VkPipelineRasterizationStateCreateInfo rasterizer = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.lineWidth = 1.0f,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		// It needs to be counter clockwise for reason that I don't understand
		//.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.0f, // Optional
		.depthBiasClamp = 0.0f, // Optional
		.depthBiasSlopeFactor = 0.0f // Optional
	};

	// This is really related with multisampling anti aliasing
	// but it requires a gpu feature be enabled
	VkPipelineMultisampleStateCreateInfo multisampling = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.sampleShadingEnable = VK_FALSE,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.minSampleShading = 1.0f, // Optional
		.pSampleMask = NULL, // Optional
		.alphaToCoverageEnable = VK_FALSE, // Optional
		.alphaToOneEnable = VK_FALSE // Optional
	};

	VkPipelineColorBlendAttachmentState color_blend_attachment = {
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
						  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		.blendEnable = VK_FALSE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
		.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
		.colorBlendOp = VK_BLEND_OP_ADD, // Optional
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
		.alphaBlendOp = VK_BLEND_OP_ADD // Optional
	};

	// This is the second struct of fixed function
	VkPipelineColorBlendStateCreateInfo color_blending = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY, // Optional
		.attachmentCount = 1,
		.pAttachments = &color_blend_attachment,
		.blendConstants[0] = 0.0f, // Optional
		.blendConstants[1] = 0.0f, // Optional
		.blendConstants[2] = 0.0f, // Optional
		.blendConstants[3] = 0.0f, // Optional
	};

	VkPipelineLayoutCreateInfo pipeline_layout_info = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &render->descriptor_set_layout
	};

	result = vkCreatePipelineLayout(logical_device, &pipeline_layout_info, NULL, &render->pipeline_layout);
	if (result != VK_SUCCESS) {
		print_error("Failed to create pipeline layout!");
		goto destroy_frag_module;
	}

	VkPipelineDepthStencilStateCreateInfo depth_stencil = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
	};

	VkGraphicsPipelineCreateInfo pipeline_info = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = array_size(shader_stages),
		.pStages = shader_stages,
		.pVertexInputState = &vertex_input_info,
		.pInputAssemblyState = &input_assembly,
		.pViewportState = &viewport_state,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisampling,
		.pDepthStencilState = &depth_stencil,
		.pColorBlendState = &color_blending,
		.pDynamicState = NULL, // Optional
		.renderPass = render->render_pass,
		.layout = render->pipeline_layout,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE, // Optional
		.basePipelineIndex = -1 // Optional
	};

	result = vkCreateGraphicsPipelines(logical_device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &pipeline);
	if (result != VK_SUCCESS) {
		vkDestroyPipelineLayout(logical_device, render->pipeline_layout, NULL);
		print_error("Failed to create graphics pipeline!");
		goto destroy_frag_module;
	}
	render->graphics_pipeline = pipeline;

	ret = 0;

destroy_frag_module:
	vkDestroyShaderModule(logical_device, frag_shader_module, NULL);
destroy_vert_module:
	vkDestroyShaderModule(logical_device, vert_shader_module, NULL);
destroy_frag_code:
	free(frag_shader_code);
destroy_vert_code:
	free(vert_shader_code);
return_error:
	return ret;
}

int
create_framebuffers(const VkDevice logical_device, struct vk_swapchain *swapchain, struct vk_render *render)
{
	VkFramebuffer *framebuffers;
	VkResult result;
	int i;

	framebuffers = malloc(sizeof(VkFramebuffer) * swapchain->images_count);
	if (!framebuffers) {
		print_error("Failed to allocated framebuffer vector!");
		return -1;
	}

	for (i = 0; i < swapchain->images_count; i++) {
		VkImageView attachments[] = { swapchain->image_views[i], render->depth_image_view };

		VkFramebufferCreateInfo framebuffer_info = {
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.renderPass = render->render_pass,
			.attachmentCount = array_size(attachments),
			.pAttachments = attachments,
			.width = swapchain->state.extent.width,
			.height = swapchain->state.extent.height,
			.layers = 1
		};

		result = vkCreateFramebuffer(logical_device, &framebuffer_info, NULL, &framebuffers[i]);
		if (result != VK_SUCCESS) {
			pprint_error("Failed to create framebuffer %u/%u!", i, swapchain->images_count);
			break;
		}
	}

	if (i != swapchain->images_count) {
		framebuffers_cleanup(logical_device, framebuffers, i - 1);
		return -1;
	}

	render->swapChain_framebuffers = framebuffers;
	render->framebuffer_count = swapchain->images_count;

	return 0;
}

void
framebuffers_cleanup(const VkDevice logical_device, VkFramebuffer *framebuffers, uint32_t size)
{
	int i;

	for (i = 0; i < size; i++)
		vkDestroyFramebuffer(logical_device, framebuffers[i], NULL);

	free(framebuffers);
}

int
create_depth_resources(struct vk_device *dev, struct vk_render *render, VkExtent2D swapchain_extent)
{
	VmaAllocation depth_image_memory;
	VkImageView depth_image_view;
	VkImage depth_image;
	int ret = -1;

	ret = create_image(dev, swapchain_extent.width, swapchain_extent.height, 1, render->depth_format,
					   VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
					   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &depth_image, &depth_image_memory);
	if (ret) {
		print_error("Failed to create depth buffer image!");
		goto return_error;
	}

	depth_image_view = create_image_view(dev->logical_device, depth_image, render->depth_format,
										 1, VK_IMAGE_ASPECT_DEPTH_BIT);
	if (depth_image_view == VK_NULL_HANDLE) {
		print_error("Failed to create depth buffer image view!");
		goto destroy_depth_image;
	}

	render->depth_image = depth_image;
	render->depth_image_view = depth_image_view;
	render->depth_image_memory = depth_image_memory;

	return 0;

destroy_depth_image:
	vmaDestroyImage(dev->mem_allocator, depth_image, depth_image_memory);
return_error:
	return ret;
}

