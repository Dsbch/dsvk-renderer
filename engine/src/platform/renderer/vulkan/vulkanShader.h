#pragma once

#include "pch.h"

#include <vulkan/vulkan.h>

#include <platform/renderer/shader.h>


namespace engine
{
	bool loadShaderModule(const std::vector<uint32_t>& src, VkDevice device, VkShaderModule* outShaderModule);

	class vulkanShader : public shader
	{
	public:
		vulkanShader(VkDevice device, const std::vector<uint32_t>& src);
		~vulkanShader();

		uint32_t hash() const;
		void setPushConstant(const pushConstant&);
	private:
		uint32_t mHash;
		const std::vector<std::uint32_t> mSrc;
		VkDevice mDevice;
		VkShaderModule mShaderModule;

		friend class vulkanRenderer;
	};
}