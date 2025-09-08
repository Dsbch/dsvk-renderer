#pragma once

#include <pch.h>
#include <platform/renderer/texture.h>

#include "vulkanImage.h"

namespace engine
{
	class vulkanTexture : public texture
	{
	public:
		vulkanTexture(VkDevice device, VmaAllocator allocator, immediateSubmit is, uint8_t* data, int width, int heigth, imageChannel channel);
		~vulkanTexture();
		uint32_t hash() const;
	private:
		uint32_t mHash;
		vulkanImage mImage;

		friend class vulkanRenderer;
	};
}