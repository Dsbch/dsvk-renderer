#include <pch.h>
#include "vertex.h"


namespace engine {
	std::vector<attributesDescriber::attributeInfo> vertexDescriber::info() const
	{
#ifdef VULKAN
		return {};
#endif // VULKAN

	}

	std::vector<attributesDescriber::attributeInfo> instancedAttrDescriber::info() const
	{
#ifdef VULKAN
		return {};
#endif // VULKAN
	}
}