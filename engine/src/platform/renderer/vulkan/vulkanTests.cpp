#include <pch.h>

#include "vulkanTests.h"
#include "platform/window/win32/window.h"

#include "VkBootstrap.h"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

namespace vkinit
{
	VkSemaphoreSubmitInfo semaphore_submit_info(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore)
	{
		VkSemaphoreSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		submitInfo.pNext = nullptr;
		submitInfo.semaphore = semaphore;
		submitInfo.stageMask = stageMask;
		submitInfo.deviceIndex = 0;
		submitInfo.value = 1;

		return submitInfo;
	}

	VkCommandBufferSubmitInfo command_buffer_submit_info(VkCommandBuffer cmd)
	{
		VkCommandBufferSubmitInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		info.pNext = nullptr;
		info.commandBuffer = cmd;
		info.deviceMask = 0;

		return info;
	}

	VkSubmitInfo2 submit_info(VkCommandBufferSubmitInfo* cmd, VkSemaphoreSubmitInfo* signalSemaphoreInfo,
		VkSemaphoreSubmitInfo* waitSemaphoreInfo)
	{
		VkSubmitInfo2 info = {};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		info.pNext = nullptr;

		info.waitSemaphoreInfoCount = waitSemaphoreInfo == nullptr ? 0 : 1;
		info.pWaitSemaphoreInfos = waitSemaphoreInfo;

		info.signalSemaphoreInfoCount = signalSemaphoreInfo == nullptr ? 0 : 1;
		info.pSignalSemaphoreInfos = signalSemaphoreInfo;

		info.commandBufferInfoCount = 1;
		info.pCommandBufferInfos = cmd;

		return info;
	}

	VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspectMask)
	{
		VkImageSubresourceRange subImage{};
		subImage.aspectMask = aspectMask;
		subImage.baseMipLevel = 0;
		subImage.levelCount = VK_REMAINING_MIP_LEVELS;
		subImage.baseArrayLayer = 0;
		subImage.layerCount = VK_REMAINING_ARRAY_LAYERS;

		return subImage;
	}

	void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout)
	{
		VkImageMemoryBarrier2 imageBarrier{ .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2 };
		imageBarrier.pNext = nullptr;

		imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
		imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

		imageBarrier.oldLayout = currentLayout;
		imageBarrier.newLayout = newLayout;

		VkImageAspectFlags aspectMask = (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
		imageBarrier.subresourceRange = vkinit::image_subresource_range(aspectMask);
		imageBarrier.image = image;

		VkDependencyInfo depInfo{};
		depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		depInfo.pNext = nullptr;

		depInfo.imageMemoryBarrierCount = 1;
		depInfo.pImageMemoryBarriers = &imageBarrier;

		vkCmdPipelineBarrier2(cmd, &depInfo);
	}

	VkCommandPoolCreateInfo command_pool_create_info(uint32_t queueFamilyIndex,
		VkCommandPoolCreateFlags flags /*= 0*/)
	{
		VkCommandPoolCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.pNext = nullptr;
		info.queueFamilyIndex = queueFamilyIndex;
		info.flags = flags;
		return info;
	}


	VkCommandBufferAllocateInfo command_buffer_allocate_info(
		VkCommandPool pool, uint32_t count /*= 1*/)
	{
		VkCommandBufferAllocateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		info.pNext = nullptr;

		info.commandPool = pool;
		info.commandBufferCount = count;
		info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		return info;
	}

	VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags /*= 0*/)
	{
		VkFenceCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		info.pNext = nullptr;

		info.flags = flags;

		return info;
	}

	VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags /*= 0*/)
	{
		VkSemaphoreCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		info.pNext = nullptr;
		info.flags = flags;
		return info;
	}

	VkCommandBufferBeginInfo command_buffer_begin_info(VkCommandBufferUsageFlags flags /*= 0*/)
	{
		VkCommandBufferBeginInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		info.pNext = nullptr;

		info.pInheritanceInfo = nullptr;
		info.flags = flags;
		return info;
	}
}

// vulkan part.
namespace vktest
{
	constexpr unsigned int FRAME_OVERLAP = 2;

	struct frameData
	{
		VkSemaphore swapchainSemaphore, renderSemaphore;
		VkFence renderFence;

		VkCommandPool commandPool;
		VkCommandBuffer mainCommandBuffer;
	};

	struct vulkanRenderer
	{
		vulkanRenderer(std::shared_ptr<engine::context> ctx) : mCtx(ctx) {};
		~vulkanRenderer();

		engine::error mErr;
		std::shared_ptr<engine::context> mCtx;

		VkInstance mInstance = VK_NULL_HANDLE;// Vulkan library handle
		VkPhysicalDevice mChosenGPU = VK_NULL_HANDLE;// GPU chosen as the default device
		VkDevice mDevice = VK_NULL_HANDLE; // Vulkan device for commands
		VkSurfaceKHR mSurface = VK_NULL_HANDLE;// Vulkan window surface
		VkDebugUtilsMessengerEXT mDebugMessenger = VK_NULL_HANDLE;// Vulkan debug output handle
		VkSwapchainKHR mSwapchain = VK_NULL_HANDLE;
		std::vector<VkImage> mSwapchainImages = {};
		std::vector<VkImageView> mSwapchainImageViews = {};
		std::unique_ptr<VkFormat> mSwapchainImageFormat = nullptr;
		std::unique_ptr<VkExtent2D> mSwapchainExtent = nullptr;

		frameData mFrames[FRAME_OVERLAP];

		frameData& getCurrentFrame() { return mFrames[mFrameNumber % FRAME_OVERLAP]; };

		VkQueue mGraphicsQueue;
		uint32_t mGraphicsQueueFamily;
		int mFrameNumber;

		void printGPU();
		void initVulkan(engine::winApiWindow* window);

		void createSwapchain(uint32_t width, uint32_t height);
		void destroySwapchain();

		void initCommands();
		void initSyncStructures();

		void draw();
	};

	std::string vkResultToStr(VkResult result)
	{
		switch (result) {
		case VK_SUCCESS: return "VK_SUCCESS";
		case VK_NOT_READY: return "VK_NOT_READY";
		case VK_TIMEOUT: return "VK_TIMEOUT";
		case VK_EVENT_SET: return "VK_EVENT_SET";
		case VK_EVENT_RESET: return "VK_EVENT_RESET";
		case VK_INCOMPLETE: return "VK_INCOMPLETE";
		case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
		case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
		case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
		case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
		case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
		case VK_ERROR_UNKNOWN: return "VK_ERROR_UNKNOWN";
		case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
		case VK_ERROR_INVALID_EXTERNAL_HANDLE: return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
		case VK_ERROR_FRAGMENTATION: return "VK_ERROR_FRAGMENTATION";
		case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS: return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
		case VK_PIPELINE_COMPILE_REQUIRED: return "VK_PIPELINE_COMPILE_REQUIRED";
		case VK_ERROR_NOT_PERMITTED: return "VK_ERROR_NOT_PERMITTED";
		case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR: return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
		case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR: return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
		case VK_ERROR_VALIDATION_FAILED_EXT: return "VK_ERROR_VALIDATION_FAILED_EXT";
		case VK_ERROR_INVALID_SHADER_NV: return "VK_ERROR_INVALID_SHADER_NV";
		case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
		case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT: return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
		case VK_ERROR_COMPRESSION_EXHAUSTED_EXT: return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
		default: return "UNKNOWN_VK_RESULT";
		}
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT              messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		const char* typeStr = (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) ? "VALIDATION" :
			(messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) ? "PERFORMANCE" : "GENERAL";

		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
			LOGERROR("[{}] {}", typeStr, pCallbackData->pMessage);
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
			LOGWARN("[{}] {}", typeStr, pCallbackData->pMessage);
		}
		else {
			LOGINFO("[{}] {}", typeStr, pCallbackData->pMessage);
		}

		return VK_FALSE;
	}

	void vulkanRenderer::createSwapchain(uint32_t width, uint32_t height)
	{
		vkb::SwapchainBuilder swapchainBuilder{ mChosenGPU, mDevice, mSurface };

		mSwapchainImageFormat = std::make_unique<VkFormat>(VK_FORMAT_B8G8R8A8_UNORM);

		auto vkbSwapchain = swapchainBuilder
			//.use_default_format_selection()
			.set_desired_format(VkSurfaceFormatKHR{ .format = (*mSwapchainImageFormat.get()), .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
			//use vsync present mode
			.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
			.set_desired_extent(width, height)
			.add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
			.build();
		if (!vkbSwapchain)
		{
			mErr = vkbSwapchain.error().message();
			return;
		}

		mSwapchainExtent = std::make_unique<VkExtent2D>(vkbSwapchain.value().extent);
		mSwapchain = vkbSwapchain.value().swapchain;
		mSwapchainImages = vkbSwapchain.value().get_images().value();
		mSwapchainImageViews = vkbSwapchain.value().get_image_views().value();
	}

	void vulkanRenderer::destroySwapchain()
	{
		vkDestroySwapchainKHR(mDevice, mSwapchain, nullptr);

		// destroy swapchain resources
		for (int i = 0; i < mSwapchainImageViews.size(); i++) {

			vkDestroyImageView(mDevice, mSwapchainImageViews[i], nullptr);
		}
	}

	void vulkanRenderer::initCommands()
	{
		//create a command pool for commands submitted to the graphics queue.
		//we also want the pool to allow for resetting of individual command buffers
		VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(mGraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

		for (int i = 0; i < FRAME_OVERLAP; i++) {

			auto result = vkCreateCommandPool(mDevice, &commandPoolInfo, nullptr, &mFrames[i].commandPool);
			if (result != VK_SUCCESS)
			{
				mErr = { vkResultToStr(result) };
				return;
			}

			// allocate the default command buffer that we will use for rendering
			VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(mFrames[i].commandPool, 1);

			result = vkAllocateCommandBuffers(mDevice, &cmdAllocInfo, &mFrames[i].mainCommandBuffer);
			if (result != VK_SUCCESS)
			{
				mErr = { vkResultToStr(result) };
				return;
			}
		}
	}

	void vulkanRenderer::initSyncStructures()
	{
		//create syncronization structures
		//one fence to control when the gpu has finished rendering the frame,
		//and 2 semaphores to syncronize rendering with swapchain
		//we want the fence to start signalled so we can wait on it on the first frame
		VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
		VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info(0);

		for (int i = 0; i < FRAME_OVERLAP; i++) {
			auto result = vkCreateFence(mDevice, &fenceCreateInfo, nullptr, &mFrames[i].renderFence);
			if (result != VK_SUCCESS)
			{
				mErr = { vkResultToStr(result) };
				return;
			}

			result = vkCreateSemaphore(mDevice, &semaphoreCreateInfo, nullptr, &mFrames[i].swapchainSemaphore);
			if (result != VK_SUCCESS)
			{
				mErr = { vkResultToStr(result) };
				return;
			}
			result = vkCreateSemaphore(mDevice, &semaphoreCreateInfo, nullptr, &mFrames[i].renderSemaphore);
			if (result != VK_SUCCESS)
			{
				mErr = { vkResultToStr(result) };
				return;
			}
		}
	}

	void vulkanRenderer::draw()
	{
		//> draw_1
		// wait until the gpu has finished rendering the last frame.Timeout of 1
		// second
		auto result = vkWaitForFences(mDevice, 1, &getCurrentFrame().renderFence, true, 1000000000);
		if (result != VK_SUCCESS)
		{
			mErr = { vkResultToStr(result) };
			return;
		}

		result = vkResetFences(mDevice, 1, &getCurrentFrame().renderFence);
		if (result != VK_SUCCESS)
		{
			mErr = { vkResultToStr(result) };
			return;
		}

		//< draw_1
		
		//> draw_2
		//request image from the swapchain
		uint32_t swapchainImageIndex;
		result = vkAcquireNextImageKHR(mDevice, mSwapchain, 1000000000, getCurrentFrame().swapchainSemaphore, nullptr, &swapchainImageIndex);
		if (result != VK_SUCCESS)
		{
			mErr = { vkResultToStr(result) };
			return;
		}
		//< draw_2

		//> draw_3
		//naming it cmd for shorter writing
		VkCommandBuffer cmd = getCurrentFrame().mainCommandBuffer;

		// now that we are sure that the commands finished executing, we can safely
		// reset the command buffer to begin recording again.
		result = vkResetCommandBuffer(cmd, 0);
		if (result != VK_SUCCESS)
		{
			mErr = { vkResultToStr(result) };
			return;
		}

		//begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
		VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		//start the command buffer recording
		result = vkBeginCommandBuffer(cmd, &cmdBeginInfo);
		if (result != VK_SUCCESS)
		{
			mErr = { vkResultToStr(result) };
			return;
		}
		//< draw_3

		//> draw_4
		// make the swapchain image into writeable mode before rendering
		vkinit::transition_image(cmd, mSwapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

		//make a clear-color from frame number. This will flash with a 120 frame period.
		VkClearColorValue clearValue;
		float flash = std::abs(std::sin(mFrameNumber / 120.f));
		clearValue = { { 0.0f, 0.0f, flash, 1.0f } };

		VkImageSubresourceRange clearRange = vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);

		//clear image
		vkCmdClearColorImage(cmd, mSwapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

		//make the swapchain image into presentable mode
		vkinit::transition_image(cmd, mSwapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		//finalize the command buffer (we can no longer add commands, but it can now be executed)
		result = vkEndCommandBuffer(cmd);
		if (result != VK_SUCCESS)
		{
			mErr = { vkResultToStr(result) };
			return;
		}
		//< draw_4

		//> draw_5
		//prepare the submission to the queue. 
		//we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
		//we will signal the _renderSemaphore, to signal that rendering has finished
		VkCommandBufferSubmitInfo cmdinfo = vkinit::command_buffer_submit_info(cmd);

		VkSemaphoreSubmitInfo waitInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, getCurrentFrame().swapchainSemaphore);
		VkSemaphoreSubmitInfo signalInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, getCurrentFrame().renderSemaphore);

		VkSubmitInfo2 submit = vkinit::submit_info(&cmdinfo, &signalInfo, &waitInfo);

		//submit command buffer to the queue and execute it.
		// _renderFence will now block until the graphic commands finish execution
		result = vkQueueSubmit2(mGraphicsQueue, 1, &submit, getCurrentFrame().renderFence);
		if (result != VK_SUCCESS)
		{
			mErr = { vkResultToStr(result) };
			return;
		}
		//< draw_5

		//> draw_6

		//prepare present
		// this will put the image we just rendered to into the visible window.
		// we want to wait on the _renderSemaphore for that, 
		// as its necessary that drawing commands have finished before the image is displayed to the user
		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = nullptr;
		presentInfo.pSwapchains = &mSwapchain;
		presentInfo.swapchainCount = 1;

		presentInfo.pWaitSemaphores = &getCurrentFrame().renderSemaphore;
		presentInfo.waitSemaphoreCount = 1;

		presentInfo.pImageIndices = &swapchainImageIndex;

		result = vkQueuePresentKHR(mGraphicsQueue, &presentInfo);
		if (result != VK_SUCCESS)
		{
			mErr = { vkResultToStr(result) };
			return;
		}
		//increase the number of frames drawn
		mFrameNumber++;
		//< draw_6
	}

	vulkanRenderer::~vulkanRenderer()
	{
		//make sure the gpu has stopped doing its things
		vkDeviceWaitIdle(mDevice);

		for (int i = 0; i < FRAME_OVERLAP; i++) {
			vkDestroyCommandPool(mDevice, mFrames[i].commandPool, nullptr);

			//destroy sync objects
			vkDestroyFence(mDevice, mFrames[i].renderFence, nullptr);
			vkDestroySemaphore(mDevice, mFrames[i].renderSemaphore, nullptr);
			vkDestroySemaphore(mDevice, mFrames[i].swapchainSemaphore, nullptr);
		}

		destroySwapchain();

		vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
		vkDestroyDevice(mDevice, nullptr);

		vkb::destroy_debug_utils_messenger(mInstance, mDebugMessenger);
		vkDestroyInstance(mInstance, nullptr);	
	}

	void vulkanRenderer::printGPU()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());

		auto deviceTypeToString = [](VkPhysicalDeviceType type) -> const char* {
			switch (type) {
			case VK_PHYSICAL_DEVICE_TYPE_OTHER:          return "Other";
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return "Integrated GPU";
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   return "Discrete GPU";
			case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    return "Virtual GPU";
			case VK_PHYSICAL_DEVICE_TYPE_CPU:            return "CPU";
			default:                                     return "Unknown";
			}
			};

		for (uint32_t i = 0; i < deviceCount; ++i)
		{
			VkPhysicalDeviceProperties props;
			vkGetPhysicalDeviceProperties(devices[i], &props);

			LOGINFO("GPU {}: {} (type = {}, api version = {}.{}.{})", i,
				props.deviceName,
				deviceTypeToString(props.deviceType),
				VK_VERSION_MAJOR(props.apiVersion),
				VK_VERSION_MINOR(props.apiVersion),
				VK_VERSION_PATCH(props.apiVersion));
		}
	}

	void vulkanRenderer::initVulkan(engine::winApiWindow* window)
	{
		// create instance.
		vkb::InstanceBuilder builder;

		//make the vulkan instance, with basic debug features
		auto inst_ret = builder.set_app_name(mCtx->config.inner.app.name.c_str())
#ifdef DEBUG
			.request_validation_layers(true)
			.set_debug_callback(debugCallback)
#endif // DEBUG
			.require_api_version(1, 3, 0)
			.build();
		if (!inst_ret)
		{
			mErr = { inst_ret.error().message() };
			return;
		}

		vkb::Instance vkb_inst = inst_ret.value();

		mInstance = vkb_inst.instance;
		mDebugMessenger = vkb_inst.debug_messenger;

		// create surface.
		VkWin32SurfaceCreateInfoKHR surfaceInfo{};
		surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surfaceInfo.hwnd = window->getHandle();
		surfaceInfo.hinstance = window->getInstance();

		VkResult result = vkCreateWin32SurfaceKHR(mInstance, &surfaceInfo, nullptr, &mSurface);
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

		// use vkbootstrap to select a gpu. 
		// We want a gpu that can write to the surface and supports vulkan 1.3 with the correct features.
		vkb::PhysicalDeviceSelector selector{ vkb_inst };

		vkb::PhysicalDevice physicalDevice = selector
			.set_minimum_version(1, 3)
			.set_required_features_13(features)
			.set_required_features_12(features12)
			.set_surface(mSurface)
			.prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
			.select()
			.value();

		//create the final vulkan device
		vkb::DeviceBuilder deviceBuilder{ physicalDevice };

		auto buildResult = deviceBuilder.build();
		if (!buildResult)
		{
			mErr = { buildResult.error().message() };
			return;
		}

		vkb::Device vkbDevice = buildResult.value();

		// Get the VkDevice handle used in the rest of a vulkan application
		mDevice = vkbDevice.device;
		mChosenGPU = physicalDevice.physical_device;

		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(physicalDevice.physical_device, &props);
		LOGINFO("Selected GPU: {}", props.deviceName);

		// use vkbootstrap to get a Graphics queue
		mGraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
		mGraphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
	}
}

// app part.
namespace vktest
{
	vulkanTest::vulkanTest(std::shared_ptr<engine::context> ctx) :
		mCtx(ctx),
		mRenderer(std::make_unique<vulkanRenderer>(ctx)),
		mWindow(nullptr)
	{
		engine::cond cv;
		mCtx->mThreadPool->start(
			[&]() -> void {
				mWindow = engine::windowFactory::createWindow(mCtx, mCtx->config.inner.wnd.name, mCtx->config.inner.wnd.width, mCtx->config.inner.wnd.height, mCtx->config.inner.wnd.isFullscreen, mCtx->config.inner.app.name, mCtx->config.inner.wnd.showCursor);
				if (mErr = mWindow->checkError(); mErr)
					return;

				cv.notifyOne();

				mWindow->startPolling();
			}
		);

		cv.wait([&] { return mWindow.get(); });

		mRenderer->initVulkan(static_cast<engine::winApiWindow*>(mWindow.get()));
		if (mRenderer->mErr)
			return;

		mRenderer->createSwapchain(mWindow->getWidth(), mWindow->getHeight());
		if (mRenderer->mErr)
			return;

		mRenderer->initCommands();
		if (mRenderer->mErr)
			return;

		mRenderer->initSyncStructures();
		if (mRenderer->mErr)
			return;
	}

	engine::error vulkanTest::checkError()
	{
		if (mRenderer->mErr)
			return mRenderer->mErr;

		return mErr;
	}

	vulkanTest::~vulkanTest()
	{
	}

	void vulkanTest::run()
	{
		while (true)
		{
			// process events.
			while (mCtx->mEventDispatcher->hasEvents())
			{
				auto event = mCtx->mEventDispatcher->getEvent();

				if (event->getEventType() == engine::close)
				{
					LOGINFO("app was closed");
					return;
				}

				if (event->getEventType() == engine::windowResize)
				{
					auto resizeEvent = static_cast<engine::windowResizeEvent*>(event.get());

					mRenderer->destroySwapchain();
					mRenderer->createSwapchain(resizeEvent->getWidth(), resizeEvent->getHeight());
					if (mErr)
					{
						LOGERROR(mErr.err());
						return;
					}
				}
			}

			// do rendering here.
			mRenderer->draw();
		}
	}

}