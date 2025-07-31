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
		_graphicsQueue(),
		_graphicsQueueFamily(),
		mCamera(ctx, ctx->config.inner.camera.fov, ctx->config.inner.camera.nearPlane, ctx->config.inner.camera.farPlane, ctx->config.inner.wnd.width, ctx->config.inner.wnd.height),
		mGraphicsPipeline(VK_NULL_HANDLE),
		mDescriptorSet(VK_NULL_HANDLE),
		mComputePipeline(VK_NULL_HANDLE),
		mSwapChain(VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE)
	{};

	vulkanRenderer::~vulkanRenderer()
	{
		//make sure the gpu has stopped doing its things
		vkDeviceWaitIdle(_device);

		_mainDeletionQueue.flush();

		vkDestroySurfaceKHR(_instance, _surface, nullptr);

		vkDestroyDevice(_device, nullptr);
		vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
		vkDestroyInstance(_instance, nullptr);
	}

	engine::error vulkanRenderer::checkError()
	{
		return mErr;
	}

	void vulkanRenderer::init(engine::window* window)
	{
		init_vulkan(window);

		init_swapchain(window->getWidth(), window->getHeight());

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

		mComputePipeline = { _device };
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

		mGraphicsPipeline = { _device };
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
		mGraphicsPipeline.setColorAttachmentFormat(mSwapChain.getImageFormat());
		mGraphicsPipeline.setDepthFormat(VK_FORMAT_UNDEFINED);

		//finally build the pipeline.
		mGraphicsPipeline.buildPipeline(&pushConstant, {});

		//clean structures.
		vkDestroyShaderModule(_device, triangleFragShader, nullptr);
		vkDestroyShaderModule(_device, triangleVertexShader, nullptr);

		_mainDeletionQueue.push_function([&]() { mGraphicsPipeline.destroy(); });
	}

	void vulkanRenderer::init_vulkan(engine::window* window)
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
		auto surfaceResult = window->makeVulkunSurface(_instance);
		if (!surfaceResult)
		{
			mErr = surfaceResult.err();
			return;
		}

		_surface = surfaceResult.value();

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
		mSwapChain = { _allocator, _device, _surface, _chosenGPU };

		mErr = mSwapChain.init(width, height, _graphicsQueueFamily);
		if (mErr)
			return;

		//add to deletion queues
		_mainDeletionQueue.push_function([=]() {
			mSwapChain.destroy();
			});
	}

	void vulkanRenderer::init_descriptors()
	{
		mDescriptorSet = { _device };

		set_descriptor_bindings();

		_mainDeletionQueue.push_function([&]() { mDescriptorSet.destroy(); });
		_mainDeletionQueue.push_function([&]() { mDescriptorSet.destroyPool(); });
	}

	void vulkanRenderer::set_descriptor_bindings()
	{
		VkDescriptorSetLayoutBinding imageBind{};
		imageBind.binding = 0;
		imageBind.descriptorCount = 1;
		imageBind.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

		VkDescriptorImageInfo imgInfo{};
		imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		imgInfo.imageView = mSwapChain.getDrawImage().imageView;

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
	}

	void vulkanRenderer::clear(VkCommandBuffer cmd)
	{
		// bind the gradient drawing compute pipeline
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mComputePipeline.getPipeline().first);

		// bind the descriptor set containing the draw image for the compute pipeline
		auto set = mDescriptorSet.getDescriptorSet().first;
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mComputePipeline.getPipeline().second, 0, 1, &set, 0, nullptr);

		// execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
		vkCmdDispatch(cmd, uint32_t(std::ceil(double(mSwapChain.getDrawImage().imageExtent.width) / 16.0)), uint32_t(std::ceil(double(mSwapChain.getDrawImage().imageExtent.height) / 16.0)), 1);
	}

	void vulkanRenderer::draw()
	{
		mSwapChain.pickImageExtent();

		//wait until the gpu has finished rendering the last frame. Timeout of 1 second
		VK_CHECK(vkWaitForFences(_device, 1, &mSwapChain.getCurrentFrameData().renderFence, true, 1000000000));

		//request image from the swapchain.
		// keep in mind that we use swapChain semaphore as signaling here.
		uint32_t swapchainImageIndex;
		VkResult e = vkAcquireNextImageKHR(_device, mSwapChain.getSwapChain(), 1000000000, mSwapChain.getCurrentFrameData().swapchainSemaphore, nullptr, &swapchainImageIndex);
		if (e == VK_ERROR_OUT_OF_DATE_KHR) {
			return;
		}

		// reset fence to reuse.
		VK_CHECK(vkResetFences(_device, 1, &mSwapChain.getCurrentFrameData().renderFence));

		//now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again.
		VK_CHECK(vkResetCommandBuffer(mSwapChain.getCurrentFrameData().commandBuffer, 0));

		//naming it cmd for shorter writing
		VkCommandBuffer cmd = mSwapChain.getCurrentFrameData().commandBuffer;

		//begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
		VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		// start recording.
		VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

		// transition our main draw image into general layout so we can write into it
		// we will overwrite it all so we dont care about what was the older layout
		vkinit::transition_image(cmd, mSwapChain.getDrawImage().image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

		// draw with compute, clear image.
		clear(cmd);

		vkinit::transition_image(cmd, mSwapChain.getDrawImage().image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		draw_geometry(cmd);

		//transition the draw image and the swapchain image into their correct transfer layouts
		vkinit::transition_image(cmd, mSwapChain.getDrawImage().image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		vkinit::transition_image(cmd, mSwapChain.getSwapChainImages()[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		// copy from the draw image into the swapchain
		vkinit::copy_image_to_image(cmd, mSwapChain.getDrawImage().image, mSwapChain.getSwapChainImages()[swapchainImageIndex], mSwapChain.getDrawImage().imageExtent, mSwapChain.getSwapChainExtent());

		// set swapchain image layout to Attachment Optimal so we can draw it
		vkinit::transition_image(cmd, mSwapChain.getSwapChainImages()[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		// set swapchain image layout to Present so we can draw it
		vkinit::transition_image(cmd, mSwapChain.getSwapChainImages()[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		//finalize the command buffer (we can no longer add commands, but it can now be executed)
		VK_CHECK(vkEndCommandBuffer(cmd));


		//prepare the submission to the queue. 
		//we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
		// (remember when we ask GPU for image from swap chain we provide that semaphore to signal.)
		// we will signal the _renderSemaphore, to signal that rendering has finished

		VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);

		VkSemaphoreSubmitInfo waitInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, mSwapChain.getCurrentFrameData().swapchainSemaphore);
		VkSemaphoreSubmitInfo signalInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, mSwapChain.getCurrentFrameData().renderSemaphore);

		VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, &signalInfo, &waitInfo);

		// submit command buffer to the queue and execute it.
		// _renderFence will now block until the graphic commands finish execution
		VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, mSwapChain.getCurrentFrameData().renderFence));

		//prepare present
		// this will put the image we just rendered to into the visible window.
		// we want to wait on the _renderSemaphore for that, 
		// as its necessary that drawing commands have finished before the image is displayed to the user
		VkPresentInfoKHR presentInfo = vktest::present_info();

		presentInfo.pSwapchains = &mSwapChain.getSwapChain();
		presentInfo.swapchainCount = 1;

		presentInfo.pWaitSemaphores = &mSwapChain.getCurrentFrameData().renderSemaphore;
		presentInfo.waitSemaphoreCount = 1;

		presentInfo.pImageIndices = &swapchainImageIndex;

		vkQueuePresentKHR(_graphicsQueue, &presentInfo);


		//increase the number of frames drawn
		mSwapChain.inrement();
	}

	void vulkanRenderer::resize(uint32_t width, uint32_t height)
	{
		mSwapChain.resize(width, height);

		// reconfigure source for destroyed imageView.
		mDescriptorSet.clearBindings();
		mDescriptorSet.destroy();
		set_descriptor_bindings();

		mCamera.changeViewPort(width, height);
	}

	void vulkanRenderer::draw_geometry(VkCommandBuffer cmd)
	{
		//begin a render pass connected to our draw image.
		VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(mSwapChain.getDrawImage().imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		VkRenderingInfo renderInfo = vkinit::rendering_info(mSwapChain.getDrawImage().imageExtent, &colorAttachment, nullptr);
		vkCmdBeginRendering(cmd, &renderInfo);

		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline.getPipeline().first);

		//set dynamic viewport and scissor
		VkViewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = float(mSwapChain.getDrawImage().imageExtent.width);
		viewport.height = float(mSwapChain.getDrawImage().imageExtent.height);
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;

		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.width = (mSwapChain.getDrawImage().imageExtent.width);
		scissor.extent.height = (mSwapChain.getDrawImage().imageExtent.height);

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