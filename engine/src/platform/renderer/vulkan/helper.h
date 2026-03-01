#pragma once
#include <pch.h>

#include <vulkan/vulkan.h>

namespace engine
{
	struct deviceLimits
	{
		uint32_t maxUniformBuffers;
		uint32_t maxStorageBuffers;
		uint32_t maxCombinedImageSamplers;
		uint32_t maxImage;
		VkSampleCountFlagBits maxMultiSampling;
		float maxFiltering;
	};

	inline std::string vkResultToStr(VkResult result)
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

	inline VkCommandPoolCreateInfo commandPoolCreateInfo(uint32_t queueFamilyIndex,
		VkCommandPoolCreateFlags flags /*= 0*/)
	{
		VkCommandPoolCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		info.pNext = nullptr;
		info.queueFamilyIndex = queueFamilyIndex;
		info.flags = flags;
		return info;
	}

	inline VkCommandBufferAllocateInfo commandBufferAllocateInfo(
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

	inline VkFenceCreateInfo fenceCreateInfo(VkFenceCreateFlags flags /*= 0*/)
	{
		VkFenceCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		info.pNext = nullptr;

		info.flags = flags;

		return info;
	}

	inline VkSemaphoreTypeCreateInfo timelineSemaphoreCreateInfo(uint32_t initialValue)
	{
		VkSemaphoreTypeCreateInfo info;
		info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
		info.pNext = NULL;
		info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
		info.initialValue = initialValue;

		return info;
	}

	inline VkTimelineSemaphoreSubmitInfo timelimeSemaphoreSubmitInfo(uint64_t& waitValue, uint64_t& signalValue, uint32_t waitCount, uint32_t signalCount)
	{
		VkTimelineSemaphoreSubmitInfo info;
		info.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
		info.pNext = NULL;
		info.waitSemaphoreValueCount = waitCount;
		info.pWaitSemaphoreValues = &waitValue;
		info.signalSemaphoreValueCount = signalCount;
		info.pSignalSemaphoreValues = &signalValue;

		return info;
	}

	inline VkSemaphoreCreateInfo semaphoreCreateInfo(VkSemaphoreCreateFlags flags, const void* pNext = nullptr)
	{
		VkSemaphoreCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		info.pNext = pNext;
		info.flags = flags;
		return info;
	}

	inline VkCommandBufferBeginInfo commandBufferBeginInfo(VkCommandBufferUsageFlags flags /*= 0*/)
	{
		VkCommandBufferBeginInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		info.pNext = nullptr;

		info.pInheritanceInfo = nullptr;
		info.flags = flags;
		return info;
	}

	inline VkCommandBufferSubmitInfo commandBufferSubmitInfo(VkCommandBuffer cmd)
	{
		VkCommandBufferSubmitInfo info{};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		info.pNext = nullptr;
		info.commandBuffer = cmd;
		info.deviceMask = 0;

		return info;
	}

	inline VkSemaphoreSubmitInfo semaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore, const void* pNext = nullptr)
	{
		VkSemaphoreSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		submitInfo.pNext = pNext;
		submitInfo.semaphore = semaphore;
		submitInfo.stageMask = stageMask;
		submitInfo.deviceIndex = 0;
		submitInfo.value = 1;

		return submitInfo;
	}

	inline VkSubmitInfo2 submitInfo(
		VkCommandBufferSubmitInfo* cmd, 
		std::vector<VkSemaphoreSubmitInfo>& signalSemaphoreInfo,
		std::vector<VkSemaphoreSubmitInfo>& waitSemaphoreInfo, 
		const void* pNext = nullptr
	)
	{
		VkSubmitInfo2 info = {};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		info.pNext = pNext;

		info.waitSemaphoreInfoCount = uint32_t(waitSemaphoreInfo.size());
		info.pWaitSemaphoreInfos = waitSemaphoreInfo.data();

		info.signalSemaphoreInfoCount = uint32_t(signalSemaphoreInfo.size());
		info.pSignalSemaphoreInfos = signalSemaphoreInfo.data();

		info.commandBufferInfoCount = 1;
		info.pCommandBufferInfos = cmd;

		return info;
	}

	inline VkSubmitInfo2 submitInfo(
		VkCommandBufferSubmitInfo* cmd,
		std::vector<VkSemaphoreSubmitInfo>& signalSemaphoreInfo,
		const void* pNext = nullptr
	)
	{
		VkSubmitInfo2 info = {};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		info.pNext = pNext;

		info.signalSemaphoreInfoCount = uint32_t(signalSemaphoreInfo.size());
		info.pSignalSemaphoreInfos = signalSemaphoreInfo.data();

		info.commandBufferInfoCount = 1;
		info.pCommandBufferInfos = cmd;

		return info;
	}

	inline VkSubmitInfo2 submitInfo(VkCommandBufferSubmitInfo* cmd, const void* pNext = nullptr)
	{
		VkSubmitInfo2 info = {};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		info.pNext = pNext;

		info.waitSemaphoreInfoCount = 0;
		info.pWaitSemaphoreInfos = nullptr;

		info.signalSemaphoreInfoCount = 0;
		info.pSignalSemaphoreInfos = nullptr;

		info.commandBufferInfoCount = 1;
		info.pCommandBufferInfos = cmd;

		return info;
	}

	inline VkRenderingAttachmentInfo depthAttachmentInfo(
		VkImageView view, VkImageLayout layout, bool needClear = true)
	{
		VkRenderingAttachmentInfo depthAttachment{};
		depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		depthAttachment.pNext = nullptr;

		depthAttachment.imageView = view;
		depthAttachment.imageLayout = layout;
		depthAttachment.loadOp = needClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachment.clearValue.depthStencil.depth = 0.f;

		return depthAttachment;
	}

	inline VkRenderingAttachmentInfo attachmentInfo(
		VkImageView drawImageView,
		VkImageView resolveImageView,
		VkResolveModeFlagBits resolveMode,
		VkClearValue* clear,
		VkImageLayout layout
	)
	{
		VkRenderingAttachmentInfo colorAttachment{};
		colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colorAttachment.pNext = nullptr;

		colorAttachment.imageView = drawImageView;
		colorAttachment.resolveImageView = resolveImageView;
		colorAttachment.resolveImageLayout = layout;
		colorAttachment.resolveMode = resolveMode;
		colorAttachment.imageLayout = layout;
		colorAttachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		if (clear) {
			colorAttachment.clearValue = *clear;
		}

		return colorAttachment;
	}

	inline VkRenderingInfo renderingInfo(VkExtent3D renderExtent, std::vector<VkRenderingAttachmentInfo>& colorAttachments, VkRenderingAttachmentInfo* depthAttachment)
	{
		VkRenderingInfo renderInfo{};
		renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderInfo.pNext = nullptr;

		renderInfo.renderArea = VkRect2D{ VkOffset2D { 0, 0 }, VkExtent2D{.width = renderExtent.width, .height = renderExtent.height} };
		renderInfo.layerCount = 1;
		renderInfo.colorAttachmentCount = uint32_t(colorAttachments.size());
		renderInfo.pColorAttachments = colorAttachments.data();
		renderInfo.pDepthAttachment = depthAttachment;
		renderInfo.pStencilAttachment = nullptr;

		return renderInfo;
	}

	inline VkSampleCountFlagBits sampleCounts(uint32_t sampleCount)
	{
		if (sampleCount == 0 || sampleCount == 1)
			return VK_SAMPLE_COUNT_1_BIT;

		if (sampleCount == 2)
			return VK_SAMPLE_COUNT_2_BIT;

		if (sampleCount == 4)
			return VK_SAMPLE_COUNT_4_BIT;

		if (sampleCount == 8)
			return VK_SAMPLE_COUNT_8_BIT;

		if (sampleCount == 16)
			return VK_SAMPLE_COUNT_16_BIT;

		if (sampleCount == 32)
			return VK_SAMPLE_COUNT_32_BIT;

		if (sampleCount == 64)
			return VK_SAMPLE_COUNT_64_BIT;

		return VK_SAMPLE_COUNT_1_BIT;
	}

	inline uint32_t sampleCountsAsUint(VkSampleCountFlagBits samples)
	{
		if (samples == VK_SAMPLE_COUNT_1_BIT)
			return 1;

		if (samples == VK_SAMPLE_COUNT_2_BIT)
			return 2;

		if (samples == VK_SAMPLE_COUNT_4_BIT)
			return 4;

		if (samples == VK_SAMPLE_COUNT_8_BIT)
			return 8;

		if (samples == VK_SAMPLE_COUNT_16_BIT)
			return 16;

		if (samples == VK_SAMPLE_COUNT_32_BIT)
			return 32;

		if (samples == VK_SAMPLE_COUNT_64_BIT)
			return 64;

		return 1;
	}

	inline VkResolveModeFlagBits getResolveMode(uint32_t sampleCount)
	{
		if (sampleCount <= 1)
			return VK_RESOLVE_MODE_NONE;

		return VK_RESOLVE_MODE_AVERAGE_BIT;
	}
}