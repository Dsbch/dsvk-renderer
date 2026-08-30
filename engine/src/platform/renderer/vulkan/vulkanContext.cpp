#include <pch.h>

#include "vulkanContext.h"

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
	
	vulkanContext::vulkanContext(std::shared_ptr<context> ctx, std::shared_ptr<window> window) : 
		mCtx(ctx), 
		sChain(ctx->config.inner.graphics.framesInFlight)
	{
		mErr = initVulkan(window);
		if (mErr)
			return;

		mErr = setLimits();
		if (mErr)
			return;

		chooseGraphicsPreset();

		mErr = initImmediateSubmit();
		if (mErr)
			return;

		mErr = initSwapchain(window->getFbWidth(), window->getFbHeight());
		if (mErr)
			return;

		profiler.init(device, deviceLimits, mCtx->config.inner.graphics.framesInFlight);
		mErr = profiler.createProfiling(iSubmit);
		if (mErr)
			return;

		return;
	}

	error vulkanContext::checkError()
	{
		return mErr;
	}

	error vulkanContext::changeViewPort(uint32_t width, uint32_t height)
	{
		sChain.destroy();
		
		auto swapChainErr = sChain.build(iSubmit, width, height, graphicsQueueFamily);
		if (swapChainErr)
			return swapChainErr;

		return {};
	}

	error vulkanContext::initVulkan(std::shared_ptr<window> window)
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

		instance = vkb_inst.instance;
		debugMessenger = vkb_inst.debug_messenger;

		auto surfaceResult = window->makeVulkunSurface(instance);
		if (!surfaceResult)
			return surfaceResult.err();

		surface = surfaceResult.value();

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
		deviceFeatures.fragmentStoresAndAtomics = VK_TRUE;

		//use vkbootstrap to select a gpu. 
		//We want a gpu that can write to the SDL surface and supports vulkan 1.3 with the correct features.
		vkb::PhysicalDeviceSelector selector{ vkb_inst };
		auto selectedRes = selector
			.set_minimum_version(1, 3)
			.set_required_features_12(features12)
			.set_required_features(deviceFeatures)
			.add_required_extension(VK_EXT_MESH_SHADER_EXTENSION_NAME)
			.set_surface(surface)
			.select();
		if (!selectedRes)
			return selectedRes.error().message();

		vkb::PhysicalDevice pDevice = selectedRes.value();

		//create the final vulkan device
		vkb::DeviceBuilder deviceBuilder{ pDevice };

		auto buildResult = deviceBuilder.build();
		if (!buildResult.has_value())
			return buildResult.error().message();

		vkb::Device vkbDevice = buildResult.value();

		device = vkbDevice.device;
		physicalDevice = pDevice.physical_device;

		graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
		graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = physicalDevice;
		allocatorInfo.device = device;
		allocatorInfo.instance = instance;
		allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;

		auto vmaResult = vmaCreateAllocator(&allocatorInfo, &allocator);
		if (vmaResult != VK_SUCCESS)
			return { vkResultToStr(vmaResult) };

		error err = loadExtensions();
		if (err)
			return err;

		delQueue.init(device);

		delQueue.addDestroyTask(destroyTask{ .type = handleType::allocator, .allocator = allocator });

		return {};
	}
	error vulkanContext::setLimits()
	{
		VkPhysicalDeviceProperties props{};
		vkGetPhysicalDeviceProperties(physicalDevice, &props);

		deviceLimits.maxCombinedImageSamplers = props.limits.maxPerStageDescriptorSampledImages;
		deviceLimits.maxRWImage = props.limits.maxPerStageDescriptorStorageImages;
		deviceLimits.maxSampledImage = props.limits.maxPerStageDescriptorSampledImages;
		deviceLimits.maxStorageBuffers = props.limits.maxPerStageDescriptorStorageBuffers;
		deviceLimits.maxUniformBuffers = props.limits.maxPerStageDescriptorUniformBuffers;
		deviceLimits.maxFiltering = props.limits.maxSamplerAnisotropy;
		deviceLimits.timestampPeriod = props.limits.timestampPeriod;

		VkSampleCountFlags counts = props.limits.framebufferColorSampleCounts & props.limits.framebufferDepthSampleCounts;

		if (counts & VK_SAMPLE_COUNT_64_BIT)
			deviceLimits.maxMultiSampling = VK_SAMPLE_COUNT_64_BIT;
		else if (counts & VK_SAMPLE_COUNT_32_BIT)
			deviceLimits.maxMultiSampling = VK_SAMPLE_COUNT_32_BIT;
		else if (counts & VK_SAMPLE_COUNT_16_BIT)
			deviceLimits.maxMultiSampling = VK_SAMPLE_COUNT_16_BIT;
		else if (counts & VK_SAMPLE_COUNT_8_BIT)
			deviceLimits.maxMultiSampling = VK_SAMPLE_COUNT_8_BIT;
		else if (counts & VK_SAMPLE_COUNT_4_BIT)
			deviceLimits.maxMultiSampling = VK_SAMPLE_COUNT_4_BIT;
		else if (counts & VK_SAMPLE_COUNT_2_BIT)
			deviceLimits.maxMultiSampling = VK_SAMPLE_COUNT_2_BIT;
		else
			deviceLimits.maxMultiSampling = VK_SAMPLE_COUNT_1_BIT;

		return {};
	}

	error vulkanContext::initImmediateSubmit()
	{
		error err = iSubmit.init(mCtx, device, graphicsQueue, graphicsQueueFamily);
		if (err)
			return err;

		delQueue.addDestroyTask(destroyTask{ .type = handleType::iSub, .iSubmit = &iSubmit });

		return {};
	}

	error vulkanContext::loadExtensions()
	{
		vkCmdDrawMeshTasksEXT = (PFN_vkCmdDrawMeshTasksEXT)vkGetDeviceProcAddr(device, "vkCmdDrawMeshTasksEXT");
		if (!vkCmdDrawMeshTasksEXT)
			return { "can't load extensions" };

		vkCmdDrawMeshTasksIndirectEXT = (PFN_vkCmdDrawMeshTasksIndirectEXT)vkGetDeviceProcAddr(device, "vkCmdDrawMeshTasksIndirectEXT");
		if (!vkCmdDrawMeshTasksIndirectEXT)
			return { "can't load extensions" };

		return {};
	}

	error vulkanContext::initSwapchain(uint32_t width, uint32_t height)
	{
		sChain.init(allocator, device, surface, physicalDevice, iSubmit, preset);

		error err = sChain.build(iSubmit, width, height, graphicsQueueFamily);
		if (err)
			return err;

		delQueue.addDestroyTask(destroyTask{ .type = handleType::sChain, .sChain = &sChain });

		return {};
	}

	void vulkanContext::chooseGraphicsPreset()
	{
		preset = graphicsPreset{
			.msaa = mCtx->config.inner.graphics.msaa,
			.anisotropicFiltering = mCtx->config.inner.graphics.anisotropicFiltering,
		};

		if (preset.anisotropicFiltering > uint32_t(deviceLimits.maxFiltering))
			preset.anisotropicFiltering = uint32_t(deviceLimits.maxFiltering);

		if (preset.msaa > sampleCountsAsUint(deviceLimits.maxMultiSampling))
			preset.msaa = sampleCountsAsUint(deviceLimits.maxMultiSampling);
	}
}