#include <pch.h>

#define VMA_IMPLEMENTATION
#include "renderer.h"

struct ComputePushConstants 
{
	glm::mat4 view;
	glm::mat4 projection;
};

namespace vktest
{
	vulkanRenderer::vulkanRenderer(std::shared_ptr<engine::context> ctx)
		:
		mCtx(ctx),
		mErr(),
		_instance(VK_NULL_HANDLE),
		_debug_messenger(VK_NULL_HANDLE),
		_chosenGPU(VK_NULL_HANDLE),
		_device(VK_NULL_HANDLE),
		_surface(VK_NULL_HANDLE),
		_frameNumber(0),
		_swapchain(),
		_swapchainImageFormat(),
		_swapchainExtent(),
		_graphicsQueue(),
		_graphicsQueueFamily(),
		mCamera(ctx, ctx->config.inner.camera.fov, ctx->config.inner.camera.nearPlane, ctx->config.inner.camera.farPlane, ctx->config.inner.wnd.width, ctx->config.inner.wnd.height),
		mGraphicsPipeline(),
		mComputePipeline()
	{};

	vulkanRenderer::~vulkanRenderer()
	{
		//make sure the gpu has stopped doing its things
		vkDeviceWaitIdle(_device);

		for (int i = 0; i < FRAME_OVERLAP; i++) {

			vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr);

			//destroy sync objects
			vkDestroyFence(_device, _frames[i]._renderFence, nullptr);
			vkDestroySemaphore(_device, _frames[i]._renderSemaphore, nullptr);
			vkDestroySemaphore(_device, _frames[i]._swapchainSemaphore, nullptr);
		}

		_mainDeletionQueue.flush();

		destroy_swapchain();

		vkDestroySurfaceKHR(_instance, _surface, nullptr);

		vkDestroyDevice(_device, nullptr);
		vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
		vkDestroyInstance(_instance, nullptr);
	}

	engine::error vulkanRenderer::checkError()
	{
		return mErr;
	}

	void vulkanRenderer::init(engine::winApiWindow* window)
	{
		init_vulkan(window);

		init_swapchain(window->getWidth(), window->getHeight());

		init_commands();

		init_sync_structures();

		init_descriptors();

		init_pipelines();
	}

	void vulkanRenderer::init_pipelines()
	{
		init_background_pipelines();
		init_triangle_pipeline();
	}

	void vulkanRenderer::init_background_pipelines()
	{
		//layout code
		VkShaderModule computeDrawShader;
		if (!vkinit::load_shader_module("../assets/shaders/vkSimple.comp.spv", _device, &computeDrawShader))
		{
			fmt::print("Error when building the compute shader \n");
		}

		mComputePipeline.setDevice(_device);
		mComputePipeline.setShader(computeDrawShader);

		mComputePipeline.buildPipeline(VK_NULL_HANDLE, { mDescriptorSet.getDescriptorSet().second });

		vkDestroyShaderModule(_device, computeDrawShader, nullptr);

		_mainDeletionQueue.push_function([&]() { mComputePipeline.destroy(); });
	}

	void vulkanRenderer::init_triangle_pipeline()
	{
		VkShaderModule triangleFragShader;
		if (!vkinit::load_shader_module("../assets/shaders/vkSimple.frag.spv", _device, &triangleFragShader)) {
			LOGERROR("Error when building the triangle fragment shader module");
		}
		else {
			LOGINFO("Triangle fragment shader succesfully loaded");
		}

		VkShaderModule triangleVertexShader;
		if (!vkinit::load_shader_module("../assets/shaders/vkSimple.vert.spv", _device, &triangleVertexShader)) {
			LOGERROR("Error when building the triangle vertex shader module");
		}
		else {
			LOGINFO("Triangle vertex shader succesfully loaded");
		}

		VkPushConstantRange pushConstant{};
		pushConstant.offset = 0;
		pushConstant.size = sizeof(ComputePushConstants);
		pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		mGraphicsPipeline.setDevice(_device);
		//connecting the vertex and pixel shaders to the pipeline
		mGraphicsPipeline.setShaders(triangleVertexShader, triangleFragShader);
		//it will draw triangles
		mGraphicsPipeline.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		//filled triangles
		mGraphicsPipeline.setPolygonMode(VK_POLYGON_MODE_FILL);
		//no backface culling
		mGraphicsPipeline.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
		//no multisampling
		mGraphicsPipeline.setMultisamplingNone();
		//no blending
		mGraphicsPipeline.disableBlending();
		//no depth testing
		mGraphicsPipeline.disableDepthtest();

		//connect the image format we will draw into, from draw image
		mGraphicsPipeline.setColorAttachmentFormat(_drawImage.imageFormat);
		mGraphicsPipeline.setDepthFormat(VK_FORMAT_UNDEFINED);

		//finally build the pipeline.
		mGraphicsPipeline.buildPipeline(&pushConstant, {});

		//clean structures.
		vkDestroyShaderModule(_device, triangleFragShader, nullptr);
		vkDestroyShaderModule(_device, triangleVertexShader, nullptr);

		_mainDeletionQueue.push_function([&]() { mGraphicsPipeline.destroy(); });
	}

	void vulkanRenderer::init_vulkan(engine::winApiWindow* window)
	{
		vkb::InstanceBuilder builder;

		//make the vulkan instance, with basic debug features
		auto inst_ret = builder.set_app_name("Example Vulkan Application")
#ifdef DEBUG
			.request_validation_layers(true)
			.set_debug_callback(debugCallback)
#endif // DEBUG
			.require_api_version(1, 3, 0)
			.build();

		vkb::Instance vkb_inst = inst_ret.value();

		//grab the instance 
		_instance = vkb_inst.instance;
		_debug_messenger = vkb_inst.debug_messenger;

		//< init_instance
		// 
		//> init_device
		// create surface.
		VkWin32SurfaceCreateInfoKHR surfaceInfo{};
		surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surfaceInfo.hwnd = window->getHandle();
		surfaceInfo.hinstance = window->getInstance();

		VkResult result = vkCreateWin32SurfaceKHR(_instance, &surfaceInfo, nullptr, &_surface);
		if (result != VK_SUCCESS) {
			mErr = { "Failed to create Win32 surface" };
			return;
		}

		// Print all gpus.
		printGPU();

		//vulkan 1.3 features
		VkPhysicalDeviceVulkan13Features features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
		features.dynamicRendering = true;
		features.synchronization2 = true;

		//vulkan 1.2 features
		VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
		features12.bufferDeviceAddress = true;
		features12.descriptorIndexing = true;


		//use vkbootstrap to select a gpu. 
		//We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
		vkb::PhysicalDeviceSelector selector{ vkb_inst };
		vkb::PhysicalDevice physicalDevice = selector
			.set_minimum_version(1, 3)
			.set_required_features_13(features)
			.set_required_features_12(features12)
			.set_surface(_surface)
			.select()
			.value();


		//create the final vulkan device
		vkb::DeviceBuilder deviceBuilder{ physicalDevice };

		vkb::Device vkbDevice = deviceBuilder.build().value();

		// Get the VkDevice handle used in the rest of a vulkan application
		_device = vkbDevice.device;
		_chosenGPU = physicalDevice.physical_device;
		//< init_device

		//> init_queue
		// use vkbootstrap to get a Graphics queue
		_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
		_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
		//< init_queue

		 // initialize the memory allocator
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = _chosenGPU;
		allocatorInfo.device = _device;
		allocatorInfo.instance = _instance;
		allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
		vmaCreateAllocator(&allocatorInfo, &_allocator);

		_mainDeletionQueue.push_function([&]() {
			vmaDestroyAllocator(_allocator);
			});
	}

	void vulkanRenderer::init_swapchain(uint32_t width, uint32_t height)
	{
		create_swapchain(width, height);

		//draw image size will match the window
		VkExtent3D drawImageExtent = {
			width,
			height,
			1
		};

		//hardcoding the draw format to 32 bit float
		_drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
		_drawImage.imageExtent = drawImageExtent;

		VkImageUsageFlags drawImageUsages{};
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
		drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		VkImageCreateInfo rimg_info = vkinit::image_create_info(_drawImage.imageFormat, drawImageUsages, drawImageExtent);

		//for the draw image, we want to allocate it from gpu local memory
		VmaAllocationCreateInfo rimg_allocinfo = {};
		rimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
		rimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		//allocate and create the image
		vmaCreateImage(_allocator, &rimg_info, &rimg_allocinfo, &_drawImage.image, &_drawImage.allocation, nullptr);

		//build a image-view for the draw image to use for rendering
		VkImageViewCreateInfo rview_info = vkinit::imageview_create_info(_drawImage.imageFormat, _drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

		VK_CHECK(vkCreateImageView(_device, &rview_info, nullptr, &_drawImage.imageView));

		//add to deletion queues
		_mainDeletionQueue.push_function([=]() {
			vkDestroyImageView(_device, _drawImage.imageView, nullptr);
			vmaDestroyImage(_allocator, _drawImage.image, _drawImage.allocation);
			});
	}

	void vulkanRenderer::init_commands()
	{
		//create a command pool for commands submitted to the graphics queue.
		//we also want the pool to allow for resetting of individual command buffers
		VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		for (int i = 0; i < FRAME_OVERLAP; i++) {

			VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));

			// allocate the default command buffer that we will use for rendering
			VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1);

			VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer));
		}
	}

	void vulkanRenderer::init_sync_structures()
	{
		//create syncronization structures
		//one fence to control when the gpu has finished rendering the frame,
		//and 2 semaphores to syncronize rendering with swapchain
		//we want the fence to start signalled so we can wait on it on the first frame
		VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
		VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info(0);

		for (int i = 0; i < FRAME_OVERLAP; i++) {
			VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frames[i]._renderFence));

			VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._swapchainSemaphore));
			VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._renderSemaphore));
		}
	}

	void vulkanRenderer::create_swapchain(uint32_t width, uint32_t height)
	{
		vkb::SwapchainBuilder swapchainBuilder{ _chosenGPU,_device,_surface };

		_swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

		vkb::Swapchain vkbSwapchain = swapchainBuilder
			//.use_default_format_selection()
			.set_desired_format(VkSurfaceFormatKHR{ .format = _swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
			//use vsync present mode
			.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
			.set_desired_extent(width, height)
			.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			.build()
			.value();

		_swapchainExtent = vkbSwapchain.extent;
		//store swapchain and its related images
		_swapchain = vkbSwapchain.swapchain;
		_swapchainImages = vkbSwapchain.get_images().value();
		_swapchainImageViews = vkbSwapchain.get_image_views().value();
	}

	void vulkanRenderer::destroy_swapchain()
	{
		vkDestroySwapchainKHR(_device, _swapchain, nullptr);

		// destroy swapchain resources
		for (int i = 0; i < _swapchainImageViews.size(); i++) {

			vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
		}
	}

	void vulkanRenderer::init_descriptors()
	{
		mDescriptorSet.setDeivce(_device);

		VkDescriptorSetLayoutBinding imageBind{};
		imageBind.binding = 0;
		imageBind.descriptorCount = 1;
		imageBind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

		VkDescriptorImageInfo imgInfo{};
		imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		imgInfo.imageView = _drawImage.imageView;

		VkWriteDescriptorSet source = {};
		source.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		source.pNext = nullptr;
		source.dstBinding = 0;
		// will be set by descriptorSet class.
		source.dstSet = nullptr;
		source.descriptorCount = 1;
		source.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		source.pImageInfo = &imgInfo;

		mDescriptorSet.addBinding(imageBind, source);
		mErr = mDescriptorSet.build(VK_SHADER_STAGE_COMPUTE_BIT);
		if (mErr)
			LOGERROR(mErr.err());

		_mainDeletionQueue.push_function([&]() { mDescriptorSet.destroy(); });
		_mainDeletionQueue.push_function([&]() { mDescriptorSet.destroyPool(); });
	}

	void vulkanRenderer::clear(VkCommandBuffer cmd)
	{
		// bind the gradient drawing compute pipeline
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mComputePipeline.getPipeline().first);

		// bind the descriptor set containing the draw image for the compute pipeline
		auto set = mDescriptorSet.getDescriptorSet().first;
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mComputePipeline.getPipeline().second, 0, 1, &set, 0, nullptr);

		// execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
		vkCmdDispatch(cmd, uint32_t(std::ceil(double(_drawExtent.width) / 16.0)), uint32_t(std::ceil(double(_drawExtent.height) / 16.0)), 1);
	}

	void vulkanRenderer::draw()
	{
		// here we pick needed height and width of our image to draw.
		_drawExtent.height = std::min(_swapchainExtent.height, _drawImage.imageExtent.height);
		_drawExtent.width = std::min(_swapchainExtent.width, _drawImage.imageExtent.width);

		//wait until the gpu has finished rendering the last frame. Timeout of 1 second
		VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._renderFence, true, 1000000000));

		// flush local deletion queue of a frame.
		get_current_frame()._deletionQueue.flush();

		//request image from the swapchain.
		// keep in mind that we use swapChain semaphore as signaling here.
		uint32_t swapchainImageIndex;
		VkResult e = vkAcquireNextImageKHR(_device, _swapchain, 1000000000, get_current_frame()._swapchainSemaphore, nullptr, &swapchainImageIndex);
		if (e == VK_ERROR_OUT_OF_DATE_KHR) {
			return;
		}

		// reset fence to reuse.
		VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._renderFence));

		//now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again.
		VK_CHECK(vkResetCommandBuffer(get_current_frame()._mainCommandBuffer, 0));

		//naming it cmd for shorter writing
		VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;

		//begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
		VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		// start recording.
		VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

		// transition our main draw image into general layout so we can write into it
		// we will overwrite it all so we dont care about what was the older layout
		vkinit::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

		// draw with compute, clear image.
		clear(cmd);

		vkinit::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		draw_geometry(cmd);

		//transition the draw image and the swapchain image into their correct transfer layouts
		vkinit::transition_image(cmd, _drawImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		vkinit::transition_image(cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		// copy from the draw image into the swapchain
		vkinit::copy_image_to_image(cmd, _drawImage.image, _swapchainImages[swapchainImageIndex], _drawExtent, _swapchainExtent);

		// set swapchain image layout to Attachment Optimal so we can draw it
		vkinit::transition_image(cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		// set swapchain image layout to Present so we can draw it
		vkinit::transition_image(cmd, _swapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		//finalize the command buffer (we can no longer add commands, but it can now be executed)
		VK_CHECK(vkEndCommandBuffer(cmd));


		//prepare the submission to the queue. 
		//we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
		// (remember when we ask GPU for image from swap chain we provide that semaphore to signal.)
		// we will signal the _renderSemaphore, to signal that rendering has finished

		VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);

		VkSemaphoreSubmitInfo waitInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, get_current_frame()._swapchainSemaphore);
		VkSemaphoreSubmitInfo signalInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, get_current_frame()._renderSemaphore);

		VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, &signalInfo, &waitInfo);

		// submit command buffer to the queue and execute it.
		// _renderFence will now block until the graphic commands finish execution
		VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, get_current_frame()._renderFence));

		//prepare present
		// this will put the image we just rendered to into the visible window.
		// we want to wait on the _renderSemaphore for that, 
		// as its necessary that drawing commands have finished before the image is displayed to the user
		VkPresentInfoKHR presentInfo = vktest::present_info();

		presentInfo.pSwapchains = &_swapchain;
		presentInfo.swapchainCount = 1;

		presentInfo.pWaitSemaphores = &get_current_frame()._renderSemaphore;
		presentInfo.waitSemaphoreCount = 1;

		presentInfo.pImageIndices = &swapchainImageIndex;

		vkQueuePresentKHR(_graphicsQueue, &presentInfo);


		//increase the number of frames drawn
		_frameNumber++;
	}

	void vulkanRenderer::resize_swapchain(uint32_t width, uint32_t height)
	{
		vkDeviceWaitIdle(_device);

		destroy_swapchain();

		create_swapchain(width, height);
	}

	void vulkanRenderer::resize(uint32_t width, uint32_t height)
	{
		resize_swapchain(width, height);
		mCamera.changeViewPort(width, height);
	}

	void vulkanRenderer::draw_geometry(VkCommandBuffer cmd)
	{
		//begin a render pass connected to our draw image.
		VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(_drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		VkRenderingInfo renderInfo = vkinit::rendering_info(_drawExtent, &colorAttachment, nullptr);
		vkCmdBeginRendering(cmd, &renderInfo);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline.getPipeline().first);

		//set dynamic viewport and scissor
		VkViewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = float(_drawExtent.width);
		viewport.height = float(_drawExtent.height);
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;

		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.width = (_drawExtent.width);
		scissor.extent.height = (_drawExtent.height);

		vkCmdSetScissor(cmd, 0, 1, &scissor);

		// set push constants.
		ComputePushConstants pc;
		pc.view = mCamera.getCameraTransform();
		pc.projection = mCamera.getProjection();

		vkCmdPushConstants(cmd, mGraphicsPipeline.getPipeline().second, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ComputePushConstants), &pc);

		//launch a draw command to draw 3 vertices
		vkCmdDraw(cmd, 3, 1, 0, 0);

		vkCmdEndRendering(cmd);
	}

	void vulkanRenderer::changeCameraPos(glm::vec3 shift)
	{
		mCamera.changePosition(shift);
	}

	void vulkanRenderer::changeYaw(float yaw)
	{
		mCamera.changeYaw(yaw);
	}

	void vulkanRenderer::changePitch(float pitch)
	{
		mCamera.changePitch(pitch);
	}
}