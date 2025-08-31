#pragma once

#include <pch.h>
#include "vulkanShader.h"

namespace engine
{
	bool loadShaderModule(const std::vector<uint32_t>& src, VkDevice device, VkShaderModule* outShaderModule)
	{
		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.pNext = nullptr;

		createInfo.codeSize = src.size() * sizeof(uint32_t);
		createInfo.pCode = src.data();

		// check that the creation goes well.
		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) 
		{
			return false;
		}
		
		*outShaderModule = shaderModule;
		return true;
	}

	vulkanShader::vulkanShader(VkDevice device, const std::vector<uint32_t>& src)
		:
		shader(src),
		mSrc(src),
		mDevice(device)
	{
		if (!loadShaderModule(mSrc, mDevice, &mShaderModule))
			mErr = error{"can't create shader"};
	}

	vulkanShader::~vulkanShader()
	{
		vkDestroyShaderModule(mDevice, mShaderModule, nullptr);
	}

	uint32_t vulkanShader::hash() const
	{
		return crc32(reinterpret_cast<const uint8_t*>(mSrc.data()), mSrc.size());
	}

	void vulkanShader::setPushConstant(const pushConstant& pc)
	{
		mPushConstant = pc;
	}
}