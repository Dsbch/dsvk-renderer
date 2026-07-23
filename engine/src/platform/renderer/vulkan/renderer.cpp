#include <pch.h>
#define VMA_IMPLEMENTATION
#include "renderer.h"
#include "deletionQueue.h"

namespace engine
{
	const uint32_t errCodeBufferOverFlow = 0;
	const uint32_t errCodeOutOfDateKHR = 1;

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT              messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		const char* typeStr = (messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT) ? "VALIDATION" :
			(messageType & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) ? "PERFORMANCE" : "GENERAL";

		if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			LOGERROR("debugCallback [{}] {}", typeStr, pCallbackData->pMessage);
		}
		else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			LOGWARN("debugCallback [{}] {}", typeStr, pCallbackData->pMessage);
		}
		else
		{
			LOGINFO("debugCallback [{}] {}", typeStr, pCallbackData->pMessage);
		}

		return VK_FALSE;
	}

	vulkanRenderer::vulkanRenderer(std::shared_ptr<context> ctx, std::shared_ptr<window> window)
		:
		renderer(ctx, window),
		mWindowMinimized(false),
		mVkCmdDrawMeshTasksEXT(nullptr),
		mProfInfo()
	{
		mErr = initVulkan();
		if (mErr)
			return;

		mErr = setLimits();
		if (mErr)
			return;

		chooseGraphicsPreset();

		mErr = initImmediateSubmit();
		if (mErr)
			return;

		mErr = initSwapchain(mWindow->getWidth(), mWindow->getHeight());
		if (mErr)
			return;

		mCtx->mAmanager->setMakeShaderFunc([&](const std::vector<uint32_t>& src) { return makeShader(src); });
		mCtx->mAmanager->setMakeTextureFunc([&](const image& img) { return makeTexture(img); });
		mCtx->mAmanager->setMakeTextureWithMipsFunc([&](const imageWithMipLevels& img) { return makeTextureWithMips(img); });

		mErr = initRenderers(window);
		if (mErr)
			return;

		mGpuProfiler.init(mDevice, mDeviceLimits);
		mErr = mGpuProfiler.createProfiling();
		if (mErr)
			return;

		mDeletionQueue.addDestroyTask(destroyTask{ .type = gpuProf, .profiler = &mGpuProfiler });
	}

	vulkanRenderer::~vulkanRenderer()
	{
		auto result = vkDeviceWaitIdle(mDevice);
		if (result != VK_SUCCESS)
			LOGERROR("~vulkanRenderer vkDeviceWaitIdle: {}", vkResultToStr(result));

		error err = mUiRenderer.destroy();
		if (err)
			LOGERROR("~vulkanRenderer mUiRenderer.destroy: {}", err.err());

		err = mMeshletRenderer.destroy();
		if (err)
			LOGERROR("~vulkanRenderer mMeshletRenderer.destroy {}", err.err());

		err = mLineRenderer.destroy();
		if (err)
			LOGERROR("~vulkanRenderer mLineRenderer.destroy {}", err.err());

		mDeletionQueue.flushDeletonQueue();
	}

	error vulkanRenderer::initVulkan()
	{
		vkb::InstanceBuilder builder;

		auto inst_ret = builder
			.set_app_name(mCtx->config.inner.app.name.c_str())
#ifdef DEBUG
			.request_validation_layers(true)
			.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT)
			// enable printf in shaders.
			//.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT)
			//.add_debug_messenger_severity(VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
			// printf in shaders end.
			.set_debug_callback(debugCallback)
#endif // DEBUG
			.require_api_version(1, 3, 0)
			.build();
		if (!inst_ret)
			return { inst_ret.error().message() };

		vkb::Instance vkb_inst = inst_ret.value();

		mInstance = vkb_inst.instance;
		mDebugMessenger = vkb_inst.debug_messenger;

		auto surfaceResult = mWindow->makeVulkunSurface(mInstance);
		if (!surfaceResult)
		{
			return surfaceResult.err();
		}

		mSurface = surfaceResult.value();

		VkPhysicalDeviceMultiviewFeaturesKHR multiviewFeatures{};
		multiviewFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES_KHR;
		multiviewFeatures.multiview = VK_TRUE; // enable base multiview
		multiviewFeatures.pNext = nullptr;

		VkPhysicalDeviceFragmentShadingRateFeaturesKHR shadingRateFeatures{};
		shadingRateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR;
		shadingRateFeatures.primitiveFragmentShadingRate = VK_TRUE;
		shadingRateFeatures.pNext = &multiviewFeatures;

		VkPhysicalDeviceMeshShaderFeaturesEXT meshShaderFeatures{};
		meshShaderFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
		meshShaderFeatures.meshShader = VK_TRUE;
		meshShaderFeatures.taskShader = VK_TRUE; // if using task shader
		meshShaderFeatures.multiviewMeshShader = VK_TRUE;
		meshShaderFeatures.primitiveFragmentShadingRateMeshShader = VK_TRUE;
		meshShaderFeatures.pNext = &shadingRateFeatures;

		// Chain to Vulkan 1.3 features
		VkPhysicalDeviceVulkan13Features features13{};
		features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		features13.dynamicRendering = VK_TRUE;
		features13.synchronization2 = VK_TRUE;
		features13.pNext = &meshShaderFeatures;

		// Vulkan 1.2 features
		VkPhysicalDeviceVulkan12Features features12{};
		features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		features12.bufferDeviceAddress = VK_TRUE;
		features12.descriptorIndexing = VK_TRUE;
		features12.runtimeDescriptorArray = VK_TRUE;
		features12.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
		features12.descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE;
		features12.descriptorBindingStorageImageUpdateAfterBind = VK_TRUE;
		features12.descriptorBindingUpdateUnusedWhilePending = VK_TRUE;
		features12.descriptorBindingPartiallyBound = VK_TRUE;
		features12.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE;
		features12.scalarBlockLayout = VK_TRUE;
		features12.uniformBufferStandardLayout = VK_TRUE;
		features12.timelineSemaphore = VK_TRUE;
		features12.pNext = &features13;

		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;
		deviceFeatures.fillModeNonSolid = VK_TRUE;
		deviceFeatures.sampleRateShading = VK_TRUE;
		deviceFeatures.shaderStorageImageMultisample = VK_TRUE;
		deviceFeatures.independentBlend = VK_TRUE;

		//use vkbootstrap to select a gpu. 
		//We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features.
		vkb::PhysicalDeviceSelector selector{ vkb_inst };
		auto selectedRes = selector
			.set_minimum_version(1, 3)
			.set_required_features_12(features12)
			.set_required_features(deviceFeatures)
			.add_required_extension(VK_EXT_MESH_SHADER_EXTENSION_NAME)
			.set_surface(mSurface)
			.select();
		if (!selectedRes)
			return selectedRes.error().message();

		vkb::PhysicalDevice physicalDevice = selectedRes.value();

		//create the final vulkan device
		vkb::DeviceBuilder deviceBuilder{ physicalDevice };

		auto buildResult = deviceBuilder.build();
		if (!buildResult.has_value())
			return buildResult.error().message();

		vkb::Device vkbDevice = buildResult.value();

		mDevice = vkbDevice.device;
		mPhysicalDevice = physicalDevice.physical_device;

		mGraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
		mGraphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = mPhysicalDevice;
		allocatorInfo.device = mDevice;
		allocatorInfo.instance = mInstance;
		allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

		auto vmaResult = vmaCreateAllocator(&allocatorInfo, &mAllocator);
		if (vmaResult != VK_SUCCESS)
			return { vkResultToStr(vmaResult) };

		loadExtensions();

		mDeletionQueue.init(mDevice);

		mDeletionQueue.addDestroyTask(destroyTask{ .type = allocator, .allocator = mAllocator });

		return {};
	}

	error vulkanRenderer::setLimits()
	{
		VkPhysicalDeviceProperties props{};
		vkGetPhysicalDeviceProperties(mPhysicalDevice, &props);

		mDeviceLimits.maxCombinedImageSamplers = props.limits.maxPerStageDescriptorSampledImages;
		mDeviceLimits.maxImage = props.limits.maxPerStageDescriptorStorageImages;
		mDeviceLimits.maxStorageBuffers = props.limits.maxPerStageDescriptorStorageBuffers;
		mDeviceLimits.maxUniformBuffers = props.limits.maxPerStageDescriptorUniformBuffers;
		mDeviceLimits.maxFiltering = props.limits.maxSamplerAnisotropy;
		mDeviceLimits.timestampPeriod = props.limits.timestampPeriod;

		VkSampleCountFlags counts = props.limits.framebufferColorSampleCounts & props.limits.framebufferDepthSampleCounts;

		if (counts & VK_SAMPLE_COUNT_64_BIT)
			mDeviceLimits.maxMultiSampling = VK_SAMPLE_COUNT_64_BIT;
		else if (counts & VK_SAMPLE_COUNT_32_BIT)
			mDeviceLimits.maxMultiSampling = VK_SAMPLE_COUNT_32_BIT;
		else if (counts & VK_SAMPLE_COUNT_16_BIT)
			mDeviceLimits.maxMultiSampling = VK_SAMPLE_COUNT_16_BIT;
		else if (counts & VK_SAMPLE_COUNT_8_BIT)
			mDeviceLimits.maxMultiSampling = VK_SAMPLE_COUNT_8_BIT;
		else if (counts & VK_SAMPLE_COUNT_4_BIT)
			mDeviceLimits.maxMultiSampling = VK_SAMPLE_COUNT_4_BIT;
		else if (counts & VK_SAMPLE_COUNT_2_BIT)
			mDeviceLimits.maxMultiSampling = VK_SAMPLE_COUNT_2_BIT;
		else
			mDeviceLimits.maxMultiSampling = VK_SAMPLE_COUNT_1_BIT;

		return {};
	}

	error vulkanRenderer::initImmediateSubmit()
	{
		error err = mSubmit.init(mCtx, mDevice, mGraphicsQueue, mGraphicsQueueFamily);
		if (err)
			return err;

		mDeletionQueue.addDestroyTask(destroyTask{ .type = iSub, .iSubmit = &mSubmit });

		return {};
	}

	error vulkanRenderer::loadExtensions()
	{
		mVkCmdDrawMeshTasksEXT = (PFN_vkCmdDrawMeshTasksEXT)vkGetDeviceProcAddr(mDevice, "vkCmdDrawMeshTasksEXT");
		if (!mVkCmdDrawMeshTasksEXT)
			return { "can't load extensions" };

		mVkCmdDrawMeshTasksIndirectEXT = (PFN_vkCmdDrawMeshTasksIndirectEXT)vkGetDeviceProcAddr(mDevice, "vkCmdDrawMeshTasksIndirectEXT");
		if (!mVkCmdDrawMeshTasksIndirectEXT)
			return { "can't load extensions" };

		return {};
	}

	error vulkanRenderer::initSwapchain(uint32_t width, uint32_t height)
	{
		mSwapChain.init(mAllocator, mDevice, mSurface, mPhysicalDevice, mSubmit, mPreset);

		error err = mSwapChain.build(mSubmit, width, height, mGraphicsQueueFamily);
		if (err)
			return err;

		mDeletionQueue.addDestroyTask(destroyTask{ .type = sChain, .sChain = &mSwapChain });

		return {};
	}

	error vulkanRenderer::initRenderers(std::shared_ptr<window> window)
	{
		// Init UBO perDrawBuffer.
		mUboPerDrawBuffer.init(mDevice, mAllocator, { true, false });

		error err = mUboPerDrawBuffer.buildAsUBO(mSubmit, nullptr, sizeof(preDrawData), 0);
		if (err)
			return err;

		mDeletionQueue.addDestroyTask(destroyTask{ .type = vulkanBuf, .vulkanBuf = &mUboPerDrawBuffer });

		err = mMeshletRenderer.init(mCtx, mVkCmdDrawMeshTasksEXT, mVkCmdDrawMeshTasksIndirectEXT, mDevice, mPhysicalDevice, mAllocator, mSubmit, mDeviceLimits, mPreset, mUboPerDrawBuffer.getBuffer().buffer, mSwapChain);
		if (err)
			return err;

		err = mLineRenderer.init(mCtx, mDevice, mPhysicalDevice, mAllocator, mSubmit, mUboPerDrawBuffer.getBuffer().buffer, mSwapChain.getDepthImageFormat(), mSwapChain.getDrawImageFormat(), mPreset);
		if (err)
			return err;

		err = mUiRenderer.init(window->getGLFWhandle(), mDevice, mPhysicalDevice, mInstance, mGraphicsQueueFamily, mGraphicsQueue, mSwapChain, mPreset);
		if (err)
			return err;

		return {};
	}

	error vulkanRenderer::updatePerDrawBuffer(renderer::renderParams in)
	{
		preDrawData data{
			.debugViewProjection = in.debugCameraProjection * in.debugCameraView,
			.useDebugCamera = in.useDebugCamera,
			.cameraFront = in.cameraFront,
			.cameraPos = in.cameraPos,
			.cameraUp = in.cameraUp,
			.view = in.view,
			.projection = in.projection,
			.viewProjection = in.projection * in.view,
			.cameraFrustum = in.cameraFrustum,
			.deltaTime = in.deltaTime,
			.width = in.width,
			.height = in.height,
		};

		mUboPerDrawBuffer.markBytesAsDead(sizeof(preDrawData));

		error err = mUboPerDrawBuffer.updateBuffer(
			mSubmit,
			&data,
			sizeof(preDrawData),
			0
		);
		if (err)
			return err;

		return {};
	}

	void vulkanRenderer::chooseGraphicsPreset()
	{
		graphicsPreset preset{
			.msaa = mCtx->config.inner.graphics.msaa,
			.anisotropicFiltering = mCtx->config.inner.graphics.anisotropicFiltering,
		};

		if (preset.anisotropicFiltering > uint32_t(mDeviceLimits.maxFiltering))
			preset.anisotropicFiltering = uint32_t(mDeviceLimits.maxFiltering);

		if (preset.msaa > sampleCountsAsUint(mDeviceLimits.maxMultiSampling))
			preset.msaa = sampleCountsAsUint(mDeviceLimits.maxMultiSampling);

		renderer::setGraphicsPreset(preset);
	}

	error vulkanRenderer::drawOpaque(VkCommandBuffer cmd, renderer::renderParams in)
	{
		error err = mLineRenderer.drawLines(
			cmd,
			mSwapChain
		);
		if (err)
			return err;

		err = mMeshletRenderer.opaquePass(
			cmd,
			in,
			mSwapChain
		);
		if (err)
			return err;

		return {};
	}

	error vulkanRenderer::drawTransperent(VkCommandBuffer cmd, renderer::renderParams in)
	{
		return mMeshletRenderer.accumilationPass(cmd, in, mSwapChain);
	}

	error vulkanRenderer::compositeOpaqueAndTransperent(VkCommandBuffer cmd, renderer::renderParams in)
	{
		return mMeshletRenderer.compositePass(cmd, in, mSwapChain);
	}

	error vulkanRenderer::drawUI(VkCommandBuffer cmd)
	{
		// Draw UI.
		return mUiRenderer.onRender(cmd, mSwapChain, mProfInfo);
	}

	std::string vulkanRenderer::getVersion() const
	{
		VkPhysicalDeviceProperties props{};
		vkGetPhysicalDeviceProperties(mPhysicalDevice, &props);

		uint32_t apiVersion = props.apiVersion;
		uint32_t major = VK_VERSION_MAJOR(apiVersion);
		uint32_t minor = VK_VERSION_MINOR(apiVersion);
		uint32_t patch = VK_VERSION_PATCH(apiVersion);

		return fmt::format("VULKAN VERSION: {}.{}.{}", major, minor, patch);
	}

	std::string vulkanRenderer::getGpuName() const
	{
		VkPhysicalDeviceProperties props{};
		vkGetPhysicalDeviceProperties(mPhysicalDevice, &props);

		return props.deviceName;
	}

	error vulkanRenderer::checkError() const
	{
		return mErr;
	}

	error vulkanRenderer::changeViewPort(uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0)
		{
			mWindowMinimized = true;
			return {};
		}
		else
		{
			mWindowMinimized = false;
		}

		mSwapChain.destroy();
		auto swapChainErr = mSwapChain.build(mSubmit, width, height, mGraphicsQueueFamily);
		if (swapChainErr)
			return swapChainErr;

		error err = mMeshletRenderer.updateSwapchainDependentDescriptors(mSwapChain);
		if (err)
			return err;

		mUiRenderer.updateSwapchainDependentDescriptors(mSwapChain);

		return {};
	}

	error vulkanRenderer::render()
	{

		return {};
	}

	error vulkanRenderer::addToRender(const model& m)
	{
		registerSceneMetrics(m);

		return mMeshletRenderer.addToRender(mDevice, mAllocator, mSubmit, mSwapChain, m);
	}

	error vulkanRenderer::updateInstance(const model& m)
	{
		return mMeshletRenderer.updateInstance(m, mSubmit);
	}

	error vulkanRenderer::updateAnimations(const model& m)
	{
		return mMeshletRenderer.updateAnimations(m, mSubmit);
	}

	void vulkanRenderer::removeFromRender(const model& m)
	{
		registerSceneMetrics(m, true);

		mMeshletRenderer.removeFromRender(m);
	}

	error vulkanRenderer::render(renderer::renderParams in)
	{
		if (mWindowMinimized)
			return {};

		error err = mLineRenderer.updateDescriptors(mAllocator, mSubmit);
		if (err)
			return err;

		err = mMeshletRenderer.updateDescriptors(in, mSubmit, mDevice, mAllocator);
		if (err)
			return err;

		// Update global UBO.
		err = updatePerDrawBuffer(in);
		if (err)
			return err;

		// Register all queued events from submit, get semaphores to wait upon before render.
		auto waitSema = mSubmit.getCurrentSemaInUse();
		std::vector<VkSubmitInfo2> commands = mSubmit.getSumbitedCommands();

		auto vkResult = vkQueueSubmit2(mGraphicsQueue, uint32_t(commands.size()), commands.data(), nullptr);
		if (vkResult != VK_SUCCESS)
			return vkResultToStr(vkResult);

		auto waitResult = mSwapChain.waitOnRenderFence();
		if (waitResult)
			return waitResult.err();

		static bool firstFrame = true;

		if (!firstFrame)
			updateProfInfo(in.deltaTime);

		firstFrame = false;

		// request image from the swapchain.
		// keep in mind that we use swapChain semaphore as signaling here.
		err = mSwapChain.acquireImageIndex();
		if (err)
		{
			if (err.is(errCodeOutOfDateKHR))
			{
				mCtx->mEventDispatcher->queueEvent(std::make_shared<windowFrameBufferResizeEvent>(mWindow->getFbWidth(), mWindow->getFbHeight()));

				return {};
			}

			return err;
		}

		mSwapChain.pickImageExtent();

		auto resetResult = mSwapChain.resetRenderFence();
		if (resetResult)
			return resetResult.err();

		resetResult = mSwapChain.resetCommandBuffer();
		if (resetResult)
			return resetResult.err();

		//naming it cmd for shorter writing
		VkCommandBuffer cmd = mSwapChain.getCommandBuffer();

		//begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
		VkCommandBufferBeginInfo cmdBeginInfo = commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		// start recording.
		vkResult = vkBeginCommandBuffer(cmd, &cmdBeginInfo);
		if (vkResult != VK_SUCCESS)
			return vkResultToStr(vkResult);

		setViewportAndSciccors(cmd, mSwapChain.getDrawImageExtent());

		mGpuProfiler.reset(cmd);

		// Cross frame barriers.
		mSwapChain.transitionDepthImage(
			cmd,
			VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
			VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
			VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT
		);

		mSwapChain.transitionDrawImage(
			cmd,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_2_TRANSFER_BIT | VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
		);
		
		mSwapChain.transitionAccumImage(
			cmd,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
		);
		
		mSwapChain.transitionRevealImage(
			cmd,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
		);
		
		mSwapChain.transitionHzbChainImages(
			cmd,
			VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			VK_ACCESS_2_SHADER_READ_BIT,
			VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, 
			VK_ACCESS_2_SHADER_WRITE_BIT
		);

		err = mGpuProfiler.beginTimeStamp(cmd, "drawOpaque");
		if (err)
			return err;

		// Begin a render pass connected to our draw image and depth buffer.
		err = drawOpaque(cmd, in);
		if (err)
			return err;

		mGpuProfiler.endTimestamp(cmd, "drawOpaque");

		err = mGpuProfiler.beginTimeStamp(cmd, "drawTransperent");

		err = drawTransperent(cmd, in);
		if (err)
			return err;

		mGpuProfiler.endTimestamp(cmd, "drawTransperent");

		// Transition to sample them as textures in composite pass.
		mSwapChain.transitionAccumImage(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		mSwapChain.transitionRevealImage(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		err = mGpuProfiler.beginTimeStamp(cmd, "compositeOpaqueAndTransperent");

		err = compositeOpaqueAndTransperent(cmd, in);
		if (err)
			return err;

		mGpuProfiler.endTimestamp(cmd, "compositeOpaqueAndTransperent");

		// Preapre images for UI render, revel and accum already transitioned to needed layoyut.
		mSwapChain.transitionDepthImage(cmd, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		mSwapChain.transitionHzbChainImages(cmd, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		err = mGpuProfiler.beginTimeStamp(cmd, "drawUI");

		err = drawUI(cmd);
		if (err)
			return err;

		mGpuProfiler.endTimestamp(cmd, "drawUI");

		// Prepare for next frame.
		mSwapChain.transitionAccumImage(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		mSwapChain.transitionRevealImage(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		mSwapChain.transitionHzbChainImages(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL);
		mSwapChain.transitionDepthImage(cmd, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		//transition the draw image and the swapchain image into their correct transfer layouts
		mSwapChain.transitionDrawImage(cmd, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		mSwapChain.transitionCurrentSwapChainImage(
			cmd,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		);

		// copy from the draw image into the swapchain
		copyImageToImage(cmd, mSwapChain.getDrawImage(mPreset.msaa > 1), mSwapChain.getCurrentSwapChainImage(), mSwapChain.getResolveImageExtent(), mSwapChain.getSwapChainExtent());

		// Transition image back to it's format.
		mSwapChain.transitionDrawImage(cmd, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		// set swapchain image layout to Attachment Optimal so we can draw it
		mSwapChain.transitionCurrentSwapChainImage(
			cmd,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		);

		// set swapchain image layout to Present so we can draw it
		mSwapChain.transitionCurrentSwapChainImage(
			cmd,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
		);

		//finalize the command buffer (we can no longer add commands, but it can now be executed)
		vkResult = vkEndCommandBuffer(cmd);
		if (vkResult != VK_SUCCESS)
			return vkResultToStr(vkResult);

		// Prepare the submission to the queue. 
		//	we want to wait on the _presentSemaphore and all semaphores that were created during resource creating, 
		//  _presentSemaphore semaphore is signaled when the swapchain is ready.
		// Remember when we ask GPU for image from swap chain we provide that semaphore to signal.
		// We will signal the _renderSemaphore, to signal that rendering has finished.
		VkCommandBufferSubmitInfo cmdinfo = commandBufferSubmitInfo(cmd);

		std::vector<VkSemaphoreSubmitInfo> waitInfo{};
		std::vector<VkSemaphoreSubmitInfo> signalInfo{};

		waitInfo.push_back(semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, mSwapChain.getSwapchainSemaphore()));
		signalInfo.push_back(semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, mSwapChain.getRenderSemaphore()));

		for (auto& sema : waitSema)
			waitInfo.push_back(semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT | VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT, sema));

		VkSubmitInfo2 submit = submitInfo(&cmdinfo, signalInfo, waitInfo);

		// submit command buffer to the queue and execute it.
		// _renderFence will now block until the graphic commands finish execution.
		vkResult = vkQueueSubmit2(mGraphicsQueue, 1, &submit, mSwapChain.getRenderFence());
		if (vkResult != VK_SUCCESS)
			return vkResultToStr(vkResult);

		// Delete all submitted commands and semaphores.
		mSubmit.deleteSemaInUse(waitSema.size());
		mSubmit.deleteSubmitedCommands(commands.size());

		// prepare present.
		// this will put the image we just rendered to into the visible window.
		// we want to wait on the _renderSemaphore for that, 
		// as its necessary that drawing commands have finished before the image is displayed to the user.
		auto presentErr = mSwapChain.present(mGraphicsQueue);
		if (presentErr)
			return presentErr;

		return {};
	}

	withError<std::shared_ptr<const shader>> vulkanRenderer::makeShader(const std::vector<uint32_t>& src)
	{
		std::shared_ptr<const shader> vkShader = std::make_shared<const vulkanShader>(mDevice, src);
		if (vkShader->checkError())
			return vkShader->checkError();

		return vkShader;
	}

	withError<std::shared_ptr<const texture>> vulkanRenderer::makeTexture(const image& img)
	{
		std::shared_ptr<const texture> vkTexture = std::make_shared<const vulkanTexture>(mDevice, mAllocator, mSubmit, img);
		if (vkTexture->checkError())
			return vkTexture->checkError();

		return vkTexture;
	}

	withError<std::shared_ptr<const texture>> vulkanRenderer::makeTextureWithMips(const imageWithMipLevels& img)
	{
		std::shared_ptr<const texture> vkTexture = std::make_shared<const vulkanTexture>(mDevice, mAllocator, mSubmit, img);
		if (vkTexture->checkError())
			return vkTexture->checkError();

		return vkTexture;
	}

	void vulkanRenderer::updateProfInfo(float deltaTime)
	{
		static uint32_t frames = 0;
		static auto lastCall = std::chrono::steady_clock::now();

		frames++;
		auto now = std::chrono::steady_clock::now();
		float elapsed = std::chrono::duration<float>(now - lastCall).count();

		if (elapsed >= 1.0f)
		{
			mProfInfo.globalInfo.fps = frames / elapsed;
			frames = 0;
			lastCall = now;
		}

		mProfInfo.globalInfo.deltaTime = deltaTime;

		mProfInfo.passInfo = mGpuProfiler.getAllSlots();
	}

	void vulkanRenderer::registerSceneMetrics(const model& m, bool isDeleted)
	{
		if (isDeleted)
		{
			for (auto& mesh : *m.meshData.get())
			{
				mProfInfo.sceneInfo.maxLodMeshlets -= mesh.meshlets.second;

				for (uint32_t i = 0; i < mesh.meshlets.second; i++)
					mProfInfo.sceneInfo.maxLodTriangles -= mesh.meshlets.data[i].triangleCount;
			}

			mProfInfo.sceneInfo.entities--;
		}
		else
		{
			for (auto& mesh : *m.meshData.get())
			{
				mProfInfo.sceneInfo.maxLodMeshlets += mesh.meshlets.second;

				for (uint32_t i = 0; i < mesh.meshlets.second; i++)
					mProfInfo.sceneInfo.maxLodTriangles += mesh.meshlets.data[i].triangleCount;
			}

			mProfInfo.sceneInfo.entities++;
		}
	}

	void vulkanRenderer::visualizeNormals(const model& m)
	{
		for (int y = 0; y < m.meshData->size(); y++)
		{
			auto perMeshAttr = m.perMeshData->operator[](y);
			auto mesh = m.meshData->operator[](y);

			for (int i = 0; i < mesh.positions.size(); i++)
			{
				glm::vec3 pos = mesh.positions[i];
				glm::vec3 normal = mesh.normal[i];

				pos = perMeshAttr.meshGlobalTransform * glm::vec4{ pos, 1.0f };
				pos = glm::vec3(
					m.instanceAttributes.modelTransform.translation + m.instanceAttributes.modelTransform.rotation * m.instanceAttributes.modelTransform.scale * pos
				);

				normal = glm::transpose(glm::inverse(glm::mat3(perMeshAttr.meshGlobalTransform))) * normal;
				normal = glm::normalize(m.instanceAttributes.modelTransform.rotation * normal);

				mLineRenderer.addLine(pos, pos + normal / 10.0f);
			}
		}
	}

	profilingInfo vulkanRenderer::getProfilingInfo()
	{
		return mProfInfo;
	}

	std::shared_ptr<renderer::renderPackage> renderer::getRenderPackage() const
	{
		return mPackage;
	}

	void renderer::renderPackage::setRenderParams(renderParams params)
	{
		std::lock_guard l{ mMu };

		mRenderCallParams = params;
	}

	void renderer::renderPackage::addEntity(const model & m)
	{
		std::lock_guard l{ mMu };

		mCurrentSceneState.addedEntities.push_back(m);
	}

	void renderer::renderPackage::deleteEntity(uint32_t id)
	{
		std::lock_guard l{ mMu };

		mCurrentSceneState.deletedEntities.insert(id);
	}

	void renderer::renderPackage::updateInstanceAttributes(const model & m)
	{
		std::lock_guard l{ mMu };

		mCurrentSceneState.updateInstanceAttributes.push_back(m);
	}

	void renderer::renderPackage::updateAnimations(const model & m)
	{
		std::lock_guard l{ mMu };

		mCurrentSceneState.updateAnimations.push_back(m);
	}

	renderer::sceneState& renderer::renderPackage::getStateToRender()
	{
		std::lock_guard l{ mMu };

		mPrevSceneState = {};

		std::swap(mPrevSceneState, mCurrentSceneState);

		return mPrevSceneState;
	}
}
