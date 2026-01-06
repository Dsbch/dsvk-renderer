#include <pch.h>

#include "platform/renderer/vulkan/renderer.h"

namespace engine
{
	std::shared_ptr<renderer> makeRenderer(std::shared_ptr<context> ctx, std::shared_ptr<window> window)
	{
#ifdef VULKAN
		return std::make_shared<vulkanRenderer>(ctx, window);
#endif // VULKAN

		return std::shared_ptr<renderer>(nullptr);
	}
}
