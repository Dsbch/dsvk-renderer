#pragma once

#include <pch.h>
#include <platform/renderer/texture.h>

#include "image.h"

namespace engine
{
	// Class is used only for dynamic binding.
	class vulkanTexture : public texture
	{
	public:
		vulkanTexture(VkDevice device, VmaAllocator allocator, submit& is, const image& img);
		vulkanTexture(VkDevice device, VmaAllocator allocator, submit& is, const imageWithMipLevels& img);
		~vulkanTexture();
		uint32_t hash() const;
	private:
		uint32_t mHash;
		vulkanImage mImage;

		friend class vulkanRenderer;
		friend struct materialRegistry;
	};
}