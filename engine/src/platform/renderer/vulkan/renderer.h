#pragma once

#include <pch.h>

#include "platform/window/window.h"
#include "core/camera/camera.h"
#include "vkHelper.h"
#include "vkPipeline.h"
#include "descriptorSet.h"
#include "swapChain.h"

#include <VkBootstrap.h>
#include <vk_mem_alloc.h>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#define VK_CHECK(x)                                                 \
    {\
		VkResult err = x;                                               \
		if (err != VK_SUCCESS)                                          \
			LOGERROR(vktest::vkResultToStr(err));                               \
	}\

namespace vktest
{
	// to do rewrite from using std::function to concrete vulkan handles may be using std::variant?
	struct DeletionQueue
	{
		std::deque<std::function<void()>> deletors;

		void push_function(std::function<void()>&& function)
		{
			deletors.push_back(function);
		}

		void flush()
		{
			for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
			{
				(*it)();
			}

			deletors.clear();
		}
	};

	class vulkanRenderer
	{
	public:
		vulkanRenderer(std::shared_ptr<engine::context> ctx);
		~vulkanRenderer();
		engine::error checkError();
		void init(engine::window* window);
		void draw(); // TODO: figure out what to pass here, some abstraction over vertex/index data and materials.

		void resize(uint32_t width, uint32_t height);

		// below camera stuff, with ECS will be moved.
		void changeCameraPos(glm::vec3);
		void changeYaw(float);
		void changePitch(float);
	private:
		std::shared_ptr<engine::context> mCtx;
		engine::fpsCamera mCamera;
		engine::error mErr;

		// VMA.
		VmaAllocator _allocator;

		// Vulkan library handle.
		VkInstance _instance;
		// Vulkan debug output handle.
		VkDebugUtilsMessengerEXT _debug_messenger;
		// GPU chosen as the default device.
		VkPhysicalDevice _chosenGPU;
		// Vulkan device for commands.
		VkDevice _device; 
		// Vulkan window surface.
		VkSurfaceKHR _surface;
		// Global deletaion queue.
		DeletionQueue _mainDeletionQueue;

		// swapChain.
		swapChain mSwapChain;

		// queue stuff, to submit command buffer.
		VkQueue _graphicsQueue;
		uint32_t _graphicsQueueFamily;

		// background pipeline for clear.
		computePipeline mComputePipeline;

		// Graphics pipeline for geometry.
		classicGraphicPipeline mGraphicsPipeline;

		// Descriptor set for image.
		descriptorSet mDescriptorSet;

		void clear(VkCommandBuffer);
		void draw_geometry(VkCommandBuffer cmd);

		void init_pipelines();
		void init_background_pipelines();
		void init_triangle_pipeline();

		void init_swapchain(uint32_t width, uint32_t height);
		void init_vulkan(engine::window* window);
		void init_descriptors();
		void set_descriptor_bindings();

		void printGPU()
		{
			uint32_t deviceCount = 0;
			vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);
			std::vector<VkPhysicalDevice> devices(deviceCount);
			vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

			auto deviceTypeToString = [](VkPhysicalDeviceType type) -> const char* {
				switch (type) {
				case VK_PHYSICAL_DEVICE_TYPE_OTHER:          return "Other";
				case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return "Integrated GPU";
				case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   return "Discrete GPU";
				case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    return "Virtual GPU";
				case VK_PHYSICAL_DEVICE_TYPE_CPU:            return "CPU";
				default:                                     return "Unknown";
				}
				};

			for (uint32_t i = 0; i < deviceCount; ++i)
			{
				VkPhysicalDeviceProperties props;
				vkGetPhysicalDeviceProperties(devices[i], &props);

				LOGINFO("GPU {}: {} (type = {}, api version = {}.{}.{})", i,
					props.deviceName,
					deviceTypeToString(props.deviceType),
					VK_VERSION_MAJOR(props.apiVersion),
					VK_VERSION_MINOR(props.apiVersion),
					VK_VERSION_PATCH(props.apiVersion));
			}
		}
	};
}
