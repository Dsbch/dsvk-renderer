#include <pch.h>
#define VMA_IMPLEMENTATION
#include "renderer.h"

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
			LOGERROR("[{}] {}", typeStr, pCallbackData->pMessage);
		}
		else
			if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
			{
				LOGWARN("[{}] {}", typeStr, pCallbackData->pMessage);
			}
			else
			{
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
		mErr = initVulkan();
		if (mErr)
			return;

		mErr = setLimits();
		mErr = setDefaultBindings();

		mErr = initImmediateSubmit();
		if (mErr)
			return;

		mErr = initSwapchain(mWindow->getWidth(), mWindow->getHeight());
		if (mErr)
			return;

		mErr = initRegistry();
		if (mErr)
			return;

		mCtx->mAmanager->setMakeShaderFunc([&](const std::vector<uint32_t>& src) { return makeShader(src); });
		mCtx->mAmanager->setMakeTextureFunc([&](uint8_t* data, int width, int heigth, imageChannel channel) { return makeTexture(data, width, heigth, channel); });

		mErr = initDescriptors();
		if (mErr)
			return;

		mErr = initPipelines();
		if (mErr)
			return;

	}

	vulkanRenderer::~vulkanRenderer()
	{
		auto result = vkDeviceWaitIdle(mDevice);
		if (result != VK_SUCCESS)
		{
			LOGERROR(vkResultToStr(result));
			return;
		}

		flushDeletonQueue();
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

		mDeletionQueue.push_back(destroyTask{ .type = allocator, .allocator = mAllocator });

		return {};
	}

	error vulkanRenderer::setDefaultBindings()
	{
		mComputeBinding = computePipelineBindings{
			.descriptorSet = 0,
			.totalDescriptorsCount = 1,
			.colorAttachment = 0,
		};

		mGeometryBinding = geometryPipelineBindings{
			.descriptorSet = 0,
			.totalDescriptorsCount = 10,

			.vertexBinding = 0,
			.perInstanceBinding = 1,
			.meshletCmdBinding = 2,
			.indexBinding = 3,
			.primitiveBinding = 4,
			.meshletBinding = 5,

			.perDrawBufferUboBinding = 6,

			.albedoBinding = 7,
			.normalBinding = 8,
			.metalicRoughnesBinding = 9,
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
		auto shader = mCtx->mAmanager->getDefaultComputeShader();
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
		mPhysicalDeviceLimits.maxImage = props.limits.maxPerStageDescriptorStorageImages;
		mPhysicalDeviceLimits.maxStorageBuffers = props.limits.maxPerStageDescriptorStorageBuffers;
		mPhysicalDeviceLimits.maxUniformBuffers = props.limits.maxPerStageDescriptorUniformBuffers;
		mPhysicalDeviceLimits.maxFiltering = props.limits.maxSamplerAnisotropy;

		return {};
	}

	error vulkanRenderer::initImmediateSubmit()
	{
		error err = mSubmit.init(mCtx, mDevice, mGraphicsQueue, mGraphicsQueueFamily);
		if (err)
			return err;

		mDeletionQueue.push_back(destroyTask{ .type = iSub, .iSubmit = &mSubmit });

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

		error err = mSwapChain.build(width, height, mGraphicsQueueFamily);
		if (err)
			return err;

		mDeletionQueue.push_back(destroyTask{ .type = sChain, .sChain = &mSwapChain });

		return {};
	}

	error vulkanRenderer::initRegistry()
	{
		mVertexRegistry.init(mDevice, mAllocator, mSubmit);

		mIndexRegistry.init(mDevice, mAllocator, mSubmit);

		mPrimitiveRegistry.init(mDevice, mAllocator, mSubmit);

		mMeshletRegistry.init(mDevice, mAllocator, mSubmit);

		mPerInstanceRegistry.init(mDevice, mAllocator, mSubmit);

		error err = mPipelineRegistry.init(mDevice, mAllocator, mSubmit);
		if (err)
			return err;

		mDeletionQueue.push_back(destroyTask{ .type = buffRegistry, .buffRegistry = &mVertexRegistry });

		mDeletionQueue.push_back(destroyTask{ .type = buffRegistry, .buffRegistry = &mIndexRegistry });

		mDeletionQueue.push_back(destroyTask{ .type = buffRegistry, .buffRegistry = &mPrimitiveRegistry });

		mDeletionQueue.push_back(destroyTask{ .type = buffRegistry, .buffRegistry = &mMeshletRegistry });

		mDeletionQueue.push_back(destroyTask{ .type = buffRegistry, .buffRegistry = &mPerInstanceRegistry });

		mDeletionQueue.push_back(destroyTask{ .type = pipelineReg, .pipelineReg = &mPipelineRegistry });

		auto samp = descriptorSet::createSampler(mDevice, mPhysicalDeviceLimits.maxFiltering);
		if (!samp)
		{
			return samp.err();
		}

		mSampler = samp.value();

		mDeletionQueue.push_back(destroyTask{ .type = sampler, .sampler = &mSampler });

		mAlbedoRegistry.init(mSampler);
		mMetalicRoughnesRegistry.init(mSampler);
		mNormalRegistry.init(mSampler);

		mDeletionQueue.push_back(destroyTask{ .type = texRegistry, .texRegistry = &mAlbedoRegistry });
		mDeletionQueue.push_back(destroyTask{ .type = texRegistry, .texRegistry = &mMetalicRoughnesRegistry });
		mDeletionQueue.push_back(destroyTask{ .type = texRegistry, .texRegistry = &mNormalRegistry });

		// init uniform buffer.
		mUniformBuffer.init(mDevice, mAllocator);

		err = mUniformBuffer.buildAsUBO(mSubmit, nullptr, sizeof(uboPerDraw), 0);
		if (err)
			return err;

		mDeletionQueue.push_back(destroyTask{ .type = vulkanBuf, .vulkanBuf = &mUniformBuffer });

		return {};
	}

	error vulkanRenderer::initDescriptors()
	{
		error err = mDescriptorSetMesh.init(
			mDevice,
			mPhysicalDevice,
			poolConstraints{
				.maxImageDescriptors = mPhysicalDeviceLimits.maxImage,
				.maxCombinedImageDescriptors = mPhysicalDeviceLimits.maxCombinedImageSamplers,
				.maxBuffersDescriptors = mPhysicalDeviceLimits.maxStorageBuffers,
				.maxUniformBuffersDescriptors = mPhysicalDeviceLimits.maxUniformBuffers,
			}
			);
		if (err)
			return err;

		err = mDescriptorSetCompute.init(
			mDevice,
			mPhysicalDevice,
			poolConstraints{
				.maxImageDescriptors = mPhysicalDeviceLimits.maxImage,
				.maxCombinedImageDescriptors = mPhysicalDeviceLimits.maxCombinedImageSamplers,
				.maxBuffersDescriptors = mPhysicalDeviceLimits.maxStorageBuffers,
				.maxUniformBuffersDescriptors = mPhysicalDeviceLimits.maxUniformBuffers,
			}
			);
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
		std::vector<VkDescriptorImageInfo> colorAttachmentInfo = {
			{.sampler = VK_NULL_HANDLE, .imageView = mSwapChain.getDrawImageView(), .imageLayout = VK_IMAGE_LAYOUT_GENERAL,}
		};

		std::vector<VkDescriptorImageInfo> depthAttachmentInfo = {
			{.sampler = VK_NULL_HANDLE, .imageView = mSwapChain.getDepthImageView(), .imageLayout = VK_IMAGE_LAYOUT_GENERAL,}
		};

		mDescriptorSetCompute.addBinding(
			descriptorSet::getLayoutBindingInfo(mComputeBinding.colorAttachment, uint32_t(colorAttachmentInfo.size()), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		);

		error err = mDescriptorSetCompute.build(VK_SHADER_STAGE_COMPUTE_BIT, mComputeBinding.totalDescriptorsCount);
		if (err)
			return err;

		auto writeInfo = descriptorSet::getWriteInfo(mComputeBinding.colorAttachment, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, colorAttachmentInfo);
		mDescriptorSetCompute.updateWrite(writeInfo);

		return {};
	}

	error vulkanRenderer::setGeometryDescriptors()
	{
		const uint32_t combinedImageSamplers = 5;
		const uint32_t bufferObjects = 6;
		const uint32_t uniformObjects = 1;

		// add bindings for buffers.
		mDescriptorSetMesh.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mGeometryBinding.vertexBinding, mPhysicalDeviceLimits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSetMesh.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mGeometryBinding.perInstanceBinding, mPhysicalDeviceLimits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSetMesh.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mGeometryBinding.meshletCmdBinding, mPhysicalDeviceLimits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSetMesh.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mGeometryBinding.indexBinding, mPhysicalDeviceLimits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSetMesh.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mGeometryBinding.primitiveBinding, mPhysicalDeviceLimits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSetMesh.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mGeometryBinding.meshletBinding, mPhysicalDeviceLimits.maxStorageBuffers / bufferObjects, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
			)
		);

		mDescriptorSetMesh.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mGeometryBinding.perDrawBufferUboBinding, mPhysicalDeviceLimits.maxUniformBuffers / uniformObjects, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
			)
		);

		// add bindings for textures.
		mDescriptorSetMesh.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mGeometryBinding.albedoBinding, mPhysicalDeviceLimits.maxCombinedImageSamplers / combinedImageSamplers, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
			)
		);

		mDescriptorSetMesh.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mGeometryBinding.normalBinding, mPhysicalDeviceLimits.maxCombinedImageSamplers / combinedImageSamplers, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
			)
		);

		mDescriptorSetMesh.addBinding(
			descriptorSet::getLayoutBindingInfo(
				mGeometryBinding.metalicRoughnesBinding, mPhysicalDeviceLimits.maxCombinedImageSamplers / combinedImageSamplers, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
			)
		);

		error err = mDescriptorSetMesh.build(VK_SHADER_STAGE_ALL, mGeometryBinding.totalDescriptorsCount);
		if (err)
			return err;

		return {};
	}

	error vulkanRenderer::updateUboBuffers(renderer::renderCallIn in)
	{
		static std::once_flag descriptorWriteSet;

		std::call_once(
			descriptorWriteSet,
			[&]
			{
				std::vector<VkDescriptorBufferInfo> bufferInfo{
					VkDescriptorBufferInfo{.buffer = mUniformBuffer.getBuffer().buffer, .offset = 0, .range = VK_WHOLE_SIZE }
				};

				auto writeInfo = descriptorSet::getWriteInfo(mGeometryBinding.perDrawBufferUboBinding, bufferInfo, true);
				mDescriptorSetMesh.updateWrite(writeInfo);
			}
		);

		uboPerDraw data{
			.debugViewProjection = in.debugCameraProjection * in.debugCameraView,
			.cameraPos = in.cameraPos,
			.useDebugCamera = in.useDebugCamera,
			.cameraFront = in.cameraFront,
			.cameraUp = in.cameraUp,
			.view = in.view,
			.projection = in.projection,
			.viewProjection = in.projection * in.view,
			.cameraFrustum = in.cameraFrustum,
			.deltaTime = in.deltaTime,
		};

		mUniformBuffer.markBytesAsDead(sizeof(uboPerDraw));

		error err = mUniformBuffer.updateBuffer(
			mSubmit,
			&data,
			sizeof(uboPerDraw),
			0
		);
		if (err)
			return err;

		return {};
	}

	// Should be called before each frame.
	error vulkanRenderer::updateGeometryDescriptorsPerFrame(renderer::renderCallIn in)
	{
		// Update UBO buffers.
		error err = updateUboBuffers(in);
		if (err)
			return err;

		// Update command buffer for mesh pipeline.
		err = mPipelineRegistry.updateCommandBuffer();
		if (err)
			return err;

		// update buffers.
		if (mPipelineRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mPipelineRegistry.getWriteInfo(mGeometryBinding.meshletCmdBinding);
			mDescriptorSetMesh.updateWrite(writeInfo);
			mPipelineRegistry.setUpdated();
		}

		if (mVertexRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mVertexRegistry.getWriteInfo(mGeometryBinding.vertexBinding);
			mDescriptorSetMesh.updateWrite(writeInfo);
			mVertexRegistry.setUpdated();
		}

		if (mIndexRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mIndexRegistry.getWriteInfo(mGeometryBinding.indexBinding);
			mDescriptorSetMesh.updateWrite(writeInfo);
			mIndexRegistry.setUpdated();
		}

		if (mPrimitiveRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mPrimitiveRegistry.getWriteInfo(mGeometryBinding.primitiveBinding);
			mDescriptorSetMesh.updateWrite(writeInfo);
			mPrimitiveRegistry.setUpdated();
		}

		if (mMeshletRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mMeshletRegistry.getWriteInfo(mGeometryBinding.meshletBinding);
			mDescriptorSetMesh.updateWrite(writeInfo);
			mMeshletRegistry.setUpdated();
		}

		if (mPerInstanceRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mPerInstanceRegistry.getWriteInfo(mGeometryBinding.perInstanceBinding);
			mDescriptorSetMesh.updateWrite(writeInfo);
			mPerInstanceRegistry.setUpdated();
		}

		// Update textures.
		if (mAlbedoRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mAlbedoRegistry.getWriteInfo(mGeometryBinding.albedoBinding);
			mDescriptorSetMesh.updateWrite(writeInfo);
			mAlbedoRegistry.setUpdated();
		}

		if (mNormalRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mNormalRegistry.getWriteInfo(mGeometryBinding.normalBinding);
			mDescriptorSetMesh.updateWrite(writeInfo);
			mNormalRegistry.setUpdated();
		}

		if (mMetalicRoughnesRegistry.needDescriptorUpdate())
		{
			auto writeInfo = mMetalicRoughnesRegistry.getWriteInfo(mGeometryBinding.metalicRoughnesBinding);
			mDescriptorSetMesh.updateWrite(writeInfo);
			mMetalicRoughnesRegistry.setUpdated();
		}

		return {};
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
			case vulkanBuf:
				if (it->vulkanBuf)
					it->vulkanBuf->destroy();
				break;
			case pipelineReg:
				if (it->pipelineReg)
					it->pipelineReg->destroy();
				break;
			case vulkanTex:
				if (it->texture)
					it->texture->mImage.destroy();
				break;
			default:
				LOGERROR("unkown sampler");
			}
		}

		mDeletionQueue.clear();
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

	error vulkanRenderer::uploadGeometryData(const model& m, const std::array<uint32_t, 3> materialMappings)
	{
		// Create copy of a meshlet vector to upload to GPU.
		std::vector<meshlet> meshlets = *m.meshData.mesh.data.get();
		
		perInstanceAttr attr = m.instanceAttributes;
		attr.albedoIndex = materialMappings[0];
		attr.normalIndex = materialMappings[1];
		attr.metallicRoughnesIndex = materialMappings[2];

		auto handle = mVertexRegistry.addBlock(
			m.meshData.getHash(),
			m.meshData.vertex->data(),
			m.meshData.vertex->size() * sizeof(vertex)
		);
		if (!handle)
			return handle.err();

		for (auto& m : meshlets)
		{
			m.vertexBufferOffset += handle.value().offset / uint32_t(sizeof(vertex));
			m.vertexBufferIndex = handle.value().bufferIndex;
		}

		handle = mIndexRegistry.addBlock(
			m.meshData.getHash(),
			m.meshData.index.data->data(),
			m.meshData.index.data->size() * sizeof(uint32_t)
		);
		if (!handle)
			return handle.err();

		for (auto& m : meshlets)
		{
			m.indexBufferOffset += handle.value().offset / uint32_t(sizeof(uint32_t));
			m.indexBufferIndex = handle.value().bufferIndex;
		}

		handle = mPrimitiveRegistry.addBlock(
			m.meshData.getHash(),
			m.meshData.primitive.data->data(),
			m.meshData.primitive.data->size() * sizeof(uint32_t)
		);
		if (!handle)
			return handle.err();

		for (auto& m : meshlets)
		{
			m.triangleBufferOffset += handle.value().offset / uint32_t(sizeof(uint32_t));
			m.triangleBufferIndex = handle.value().bufferIndex;
		}

		handle = mMeshletRegistry.addBlock(
			m.meshData.getHash(),
			meshlets.data(),
			meshlets.size() * sizeof(meshlet)
		);
		if (!handle)
			return handle.err();

		auto perInstanceHandle = mPerInstanceRegistry.addBlock(
			m.id,
			&attr,
			sizeof(perInstanceAttr)
		);
		if (!perInstanceHandle)
			return perInstanceHandle.err();

		error err = mPipelineRegistry.addInstance(
			m.mat.pixelShader->hash(),
			m.id,
			m.meshData.getHash(),
			handle.value(),
			perInstanceHandle.value(),
			m.meshData.mesh
		);
		if (err)
			return err;

		return {};
	}

	withError<std::array<uint32_t, 3>> vulkanRenderer::uploadMaterialData(const model& m)
	{
		std::array<uint32_t, 3> result;

		// material data.
		if (m.mat.textures.albedoAtlas)
		{
			result[0] = mAlbedoRegistry.addTexture(m.mat.textures.albedoAtlas->hash(), static_cast<const vulkanTexture*>(m.mat.textures.albedoAtlas.get())->mImage);
		}

		if (m.mat.textures.normalAtlas)
		{
			result[1] = mNormalRegistry.addTexture(m.mat.textures.normalAtlas->hash(), static_cast<const vulkanTexture*>(m.mat.textures.normalAtlas.get())->mImage);
		}

		if (m.mat.textures.metalicRoughnesAtlas)
		{
			result[2] = mMetalicRoughnesRegistry.addTexture(m.mat.textures.metalicRoughnesAtlas->hash(), static_cast<const vulkanTexture*>(m.mat.textures.metalicRoughnesAtlas.get())->mImage);
		}

		return result;
	}

	error vulkanRenderer::addToRender(const model& m)
	{
		auto meshShader = mCtx->mAmanager->getDefaultMeshShader();
		if (!meshShader)
			return meshShader.err();

		auto taskShader = mCtx->mAmanager->getDefaultTaskShader();
		if (!taskShader)
			return taskShader.err();

		error err = mPipelineRegistry.createPipeline(
			mDevice,
			m.mat.pixelShader,
			meshShader.value(),
			taskShader.value(),
			{ mDescriptorSetMesh.getDescriptorSet().second },
			mSwapChain.getDepthImageFormat(),
			mSwapChain.getDrawImageFormat()
		);
		if (err)
			return err;

		if (mPipelineRegistry.instanceExists(m.id))
			return {};

		auto materialMappings = uploadMaterialData(m);
		if (!materialMappings)
			return materialMappings.err();

		uploadGeometryData(m, materialMappings.value());

		return {};
	}

	void vulkanRenderer::removeFromRender(const model& m)
	{
		mPerInstanceRegistry.deleteBlock(m.id);

		// Remove instance.
		mPipelineRegistry.removeInstance(m.mat.pixelShader->hash(), m.id, m.meshData.getHash());

		// Mesh isn't used.
		if (!mPipelineRegistry.meshIsUsed(m.meshData.getHash()))
		{
			mVertexRegistry.deleteBlock(m.meshData.getHash());

			mIndexRegistry.deleteBlock(m.meshData.getHash());

			mPrimitiveRegistry.deleteBlock(m.meshData.getHash());

			mMeshletRegistry.deleteBlock(m.meshData.getHash());
		}

		// TODO: figure out how to delete texture atlasses when they are no logner used.
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

	error vulkanRenderer::render(renderer::renderCallIn in)
	{
		if (mWindowMinimized)
			return {};

		// Geometry buffers/textures updated frequintly.
		error err = updateGeometryDescriptorsPerFrame(in);
		if (err)
			return err;

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

		auto resetResult = mSwapChain.resetCurrentFence();
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

		err = geometryPass(cmd, in);
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


		// Prepare the submission to the queue. 
		//	we want to wait on the _presentSemaphore and all semaphores that were created during resource creating, 
		//  _presentSemaphore semaphore is signaled when the swapchain is ready.
		// Remember when we ask GPU for image from swap chain we provide that semaphore to signal.
		// We will signal the _renderSemaphore, to signal that rendering has finished
		VkCommandBufferSubmitInfo cmdinfo = commandBufferSubmitInfo(cmd);

		std::vector<VkSemaphoreSubmitInfo> waitInfo{};
		std::vector<VkSemaphoreSubmitInfo> signalInfo;

		waitInfo.push_back(semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, mSwapChain.getCurrentFrameData().swapchainSemaphore));
		signalInfo.push_back(semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, mSwapChain.getCurrentFrameData().renderSemaphore));

		auto waitSema = mSubmit.getCurrentSemaInUse();
		for (auto& sema : waitSema)
			waitInfo.push_back(semaphoreSubmitInfo(VK_PIPELINE_STAGE_2_TASK_SHADER_BIT_EXT | VK_PIPELINE_STAGE_2_MESH_SHADER_BIT_EXT, sema));

		VkSubmitInfo2 submit = submitInfo(&cmdinfo, signalInfo, waitInfo);

		// submit command buffer to the queue and execute it.
		// _renderFence will now block until the graphic commands finish execution
		vkResult = vkQueueSubmit2(mGraphicsQueue, 1, &submit, mSwapChain.getCurrentFrameData().renderFence);
		if (vkResult != VK_SUCCESS)
		{
			return vkResultToStr(vkResult);
		}

		mSubmit.markAllSemaAsUsed();

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

	error vulkanRenderer::geometryPass(VkCommandBuffer cmd, renderer::renderCallIn in)
	{
		auto pipelines = mPipelineRegistry.getPipelines();

		if (pipelines.size() != 0)
		{
			//begin a render pass connected to our draw image and depth buffer.
			VkRenderingAttachmentInfo colorAttachment = attachmentInfo(mSwapChain.getDrawImageView(), nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
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
		}

		uint32_t cmdOffset = 0;
		for (auto& v : pipelines)
		{
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, v.pipeline);

			pushConstants pc{
				.commandBufferOffset = cmdOffset,
				.meshletCount = v.commandBufferLength,
			};

			vkCmdPushConstants(cmd, v.layout, VK_SHADER_STAGE_ALL, 0, sizeof(pushConstants), &pc);

			cmdOffset += v.commandBufferLength;

			// bind the descriptor set.
			auto set = mDescriptorSetMesh.getDescriptorSet().first;
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, v.layout, mGeometryBinding.descriptorSet, 1, &set, 0, nullptr);

			vkCmdDrawMeshTasksEXT(cmd, uint32_t(v.commandBufferLength) / mCtx->config.inner.render.shaderWorkGroup + 1, 1, 1);
		}

		if (pipelines.size() != 0)
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
		std::shared_ptr<texture> vkTexture = std::make_shared<vulkanTexture>(mDevice, mAllocator, mSubmit, data, width, heigth, channel);
		if (vkTexture->checkError())
			return vkTexture->checkError();

		mDeletionQueue.push_back(destroyTask{ .type = vulkanTex, .texture = static_cast<vulkanTexture*>(vkTexture.get()) });

		return vkTexture;
	}
}
