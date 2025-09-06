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
		renderer(ctx, window)
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

	void vulkanRenderer::changeViewPort(uint32_t width, uint32_t height)
	{
		mSwapChain.destroy();
		mSwapChain.build(width, height, mGraphicsQueueFamily);

		// reconfigure source for destroyed imageView.
		mDescriptorSetCompute.clearBindings();
		mDescriptorSetCompute.destroy();

		setBackgroundDescriptors();
	}

	void vulkanRenderer::clear(VkCommandBuffer cmd)
	{
		// bind the gradient drawing compute pipeline
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mComputePipeline.getPipeline().first);

		// bind the descriptor set containing the draw image for the compute pipeline
		auto set = mDescriptorSetCompute.getDescriptorSet().first;
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mComputePipeline.getPipeline().second, mComputeBinding.descriptorSet, 1, &set, 0, nullptr);

		// execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
		vkCmdDispatch(cmd, uint32_t(std::ceil(double(mSwapChain.getDrawImageExtent().width) / 16.0)), uint32_t(std::ceil(double(mSwapChain.getDrawImageExtent().height) / 16.0)), 1);
	}

	void vulkanRenderer::addToRender(const model& m)
	{
	}

	void vulkanRenderer::render()
	{
		auto waitResult = mSwapChain.waitOnCurrentFence();
		if (waitResult)
		{
			mErr = waitResult.err();
			return;
		}

		mSwapChain.pickImageExtent();

		// request image from the swapchain.
		// keep in mind that we use swapChain semaphore as signaling here.
		auto indexResult = mSwapChain.acquireImageIndex();
		if (!indexResult)
		{
			mErr = indexResult.err();
			return;
		}

		auto resetRes = mSwapChain.resetCommandBuffer();
		if (resetRes)
		{
			mErr = resetRes.err();
			return;
		}

		//naming it cmd for shorter writing
		VkCommandBuffer cmd = mSwapChain.getCurrentFrameData().commandBuffer;

		//begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know that
		VkCommandBufferBeginInfo cmdBeginInfo = commandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		// start recording.
		auto vkResult = vkBeginCommandBuffer(cmd, &cmdBeginInfo);
		if (vkResult != VK_SUCCESS)
		{
			mErr = vkResultToStr(vkResult);
			return;
		}

		// transition our main draw image into general layout so we can write into it
		// we will overwrite it all so we dont care about what was the older layout
		transitionImage(cmd, mSwapChain.getDrawImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

		// draw with compute, clear image.
		clear(cmd);

		transitionImage(cmd, mSwapChain.getDrawImage(), VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
		transitionImage(cmd, mSwapChain.getDepthImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

		// TODO: ADD GEOMETRY PASS.
		//draw_geometry(cmd);
		//

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
			mErr = vkResultToStr(vkResult);
			return;
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
			mErr = vkResultToStr(vkResult);
			return;
		}

		//prepare present
		// this will put the image we just rendered to into the visible window.
		// we want to wait on the _renderSemaphore for that, 
		// as its necessary that drawing commands have finished before the image is displayed to the user
		mSwapChain.present(mGraphicsQueue, indexResult.value());
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

	void vulkanRenderer::initVulkan()
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
			mErr = { inst_ret.error().message() };
			return;
		}

		vkb::Instance vkb_inst = inst_ret.value();

		mInstance = vkb_inst.instance;
		mDebugMessenger = vkb_inst.debug_messenger;

		auto surfaceResult = mWindow->makeVulkunSurface(mInstance);
		if (!surfaceResult)
		{
			mErr = surfaceResult.err();
			return;
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
			mErr = { selectedRes.error().message() };
			return;
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
			mErr = { vkResultToStr(vmaResult) };
			return;
		}

		loadExtensions();

		mDeletionQueue.push_back(destroyTask{ .type = allocator, .allocator = mAllocator });
	}

	void vulkanRenderer::setDefaultBindings()
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
			.albedoBinding = 4,
			.normalBinding = 5,
			.roughnessBinding = 6,
			.metalicBinding = 7,
			.aoBinding = 8,
		};
	}

	void vulkanRenderer::initPipelines()
	{
		initBackgroundPipeline();
		if (mErr)
			return;
	}

	void vulkanRenderer::initBackgroundPipeline()
	{
		VkShaderModule computeDrawShader;
		auto shader = mCtx->mAmanager->loadShader("../assets/shaders/vkCompiled/vkCompute.spv", this);
		if (!shader)
		{
			mErr = shader.err();
			return;
		}

		computeDrawShader = static_cast<vulkanShader*>(shader.value().get())->mShaderModule;

		mComputePipeline.init(mDevice);
		mComputePipeline.setShader(computeDrawShader);
		mErr = mComputePipeline.build(VK_NULL_HANDLE, { mDescriptorSetCompute.getDescriptorSet().second });
		if (mErr)
			return;

		mDeletionQueue.push_back(destroyTask{ .type = computePipe, .computePipe = &mComputePipeline });
	}

	void vulkanRenderer::setLimits()
	{
		VkPhysicalDeviceProperties props{};
		vkGetPhysicalDeviceProperties(mPhysicalDevice, &props);

		mPhysicalDeviceLimits.maxCombinedImageSamplers = props.limits.maxPerStageDescriptorSampledImages;
		mPhysicalDeviceLimits.maxStorageBuffers = props.limits.maxPerStageDescriptorStorageBuffers;
		mPhysicalDeviceLimits.maxFiltering = props.limits.maxSamplerAnisotropy;
	}

	void vulkanRenderer::initImmediateSubmit()
	{
		mErr = mImmediateSubmit.init(mDevice, mGraphicsQueue, mGraphicsQueueFamily);
		if (mErr)
			return;

		mDeletionQueue.push_back(destroyTask{ .type = iSub, .iSubmit = &mImmediateSubmit });
	}

	void vulkanRenderer::loadExtensions()
	{
		vkCmdDrawMeshTasksEXT = (PFN_vkCmdDrawMeshTasksEXT)vkGetDeviceProcAddr(mDevice, "vkCmdDrawMeshTasksEXT");
		if (!vkCmdDrawMeshTasksEXT)
			mErr = { "can't load extensions" };
	}

	void vulkanRenderer::initSwapchain(uint32_t width, uint32_t height)
	{
		mSwapChain.init(mAllocator, mDevice, mSurface, mPhysicalDevice);

		mErr = mSwapChain.build(width, height, mGraphicsQueueFamily);
		if (mErr)
			return;

		mDeletionQueue.push_back(destroyTask{ .type = sChain, .sChain = &mSwapChain });
	}

	void vulkanRenderer::initRegistry()
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
			mErr = samp.err();
			return;
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
	}

	void vulkanRenderer::initDescriptors()
	{
		mErr = mDescriptorSetCompute.init(mDevice, mPhysicalDevice);
		if (mErr)
			return;

		mErr = mDescriptorSetMesh.init(mDevice, mPhysicalDevice);
		if (mErr)
			return;

		mErr = mDescriptorSetPixel.init(mDevice, mPhysicalDevice);
		if (mErr)
			return;

		setBackgroundDescriptors();
		if (mErr)
			return;

		setGeometryDescriptors();
		if (mErr)
			return;

		mDeletionQueue.push_back(destroyTask{ .type = descSet, .descSet = &mDescriptorSetCompute });
		mDeletionQueue.push_back(destroyTask{ .type = descSet, .descSet = &mDescriptorSetMesh });
		mDeletionQueue.push_back(destroyTask{ .type = descSet, .descSet = &mDescriptorSetPixel });
		mDeletionQueue.push_back(destroyTask{ .type = descPool });
	}

	void vulkanRenderer::setBackgroundDescriptors()
	{
		std::vector<VkDescriptorImageInfo> imgInfo = {
			{.sampler = VK_NULL_HANDLE, .imageView = mSwapChain.getDrawImageView(), .imageLayout = VK_IMAGE_LAYOUT_GENERAL, }
		};

		mDescriptorSetCompute.addBinding(
			descriptorSet::getLayoutBindingInfo(mComputeBinding.textureBinding, uint32_t(imgInfo.size()), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		);

		mDescriptorSetCompute.addWrite(descriptorSet::getWriteInfo(mComputeBinding.textureBinding, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, imgInfo));

		mErr = mDescriptorSetCompute.build(VK_SHADER_STAGE_COMPUTE_BIT);
		if (mErr)
			return;
	}

	void vulkanRenderer::setGeometryDescriptors()
	{
		// add bindings for buffers.
		mDescriptorSetMesh.addBinding(mVertexRegistry.getLayoutBinding(mGeometryBinding.vertexBinding, mPhysicalDeviceLimits.maxStorageBuffers));
		mDescriptorSetMesh.addBinding(mIndexRegistry.getLayoutBinding(mGeometryBinding.indexBinding, mPhysicalDeviceLimits.maxStorageBuffers));
		mDescriptorSetMesh.addBinding(mPrimitiveRegistry.getLayoutBinding(mGeometryBinding.primitiveBinding, mPhysicalDeviceLimits.maxStorageBuffers));
		mDescriptorSetMesh.addBinding(mMeshletRegistry.getLayoutBinding(mGeometryBinding.meshletBinding, mPhysicalDeviceLimits.maxStorageBuffers));

		// add bindings for textures.
		mDescriptorSetMesh.addBinding(mAlbedoRegistry.getLayoutBinding(mGeometryBinding.albedoBinding, mPhysicalDeviceLimits.maxCombinedImageSamplers));
		mDescriptorSetMesh.addBinding(mNormalRegistry.getLayoutBinding(mGeometryBinding.normalBinding, mPhysicalDeviceLimits.maxCombinedImageSamplers));
		mDescriptorSetMesh.addBinding(mRoughnessRegistry.getLayoutBinding(mGeometryBinding.roughnessBinding, mPhysicalDeviceLimits.maxCombinedImageSamplers));
		mDescriptorSetMesh.addBinding(mMetalicRegistry.getLayoutBinding(mGeometryBinding.metalicBinding, mPhysicalDeviceLimits.maxCombinedImageSamplers));
		mDescriptorSetMesh.addBinding(mAoRegistry.getLayoutBinding(mGeometryBinding.aoBinding, mPhysicalDeviceLimits.maxCombinedImageSamplers));

		updateGeometryDescriptors();

		mErr = mDescriptorSetMesh.build(VK_SHADER_STAGE_MESH_BIT_EXT);
		if (mErr)
			return;

		mErr = mDescriptorSetPixel.build(VK_SHADER_STAGE_FRAGMENT_BIT);
		if (mErr)
			return;
	}

	void vulkanRenderer::updateGeometryDescriptors()
	{
		// update buffers.
		if (mVertexRegistry.needDecriptorUpdate())
		{
			mDescriptorSetMesh.clearWrites();
			mDescriptorSetMesh.addWrite(mVertexRegistry.getWriteInfo(mGeometryBinding.vertexBinding));
			mDescriptorSetMesh.updateWrite();
		}

		if (mIndexRegistry.needDecriptorUpdate())
		{
			mDescriptorSetMesh.clearWrites();
			mDescriptorSetMesh.addWrite(mIndexRegistry.getWriteInfo(mGeometryBinding.indexBinding));
			mDescriptorSetMesh.updateWrite();
		}

		if (mPrimitiveRegistry.needDecriptorUpdate())
		{
			mDescriptorSetMesh.clearWrites();
			mDescriptorSetMesh.addWrite(mPrimitiveRegistry.getWriteInfo(mGeometryBinding.primitiveBinding));
			mDescriptorSetMesh.updateWrite();
		}

		if (mMeshletRegistry.needDecriptorUpdate())
		{
			mDescriptorSetMesh.clearWrites();
			mDescriptorSetMesh.addWrite(mMeshletRegistry.getWriteInfo(mGeometryBinding.meshletBinding));
			mDescriptorSetMesh.updateWrite();
		}

		// Update textures.
		if (mAlbedoRegistry.needDecriptorUpdate())
		{
			mDescriptorSetMesh.clearWrites();
			mDescriptorSetMesh.addWrite(mAlbedoRegistry.getWriteInfo(mGeometryBinding.albedoBinding));
			mDescriptorSetMesh.updateWrite();
		}

		if (mNormalRegistry.needDecriptorUpdate())
		{
			mDescriptorSetMesh.clearWrites();
			mDescriptorSetMesh.addWrite(mNormalRegistry.getWriteInfo(mGeometryBinding.normalBinding));
			mDescriptorSetMesh.updateWrite();
		}

		if (mRoughnessRegistry.needDecriptorUpdate())
		{
			mDescriptorSetMesh.clearWrites();
			mDescriptorSetMesh.addWrite(mRoughnessRegistry.getWriteInfo(mGeometryBinding.roughnessBinding));
			mDescriptorSetMesh.updateWrite();
		}

		if (mMetalicRegistry.needDecriptorUpdate())
		{
			mDescriptorSetMesh.clearWrites();
			mDescriptorSetMesh.addWrite(mMetalicRegistry.getWriteInfo(mGeometryBinding.metalicBinding));
			mDescriptorSetMesh.updateWrite();
		}

		if (mAoRegistry.needDecriptorUpdate())
		{
			mDescriptorSetMesh.clearWrites();
			mDescriptorSetMesh.addWrite(mAoRegistry.getWriteInfo(mGeometryBinding.aoBinding));
			mDescriptorSetMesh.updateWrite();
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