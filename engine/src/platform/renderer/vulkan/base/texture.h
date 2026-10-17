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
		UID hash() const;
	private:
		UID mHash;
		vulkanImage mImage;

		friend class vulkanRenderer;
		friend struct materialRegistry;
	};
}