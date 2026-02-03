#include <pch.h>
#define VMA_IMPLEMENTATION
#include "renderer.h"
#include "deletionQueue.h"

namespace engine
{
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
		else
			if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
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
		mVkCmdDrawMeshTasksEXT(nullptr)
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
		mCtx->mAmanager->setMakeTextureFunc([&](uint8_t* data, int width, int heigth, imageChannel channel) { return makeTexture(data, width, heigth, channel); });

		mErr = mUi.init(window->getGLFWhandle(), mDevice, mPhysicalDevice, mInstance, mGraphicsQueueFamily, mGraphicsQueue, mSwapChain.getDrawImageFormat());
		if (mErr)
			return;

		mErr = initRenderers();
		if (mErr)
			return;
	}

	vulkanRenderer::~vulkanRenderer()
	{
		auto result = vkDeviceWaitIdle(mDevice);
		if (result != VK_SUCCESS)
			LOGERROR("~vulkanRenderer vkDeviceWaitIdle: {}", vkResultToStr(result));

		error err = mUi.destroy();
		if (err)
			LOGERROR("~vulkanRenderer mUi.destroy: {}", err.err());

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

		auto inst_ret = builder.set_app_name(mCtx->config.inner.app.name.c_str())
#ifdef DEBUG
			.request_validation_layers(true)
			// enable printf in shaders.
			//.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT)
			//.add_debug_messenger_severity(VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
			// printf in shaders end.
			.set_debug_callback(debugCallback)
#endif // DEBUG
			.require_api_version(1, 3, 0)
			.build();
		if (!inst_ret)
		{
			return { inst_ret.error().message() };
		}

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

		//use vkbootstrap to select a gpu. 
		//We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features
		vkb::PhysicalDeviceSelector selector{ vkb_inst };
		auto selectedRes = selector
			.set_minimum_version(1, 3)
			.set_required_features_12(features12)
			.set_required_features(deviceFeatures)
			.add_required_extension(VK_EXT_MESH_SHADER_EXTENSION_NAME)
			.set_surface(mSurface)
			.select();
		if (!selectedRes)
		{
			return selectedRes.error().message();
		}

		vkb::PhysicalDevice physicalDevice = selectedRes.value();

		//create the final vulkan device
		vkb::DeviceBuilder deviceBuilder{ physicalDevice };

		auto buildResult = deviceBuilder.build();
		if (!buildResult.has_value())
		{
			return buildResult.error().message();
		}

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
		{
			return { vkResultToStr(vmaResult) };
		}

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

		return {};
	}

	error vulkanRenderer::initSwapchain(uint32_t width, uint32_t height)
	{
		mSwapChain.init(mAllocator, mDevice, mSurface, mPhysicalDevice);

		error err = mSwapChain.build(width, height, mGraphicsQueueFamily, mPreset);
		if (err)
			return err;

		mDeletionQueue.addDestroyTask(destroyTask{ .type = sChain, .sChain = &mSwapChain });

		return {};
	}

	error vulkanRenderer::initRenderers()
	{
		// Init UBO perDrawBuffer.
		mUboPerDrawBuffer.init(mDevice, mAllocator);

		error err = mUboPerDrawBuffer.buildAsUBO(mSubmit, nullptr, sizeof(preDrawData), 0);
		if (err)
			return err;

		mDeletionQueue.addDestroyTask(destroyTask{ .type = vulkanBuf, .vulkanBuf = &mUboPerDrawBuffer });

		err = mMeshletRenderer.init(mCtx, mVkCmdDrawMeshTasksEXT, mDevice, mPhysicalDevice, mAllocator, mSubmit, mDeviceLimits, mPreset, mUboPerDrawBuffer.getBuffer().buffer);
		if (err)
			return err;

		err = mLineRenderer.init(mCtx, mDevice, mPhysicalDevice, mAllocator, mSubmit, mUboPerDrawBuffer.getBuffer().buffer, mSwapChain.getDepthImageFormat(), mSwapChain.getDrawImageFormat(), mPreset);
		if (err)
			return err;

		return {};
	}

	error vulkanRenderer::updatePerDrawBuffer(renderer::renderCallIn in)
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
		auto swapChainErr = mSwapChain.build(width, height, mGraphicsQueueFamily, mPreset);
		if (swapChainErr)
			return swapChainErr;

		return {};
	}

	error vulkanRenderer::addToRender(const model& m)
	{
		//glm::vec3 lightPositions[4] =
		//{
		//	glm::vec3(0.0f, 0.0f, 2.0f),
		//	glm::vec3(0.0f, 0.0f, -2.0f),
		//	glm::vec3(2.0f, 0.0f, 0.0f),
		//	glm::vec3(-2.0f, 0.0f, 0.0f),
		//};

		/*for (auto& p : lightPositions)
		{
			mLineRenderer.addLine(p, m.instanceAttributes.bsWorldCenter);
		}*/

		//for (auto& v : *m.meshData.vertex.get())
		//{
		//	glm::vec3 pos = glm::vec3(m.instanceAttributes.modelMatrix * glm::vec4(v.position, 1.0f));

		//	mLineRenderer.addLine(pos, pos + v.normal);
		//}

		return mMeshletRenderer.addToRender(mDevice, mSubmit, mSwapChain.getDepthImageFormat(), mSwapChain.getDrawImageFormat(), m);
	}

	error vulkanRenderer::updateInstance(const model& m)
	{
		return mMeshletRenderer.updateInstance(m, mSubmit);
	}

	void vulkanRenderer::removeFromRender(const model& m)
	{
		mMeshletRenderer.removeFromRender(m);
	}

	error vulkanRenderer::render(renderer::renderCallIn in)
	{
		if (mWindowMinimized)
			return {};

		error err = mLineRenderer.updateDescriptors(mAllocator, mSubmit);
		if (err)
			return err;

		err = mMeshletRenderer.updateDescriptors(in, mSubmit);
		if (err)
			return err;

		// Update global UBO.
		err = updatePerDrawBuffer(in);
		if (err)
			return err;

		auto waitResult = mSwapChain.waitOnRenderFence();
		if (waitResult)
			return waitResult.err();

		// request image from the swapchain.
		// keep in mind that we use swapChain semaphore as signaling here.
		err = mSwapChain.acquireImageIndex();
		if (err)
		{
			if (err.err() == "VK_ERROR_OUT_OF_DATE_KHR")
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
		auto vkResult = vkBeginCommandBuffer(cmd, &cmdBeginInfo);
		if (vkResult != VK_SUCCESS)
		{
			return vkResultToStr(vkResult);
		}

		// transition our main draw image into general layout so we can write into it
		// we will overwrite it all so we dont care about what was the older layout
		//transitionImage(cmd, mSwapChain.getDrawImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

		transitionImage(cmd, mSwapChain.getDrawImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		transitionImage(cmd, mSwapChain.getDepthImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
		transitionImage(cmd, mSwapChain.getResolveImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		// Geometry pass START.
		// Begin a render pass connected to our draw image and depth buffer.

		VkClearValue clear{
			.color = VkClearColorValue{.float32 = { 0.0f, 0.0f, 0.0f, 0.0f} },
		};

		VkRenderingAttachmentInfo colorAttachment = attachmentInfo(mSwapChain.getDrawImageView(), mPreset.msaa <= 1 ? nullptr : mSwapChain.getResolveImageView(), getResolveMode(mPreset.msaa), &clear, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		VkRenderingAttachmentInfo depthAttachment = depthAttachmentInfo(mSwapChain.getDepthImageView(), VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
		VkRenderingInfo renderInfo = renderingInfo(mSwapChain.getDrawImageExtent(), &colorAttachment, &depthAttachment);

		vkCmdBeginRendering(cmd, &renderInfo);

		//set dynamic viewport and scissor
		VkViewport viewport = {};
		viewport.x = 0;
		viewport.y = 0;
		viewport.width = float(mSwapChain.getDrawImageExtent().width);
		viewport.height = float(mSwapChain.getDrawImageExtent().height);
		viewport.minDepth = 0.f;
		viewport.maxDepth = 1.f;

		vkCmdSetViewport(cmd, 0, 1, &viewport);

		VkRect2D scissor = {};
		scissor.offset.x = 0;
		scissor.offset.y = 0;
		scissor.extent.width = (mSwapChain.getDrawImageExtent().width);
		scissor.extent.height = (mSwapChain.getDrawImageExtent().height);

		vkCmdSetScissor(cmd, 0, 1, &scissor);

		err = mLineRenderer.drawLines(cmd);
		if (err)
			return err;

		err = mMeshletRenderer.geometryPass(cmd, in);
		if (err)
			return err;

		vkCmdEndRendering(cmd);
		// Geometry pass END.

		// UI pass START.
		// Imgui can't work with msaa color attachments.
		colorAttachment = attachmentInfo(mPreset.msaa <= 1 ? mSwapChain.getDrawImageView() : mSwapChain.getResolveImageView(), nullptr, VK_RESOLVE_MODE_NONE, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		renderInfo = renderingInfo(mSwapChain.getDrawImageExtent(), &colorAttachment, nullptr);

		vkCmdBeginRendering(cmd, &renderInfo);

		// Render UI.
		mUi.onRender(cmd);

		vkCmdEndRendering(cmd);
		// UI pass end.

		//transition the resolve image and the swapchain image into their correct transfer layouts
		transitionImage(cmd, mPreset.msaa <= 1 ? mSwapChain.getDrawImage() : mSwapChain.getResolveImage(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		transitionImage(cmd, mSwapChain.getCurrentSwapChainImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		// copy from the resolve image into the swapchain
		copyImageToImage(cmd, mPreset.msaa <= 1 ? mSwapChain.getDrawImage() : mSwapChain.getResolveImage(), mSwapChain.getCurrentSwapChainImage(), mSwapChain.getResolveImageExtent(), mSwapChain.getSwapChainExtent());

		// set swapchain image layout to Attachment Optimal so we can draw it
		transitionImage(cmd, mSwapChain.getCurrentSwapChainImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		// set swapchain image layout to Present so we can draw it
		transitionImage(cmd, mSwapChain.getCurrentSwapChainImage(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		//finalize the command buffer (we can no longer add commands, but it can now be executed)
		vkResult = vkEndCommandBuffer(cmd);
		if (vkResult != VK_SUCCESS)
		{
			return vkResultToStr(vkResult);
		}

		// Prepare the submission to the queue. 
		//	we want to wait on the _presentSemaphore and all semaphores that were created during resource creating, 
		//  _presentSemaphore semaphore is signaled when the swapchain is ready.
		// Remember when we ask GPU for image from swap chain we provide that semaphore to signal.
		// We will signal the _renderSemaphore, to signal that rendering has finished
		VkCommandBufferSubmitInfo cmdinfo = commandBufferSubmitInfo(cmd);

		std::vector<VkSemaphoreSubmitInfo> waitInfo{};
		std::vector<VkSemaphoreSubmitInfo> signalInfo;

		waitInfo.push_back(semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, mSwapChain.getSwapchainSemaphore()));
		signalInfo.push_back(semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, mSwapChain.getRenderSemaphore()));

		auto waitSema = mSubmit.getCurrentSemaInUse();
		for (auto& sema : waitSema)
			waitInfo.push_back(semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT | VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT, sema));

		VkSubmitInfo2 submit = submitInfo(&cmdinfo, signalInfo, waitInfo);

		// submit command buffer to the queue and execute it.
		// _renderFence will now block until the graphic commands finish execution
		vkResult = vkQueueSubmit2(mGraphicsQueue, 1, &submit, mSwapChain.getRenderFence());
		if (vkResult != VK_SUCCESS)
		{
			return vkResultToStr(vkResult);
		}

		mSubmit.markAllSemaAsUsed();

		// prepare present
		// this will put the image we just rendered to into the visible window.
		// we want to wait on the _renderSemaphore for that, 
		// as its necessary that drawing commands have finished before the image is displayed to the user
		auto presentErr = mSwapChain.present(mGraphicsQueue);
		if (presentErr)
		{
			if (presentErr.err() == "VK_ERROR_OUT_OF_DATE_KHR")
			{
				mCtx->mEventDispatcher->queueEvent(std::make_shared<windowFrameBufferResizeEvent>(mWindow->getFbWidth(), mWindow->getFbHeight()));

				return {};
			}

			return presentErr;
		}

		return {};
	}

	withError<std::shared_ptr<shader>> vulkanRenderer::makeShader(const std::vector<uint32_t>& src)
	{
		std::shared_ptr<shader> vkShader = std::make_shared<vulkanShader>(mDevice, src);
		if (vkShader->checkError())
			return vkShader->checkError();

		return vkShader;
	}

	withError<std::shared_ptr<texture>> vulkanRenderer::makeTexture(uint8_t* data, int width, int heigth, imageChannel channel)
	{
		std::shared_ptr<texture> vkTexture = std::make_shared<vulkanTexture>(mDevice, mAllocator, mSubmit, data, width, heigth, channel);
		if (vkTexture->checkError())
			return vkTexture->checkError();

		return vkTexture;
	}
}
