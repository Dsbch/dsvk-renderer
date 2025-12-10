#include <pch.h>
#include "vulkanRenderer.h"

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

	PFN_vkCmdDrawMeshTasksEXT vkCmdDrawMeshTasksEXT = nullptr;

	vulkanRenderer::vulkanRenderer(std::shared_ptr<context> ctx, std::shared_ptr<window> window)
		:
		renderer(ctx, window),
		mWindowMinimized(false)
	{
		initVulkan();
		if (mErr)
			return;

		setLimits();
		setDefaultBindings();

		initImmediateSubmit();
		if (mErr)
			return;

		initSwapchain(mWindow->getWidth(), mWindow->getHeight());
		if (mErr)
			return;

		initRegistry();
		if (mErr)
			return;

		initDescriptors();
		if (mErr)
			return;

		initPipelines();
		if (mErr)
			return;
	}

	vulkanRenderer::~vulkanRenderer()
	{
		vkDeviceWaitIdle(mDevice);

		for (auto& [k, v] : mGeometryPipelines)
		{
			v.destroy();
		}

		mGeometryPipelines.clear();

		flushDeletonQueue();
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
		auto swapChainErr = mSwapChain.build(width, height, mGraphicsQueueFamily);
		if (swapChainErr)
			return swapChainErr;

		// reconfigure source for destroyed imageView.
		mDescriptorSetCompute.clearBindings();
		mDescriptorSetCompute.destroy();

		setBackgroundDescriptors();

		return {};
	}

	void vulkanRenderer::clear(VkCommandBuffer cmd)
	{
		// bind the compute pipeline
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mComputePipeline.getPipeline().first);

		// bind the descriptor set containing the draw image for the compute pipeline
		auto set = mDescriptorSetCompute.getDescriptorSet().first;
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mComputePipeline.getPipeline().second, mComputeBinding.descriptorSet, 1, &set, 0, nullptr);

		// execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
		vkCmdDispatch(cmd, uint32_t(std::ceil(double(mSwapChain.getDrawImageExtent().width) / 16.0)), uint32_t(std::ceil(double(mSwapChain.getDrawImageExtent().height) / 16.0)), 1);
	}

	error vulkanRenderer::addToRender(model& m)
	{
		if (auto pipeData = mGeometryPipelines.find(m.mat.pixelShader); pipeData == mGeometryPipelines.end())
		{
			auto taskShader = mCtx->mAmanager->getDefaultTaskShader(this);
			if (!taskShader)
				return taskShader.err();

			auto meshShader = mCtx->mAmanager->getDefaultMeshShader(this);
			if (!meshShader)
				return meshShader.err();

			pipelineData pData;
			auto err = pData.init(
				mDevice,
				mAllocator,
				mImmediateSubmit,
				m.mat.pixelShader,
				meshShader.value(),
				taskShader.value(),
				{ mDescriptorSetMesh.getDescriptorSet().second },
				mSwapChain.getDepthImageFormat(),
				mSwapChain.getDrawImageFormat()
			);
			if (err)
				return err;

			mGeometryPipelines[m.mat.pixelShader] = std::move(pData);
		}

		// material data.
		if (m.mat.albedoTexture)
		{
			m.instanceAttributes.albedoIndex = mAlbedoRegistry.addTexture(m.mat.albedoTexture->hash(), static_cast<const vulkanTexture*>(m.mat.albedoTexture.get())->mImage);
		}

		if (m.mat.normalTexture)
		{
			m.instanceAttributes.normalIndex = mNormalRegistry.addTexture(m.mat.normalTexture->hash(), static_cast<const vulkanTexture*>(m.mat.normalTexture.get())->mImage);
		}

		if (m.mat.roughnessTexture)
		{
			m.instanceAttributes.roughnessIndex = mRoughnessRegistry.addTexture(m.mat.roughnessTexture->hash(), static_cast<const vulkanTexture*>(m.mat.roughnessTexture.get())->mImage);
		}

		if (m.mat.metalicTexture)
		{
			m.instanceAttributes.metalicIndex = mMetalicRegistry.addTexture(m.mat.metalicTexture->hash(), static_cast<const vulkanTexture*>(m.mat.metalicTexture.get())->mImage);
		}

		if (m.mat.aoTexture)
		{
			m.instanceAttributes.aoIndex = mAoRegistry.addTexture(m.mat.aoTexture->hash(), static_cast<const vulkanTexture*>(m.mat.aoTexture.get())->mImage);
		}

		// geometry data.
		// TODO: FOR NOW ONLY ONE LOD LEVEL, UPLOAD ALL LOD LEVELS.
		auto handle = mVertexRegistry.addBlock(
			m.mesh.getHash(),
			m.mesh.lodLevels[0].vertexBuffer->data(),
			m.mesh.lodLevels[0].vertexBuffer->size() * sizeof(vertex)
		);
		if (!handle)
			return handle.err();

		// For now only front.
		for (auto& meshlet : *(m.mesh.lodLevels.front().meshletBuffer.get()))
		{
			meshlet.vertexBufferOffset += handle.value().offset;
			meshlet.vertexBufferIndex = handle.value().bufferIndex;
		}

		handle = mIndexRegistry.addBlock(
			m.mesh.getHash(),
			m.mesh.lodLevels[0].indexBuffer->data(),
			m.mesh.lodLevels[0].indexBuffer->size() * sizeof(uint32_t)
		);
		if (!handle)
			return handle.err();

		// For now only front.
		for (auto& meshlet : *(m.mesh.lodLevels.front().meshletBuffer.get()))
		{
			meshlet.indexBufferOffset += handle.value().offset;
			meshlet.indexBufferIndex = handle.value().bufferIndex;
		}

		handle = mPrimitiveRegistry.addBlock(
			m.mesh.getHash(),
			m.mesh.lodLevels[0].primitiveBuffer->data(),
			m.mesh.lodLevels[0].primitiveBuffer->size() * sizeof(uint32_t)
		);
		if (!handle)
			return handle.err();

		// For now only front.
		for (auto& meshlet : *(m.mesh.lodLevels.front().meshletBuffer.get()))
		{
			meshlet.triangleBufferOffset += handle.value().offset;
			meshlet.triangleBufferIndex = handle.value().bufferIndex;
		}

		handle = mMeshletRegistry.addBlock(
			m.mesh.getHash(),
			m.mesh.lodLevels[0].meshletBuffer->data(),
			m.mesh.lodLevels[0].meshletBuffer->size() * sizeof(meshlet)
		);
		if (!handle)
			return handle.err();

		auto err = mGeometryPipelines[m.mat.pixelShader].addInstance(
			m.id,
			handle.value(),
			uint32_t(m.mesh.lodLevels[0].meshletBuffer->size()),
			m.instanceAttributes
		);
		if (err)
			return err;

		return {};
	}

	void vulkanRenderer::removeFromRender(const model& m)
	{
		if (auto pipeData = mGeometryPipelines.find(m.mat.pixelShader); pipeData == mGeometryPipelines.end())
		{
			return;
		}

		// TODO: commented code below is incorrect.
		// I need to delete texture from registry only when all models are not using that texture.
		// Also I will need to update perInstance attrs in pipelineData after delete.
		//if (m.mat.albedoTexture)
		//{
		//	mAlbedoRegistry.deleteTexture(m.instanceAttributes.matOffset.albedo);
		//}

		//if (m.mat.normalTexture)
		//{
		//	mAlbedoRegistry.deleteTexture(m.instanceAttributes.matOffset.normal);
		//}

		//if (m.mat.roughnessTexture)
		//{
		//	mAlbedoRegistry.deleteTexture(m.instanceAttributes.matOffset.roughness);
		//}

		//if (m.mat.metalicTexture)
		//{
		//	mAlbedoRegistry.deleteTexture(m.instanceAttributes.matOffset.metalic);
		//}

		//if (m.mat.aoTexture)
		//{
		//	mAlbedoRegistry.deleteTexture(m.instanceAttributes.matOffset.ao);
		//}

		mVertexRegistry.deleteBlock(m.mesh.hash);
		mIndexRegistry.deleteBlock(m.mesh.hash);
		mPrimitiveRegistry.deleteBlock(m.mesh.hash);
		mMeshletRegistry.deleteBlock(m.mesh.hash);
		mGeometryPipelines[m.mat.pixelShader].removeInstance(m.id);
	}

	error vulkanRenderer::render()
	{
		if (mWindowMinimized)
			return {};

		auto waitResult = mSwapChain.waitOnCurrentFence();
		if (waitResult)
		{
			return waitResult.err();
		}

		mSwapChain.pickImageExtent();

		// request image from the swapchain.
		// keep in mind that we use swapChain semaphore as signaling here.
		auto indexResult = mSwapChain.acquireImageIndex();
		if (!indexResult)
		{
			if (indexResult.err().err() == "VK_ERROR_OUT_OF_DATE_KHR")
			{
				mCtx->mEventDispatcher->queueEvent(std::make_shared<windowFrameBufferResizeEvent>(mWindow->getFbWidth(), mWindow->getFbHeight()));

				return {};
			}

			return indexResult.err();
		}

		auto resetResult = mSwapChain.resetCurrentFence(); if (waitResult)
			if (resetResult)
			{
				return resetResult.err();
			}

		auto resetRes = mSwapChain.resetCommandBuffer();
		if (resetRes)
		{
			return resetRes.err();
		}

		//naming it cmd for shorter writing
		VkCommandBuffer cmd = mSwapChain.getCurrentFrameData().commandBuffer;

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
		transitionImage(cmd, mSwapChain.getDrawImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

		// draw with compute, clear image.
		clear(cmd);

		transitionImage(cmd, mSwapChain.getDrawImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		transitionImage(cmd, mSwapChain.getDepthImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		auto err = geometryPass(cmd);
		if (err)
			return err;

		//transition the draw image and the swapchain image into their correct transfer layouts
		transitionImage(cmd, mSwapChain.getDrawImage(), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		transitionImage(cmd, mSwapChain.getSwapChainImages()[indexResult.value()], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		// copy from the draw image into the swapchain
		copyImageToImage(cmd, mSwapChain.getDrawImage(), mSwapChain.getSwapChainImages()[indexResult.value()], mSwapChain.getDrawImageExtent(), mSwapChain.getSwapChainExtent());

		// set swapchain image layout to Attachment Optimal so we can draw it
		transitionImage(cmd, mSwapChain.getSwapChainImages()[indexResult.value()], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		// set swapchain image layout to Present so we can draw it
		transitionImage(cmd, mSwapChain.getSwapChainImages()[indexResult.value()], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		//finalize the command buffer (we can no longer add commands, but it can now be executed)
		vkResult = vkEndCommandBuffer(cmd);
		if (vkResult != VK_SUCCESS)
		{
			return vkResultToStr(vkResult);
		}

		//prepare the submission to the queue. 
		//we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
		// (remember when we ask GPU for image from swap chain we provide that semaphore to signal.)
		// we will signal the _renderSemaphore, to signal that rendering has finished

		VkCommandBufferSubmitInfo cmdinfo = commandBufferSubmitInfo(cmd);

		VkSemaphoreSubmitInfo waitInfo = semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, mSwapChain.getCurrentFrameData().swapchainSemaphore);
		VkSemaphoreSubmitInfo signalInfo = semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, mSwapChain.getCurrentFrameData().renderSemaphore);

		VkSubmitInfo2 submit = submitInfo(&cmdinfo, &signalInfo, &waitInfo);

		// submit command buffer to the queue and execute it.
		// _renderFence will now block until the graphic commands finish execution
		vkResult = vkQueueSubmit2(mGraphicsQueue, 1, &submit, mSwapChain.getCurrentFrameData().renderFence);
		if (vkResult != VK_SUCCESS)
		{
			return vkResultToStr(vkResult);
		}

		// prepare present
		// this will put the image we just rendered to into the visible window.
		// we want to wait on the _renderSemaphore for that, 
		// as its necessary that drawing commands have finished before the image is displayed to the user
		auto presentErr = mSwapChain.present(mGraphicsQueue, indexResult.value());
		if (presentErr)
		{
			if (presentErr.err() == "VK_ERROR_OUT_OF_DATE_KHR")
			{
				mCtx->mEventDispatcher->queueEvent(std::make_shared<windowFrameBufferResizeEvent>(mWindow->getFbWidth(), mWindow->getFbHeight()));

				return {};
			}

			return presentErr;
		}

		mSwapChain.increment();

		return {};
	}

	error vulkanRenderer::geometryPass(VkCommandBuffer cmd)
	{
		updateGeometryDescriptors();

		if (mGeometryPipelines.size() != 0)
		{
			//begin a render pass connected to our draw image.
			VkRenderingAttachmentInfo colorAttachment = attachmentInfo(mSwapChain.getDrawImageView(), nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			VkRenderingAttachmentInfo depthAttachment = depthAttachmentInfo(mSwapChain.getDepthImageView(), VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

			VkRenderingInfo renderInfo = renderingInfo(mSwapChain.getDrawImageExtent(), &colorAttachment, &depthAttachment);

			vkCmdBeginRendering(cmd, &renderInfo);
		}

		error err;
		for (auto& [shader, v] : mGeometryPipelines)
		{
			err = v.updateMeshletToInstanceBuffer();
			if (err)
				return err;

			// update perInstanceRegistry for pipeline.
			// Need to update every frame for each pipeline.
			mDescriptorSetMesh.clearWrites();
			mDescriptorSetMesh.addWrite(v.getMeshletToInstanceWriteInfo(mGeometryBinding.perInstanceBinding));
			mDescriptorSetMesh.updateWrite();

			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, v.getPipeline().first);

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

			// set push constants.
			vkCmdPushConstants(cmd, v.getPipeline().second, VK_SHADER_STAGE_ALL, 0, shader->getPushConstant().size, shader->getPushConstant().data.data());

			// bind the descriptor set.
			auto set = mDescriptorSetMesh.getDescriptorSet().first;
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, v.getPipeline().second, 0, 1, &set, 0, nullptr);

			vkCmdDrawMeshTasksEXT(cmd, uint32_t(v.getTaskShaderCount()), 1, 1);
		}

		if (mGeometryPipelines.size() != 0)
			vkCmdEndRendering(cmd);

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
		std::shared_ptr<texture> vkTexture = std::make_shared<vulkanTexture>(mDevice, mAllocator, mImmediateSubmit, data, width, heigth, channel);
		if (vkTexture->checkError())
			return vkTexture->checkError();

		return vkTexture;
	}

	error vulkanRenderer::initVulkan()
	{
		vkb::InstanceBuilder builder;

		auto inst_ret = builder.set_app_name(mCtx->config.inner.app.name.c_str())
#ifdef DEBUG
			.request_validation_layers(true)
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
		features12.pNext = &features13;

		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;

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
			return { selectedRes.error().message() };
		}

		vkb::PhysicalDevice physicalDevice = selectedRes.value();

		//create the final vulkan device
		vkb::DeviceBuilder deviceBuilder{ physicalDevice };

		vkb::Device vkbDevice = deviceBuilder.build().value();

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

		mDeletionQueue.push_back(destroyTask{ .type = allocator, .allocator = mAllocator });

		return {};
	}

	error vulkanRenderer::setDefaultBindings()
	{
		mComputeBinding = computePipelineBindings{
			.descriptorSet = 0,
			.textureBinding = 0,
		};

		mGeometryBinding = geometryPipelineBindings{
			.descriptorSet = 0,
			.vertexBinding = 0,
			.indexBinding = 1,
			.primitiveBinding = 2,
			.meshletBinding = 3,
			.perInstanceBinding = 4,
			.meshletToInstanceBinding = 5,
			.albedoBinding = 6,
			.normalBinding = 7,
			.roughnessBinding = 8,
			.metalicBinding = 9,
			.aoBinding = 10,
		};

		return {};
	}

	error vulkanRenderer::initPipelines()
	{
		return initBackgroundPipeline();
	}

	error vulkanRenderer::initBackgroundPipeline()
	{
		VkShaderModule computeDrawShader;
		auto shader = mCtx->mAmanager->getDefaultComputeShader(this);
		if (!shader)
		{
			return shader.err();
		}

		computeDrawShader = static_cast<vulkanShader*>(shader.value().get())->mShaderModule;

		mComputePipeline.init(mDevice);
		mComputePipeline.setShader(computeDrawShader);

		auto buildErr = mComputePipeline.build(VK_NULL_HANDLE, { mDescriptorSetCompute.getDescriptorSet().second });
		if (buildErr)
			return buildErr;

		mDeletionQueue.push_back(destroyTask{ .type = computePipe, .computePipe = &mComputePipeline });

		return {};
	}

	error vulkanRenderer::setLimits()
	{
		VkPhysicalDeviceProperties props{};
		vkGetPhysicalDeviceProperties(mPhysicalDevice, &props);

		mPhysicalDeviceLimits.maxCombinedImageSamplers = props.limits.maxPerStageDescriptorSampledImages;
		mPhysicalDeviceLimits.maxStorageBuffers = props.limits.maxPerStageDescriptorStorageBuffers;
		mPhysicalDeviceLimits.maxFiltering = props.limits.maxSamplerAnisotropy;

		LOGINFO(
			"vulkan limits maxCombinedImageSamplers {}, maxStorageBuffers {}, maxFiltering {}",
			mPhysicalDeviceLimits.maxCombinedImageSamplers,
			mPhysicalDeviceLimits.maxStorageBuffers,
			mPhysicalDeviceLimits.maxFiltering
		);

		return {};
	}

	error vulkanRenderer::initImmediateSubmit()
	{
		auto err = mImmediateSubmit.init(mDevice, mGraphicsQueue, mGraphicsQueueFamily);
		if (err)
			return err;

		mDeletionQueue.push_back(destroyTask{ .type = iSub, .iSubmit = &mImmediateSubmit });

		return {};
	}

	error vulkanRenderer::loadExtensions()
	{
		vkCmdDrawMeshTasksEXT = (PFN_vkCmdDrawMeshTasksEXT)vkGetDeviceProcAddr(mDevice, "vkCmdDrawMeshTasksEXT");
		if (!vkCmdDrawMeshTasksEXT)
			return { "can't load extensions" };

		return {};
	}

	error vulkanRenderer::initSwapchain(uint32_t width, uint32_t height)
	{
		mSwapChain.init(mAllocator, mDevice, mSurface, mPhysicalDevice);

		auto err = mSwapChain.build(width, height, mGraphicsQueueFamily);
		if (err)
			return err;

		mDeletionQueue.push_back(destroyTask{ .type = sChain, .sChain = &mSwapChain });

		return {};
	}

	error vulkanRenderer::initRegistry()
	{
		mVertexRegistry.init(mDevice, mAllocator, mImmediateSubmit);
		mIndexRegistry.init(mDevice, mAllocator, mImmediateSubmit);
		mPrimitiveRegistry.init(mDevice, mAllocator, mImmediateSubmit);
		mMeshletRegistry.init(mDevice, mAllocator, mImmediateSubmit);

		mDeletionQueue.push_back(destroyTask{ .type = buffRegistry, .buffRegistry = &mVertexRegistry });
		mDeletionQueue.push_back(destroyTask{ .type = buffRegistry, .buffRegistry = &mIndexRegistry });
		mDeletionQueue.push_back(destroyTask{ .type = buffRegistry, .buffRegistry = &mPrimitiveRegistry });
		mDeletionQueue.push_back(destroyTask{ .type = buffRegistry, .buffRegistry = &mMeshletRegistry });

		auto samp = descriptorSet::createSampler(mDevice, mPhysicalDeviceLimits.maxFiltering);
		if (!samp)
		{
			return samp.err();
		}

		mSampler = samp.value();

		mDeletionQueue.push_back(destroyTask{ .type = sampler, .sampler = &mSampler });

		mAlbedoRegistry.init(mSampler);
		mRoughnessRegistry.init(mSampler);
		mNormalRegistry.init(mSampler);
		mMetalicRegistry.init(mSampler);
		mAoRegistry.init(mSampler);

		mDeletionQueue.push_back(destroyTask{ .type = texRegistry, .texRegistry = &mAlbedoRegistry });
		mDeletionQueue.push_back(destroyTask{ .type = texRegistry, .texRegistry = &mRoughnessRegistry });
		mDeletionQueue.push_back(destroyTask{ .type = texRegistry, .texRegistry = &mNormalRegistry });
		mDeletionQueue.push_back(destroyTask{ .type = texRegistry, .texRegistry = &mMetalicRegistry });
		mDeletionQueue.push_back(destroyTask{ .type = texRegistry, .texRegistry = &mAoRegistry });

		return {};
	}

	error vulkanRenderer::initDescriptors()
	{
		auto err = mDescriptorSetMesh.init(mDevice, mPhysicalDevice, poolConstraints{ .maxTextureDescriptors = mPhysicalDeviceLimits.maxCombinedImageSamplers, .maxStorageDescriptors = mPhysicalDeviceLimits.maxStorageBuffers });
		if (err)
			return err;

		err = mDescriptorSetCompute.init(mDevice, mPhysicalDevice);
		if (err)
			return err;

		err = setBackgroundDescriptors();
		if (err)
			return err;

		err = setGeometryDescriptors();
		if (err)
			return err;

		mDeletionQueue.push_back(destroyTask{ .type = descSet, .descSet = &mDescriptorSetCompute });
		mDeletionQueue.push_back(destroyTask{ .type = descSet, .descSet = &mDescriptorSetMesh });
		mDeletionQueue.push_back(destroyTask{ .type = descPool });

		return {};
	}

	error vulkanRenderer::setBackgroundDescriptors()
	{
		std::vector<VkDescriptorImageInfo> imgInfo = {
			{.sampler = VK_NULL_HANDLE, .imageView = mSwapChain.getDrawImageView(), .imageLayout = VK_IMAGE_LAYOUT_GENERAL, }
		};

		mDescriptorSetCompute.addBinding(
			descriptorSet::getLayoutBindingInfo(mComputeBinding.textureBinding, uint32_t(imgInfo.size()), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		);

		mDescriptorSetCompute.addWrite(descriptorSet::getWriteInfo(mComputeBinding.textureBinding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, imgInfo));

		return mDescriptorSetCompute.build(VK_SHADER_STAGE_COMPUTE_BIT);
	}

	error vulkanRenderer::setGeometryDescriptors()
	{
		// add bindings for buffers.
		mDescriptorSetMesh.addBinding(mVertexRegistry.getLayoutBinding(mGeometryBinding.vertexBinding, mPhysicalDeviceLimits.maxStorageBuffers / 6));
		mDescriptorSetMesh.addBinding(mIndexRegistry.getLayoutBinding(mGeometryBinding.indexBinding, mPhysicalDeviceLimits.maxStorageBuffers / 6));
		mDescriptorSetMesh.addBinding(mPrimitiveRegistry.getLayoutBinding(mGeometryBinding.primitiveBinding, mPhysicalDeviceLimits.maxStorageBuffers / 6));
		mDescriptorSetMesh.addBinding(mMeshletRegistry.getLayoutBinding(mGeometryBinding.meshletBinding, mPhysicalDeviceLimits.maxStorageBuffers / 6));

		// add binding for per instance attributes.
		// we have unique buffer and registry per pipeline.
		// managed differently from other bindings.
		mDescriptorSetMesh.addBinding(descriptorSet::getLayoutBindingInfo(mGeometryBinding.perInstanceBinding, mPhysicalDeviceLimits.maxStorageBuffers / 6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER));
		mDescriptorSetMesh.addBinding(descriptorSet::getLayoutBindingInfo(mGeometryBinding.meshletToInstanceBinding, mPhysicalDeviceLimits.maxStorageBuffers / 6, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER));

		// add bindings for textures.
		mDescriptorSetMesh.addBinding(mAlbedoRegistry.getLayoutBinding(mGeometryBinding.albedoBinding, mPhysicalDeviceLimits.maxCombinedImageSamplers / 5));
		mDescriptorSetMesh.addBinding(mNormalRegistry.getLayoutBinding(mGeometryBinding.normalBinding, mPhysicalDeviceLimits.maxCombinedImageSamplers / 5));
		mDescriptorSetMesh.addBinding(mRoughnessRegistry.getLayoutBinding(mGeometryBinding.roughnessBinding, mPhysicalDeviceLimits.maxCombinedImageSamplers / 5));
		mDescriptorSetMesh.addBinding(mMetalicRegistry.getLayoutBinding(mGeometryBinding.metalicBinding, mPhysicalDeviceLimits.maxCombinedImageSamplers / 5));
		mDescriptorSetMesh.addBinding(mAoRegistry.getLayoutBinding(mGeometryBinding.aoBinding, mPhysicalDeviceLimits.maxCombinedImageSamplers / 5));

		auto err = mDescriptorSetMesh.build(VK_SHADER_STAGE_ALL);
		if (err)
			return err;

		updateGeometryDescriptors();

		return {};
	}

	// Should be called before each geometry pass.
	void vulkanRenderer::updateGeometryDescriptors()
	{
		// update buffers.
		if (mVertexRegistry.needDecriptorUpdate())
		{
			mDescriptorSetMesh.clearWrites();
			mDescriptorSetMesh.addWrite(mVertexRegistry.getWriteInfo(mGeometryBinding.vertexBinding));
			mDescriptorSetMesh.updateWrite();

			mVertexRegistry.setUpdated();
		}

		if (mIndexRegistry.needDecriptorUpdate())
		{
			mDescriptorSetMesh.clearWrites();
			mDescriptorSetMesh.addWrite(mIndexRegistry.getWriteInfo(mGeometryBinding.indexBinding));
			mDescriptorSetMesh.updateWrite();

			mIndexRegistry.setUpdated();
		}

		if (mPrimitiveRegistry.needDecriptorUpdate())
		{
			mDescriptorSetMesh.clearWrites();
			mDescriptorSetMesh.addWrite(mPrimitiveRegistry.getWriteInfo(mGeometryBinding.primitiveBinding));
			mDescriptorSetMesh.updateWrite();

			mPrimitiveRegistry.setUpdated();
		}

		if (mMeshletRegistry.needDecriptorUpdate())
		{
			mDescriptorSetMesh.clearWrites();
			mDescriptorSetMesh.addWrite(mMeshletRegistry.getWriteInfo(mGeometryBinding.meshletBinding));
			mDescriptorSetMesh.updateWrite();

			mMeshletRegistry.setUpdated();
		}

		// Update textures.
		if (mAlbedoRegistry.needDecriptorUpdate())
		{
			mDescriptorSetMesh.clearWrites();
			mDescriptorSetMesh.addWrite(mAlbedoRegistry.getWriteInfo(mGeometryBinding.albedoBinding));
			mDescriptorSetMesh.updateWrite();

			mAlbedoRegistry.setUpdated();
		}

		if (mNormalRegistry.needDecriptorUpdate())
		{
			mDescriptorSetMesh.clearWrites();
			mDescriptorSetMesh.addWrite(mNormalRegistry.getWriteInfo(mGeometryBinding.normalBinding));
			mDescriptorSetMesh.updateWrite();

			mNormalRegistry.setUpdated();
		}

		if (mRoughnessRegistry.needDecriptorUpdate())
		{
			mDescriptorSetMesh.clearWrites();
			mDescriptorSetMesh.addWrite(mRoughnessRegistry.getWriteInfo(mGeometryBinding.roughnessBinding));
			mDescriptorSetMesh.updateWrite();

			mRoughnessRegistry.setUpdated();
		}

		if (mMetalicRegistry.needDecriptorUpdate())
		{
			mDescriptorSetMesh.clearWrites();
			mDescriptorSetMesh.addWrite(mMetalicRegistry.getWriteInfo(mGeometryBinding.metalicBinding));
			mDescriptorSetMesh.updateWrite();

			mMetalicRegistry.setUpdated();
		}

		if (mAoRegistry.needDecriptorUpdate())
		{
			mDescriptorSetMesh.clearWrites();
			mDescriptorSetMesh.addWrite(mAoRegistry.getWriteInfo(mGeometryBinding.aoBinding));
			mDescriptorSetMesh.updateWrite();

			mAoRegistry.setUpdated();
		}
	}

	void vulkanRenderer::flushDeletonQueue()
	{
		for (auto it = mDeletionQueue.rbegin(); it != mDeletionQueue.rend(); it++)
		{
			switch (it->type)
			{
			case allocator:
				vmaDestroyAllocator(it->allocator);
				break;
			case iSub:
				if (it->iSubmit)
					it->iSubmit->destroy();
				break;
			case sChain:
				if (it->sChain)
					it->sChain->destroy();
				break;
			case descPool:
				descriptorSet::destroyPool();
				break;
			case descSet:
				if (it->descSet)
					it->descSet->destroy();
				break;
			case computePipe:
				if (it->computePipe)
					it->computePipe->destroy();
				break;
			case buffRegistry:
				if (it->buffRegistry)
					it->buffRegistry->destroy();
				break;
			case texRegistry:
				if (it->texRegistry)
					it->texRegistry->destroy();
				break;
			case sampler:
				if (it->sampler)
					vkDestroySampler(mDevice, *it->sampler, nullptr);
				break;
			default:
				LOGERROR("unkown sampler");
			}
		}

		mDeletionQueue.clear();
	}
}