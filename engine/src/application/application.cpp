#include <pch.h>
#include "application.h"
#include "core/layers/layerStack.h"
#include "core/layers/layer.h"
#include "platform/window/window.h"
#include "platform/window/windowFactory.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

namespace engine
{
	application* application::app = nullptr;

	error application::checkError()
	{
		if (auto err = mWindow->checkError(); err)
			return err;

		if (auto err = mLayerStack->checkError(); err)
			return err;

		return mErr;
	}

	error application::initApplication()
	{
		if (app)
		{
			return { "application already created" };
		}

		if (mCtx->config.inner.log.useFile)
		{
			logger::initLogger(mCtx->config.inner.app.name, mCtx->config.inner.log.file, mCtx->config.inner.log.pattern, mCtx->config.inner.log.level);
		}
		else
		{
			logger::initLogger(mCtx->config.inner.app.name, mCtx->config.inner.log.pattern, mCtx->config.inner.log.level);
		}

		if (auto err = mCtx->config.checkError(); err)
			LOGERROR("{}", err.err());

		return {};
	}

	error application::createWindow()
	{
		cond cv;
		mCtx->mThreadPool->start(
			[&]() -> void {
				mWindow = windowFactory::createWindow(mCtx, mCtx->config.inner.wnd.name, mCtx->config.inner.wnd.width, mCtx->config.inner.wnd.height, mCtx->config.inner.wnd.isFullscreen, mCtx->config.inner.app.name, mCtx->config.inner.wnd.showCursor);
				if (mErr = mWindow->checkError(); mErr)
					return;

				cv.notifyOne();

				mWindow->startPolling();
			}
		);

		cv.wait([&] {return mWindow.get(); });

#ifdef OPENGL
		return mWindow->makeOpenglContext();
#endif // OPENGL
#ifdef VULKAN
		return {};
#endif // VULKAN
	}

	error application::createLayerStack()
	{
		pushLayer(std::make_unique<worldLayer>(mCtx));

		return mLayerStack->checkError();
	}

	void application::update(std::chrono::milliseconds& nextGameUpdate, std::chrono::milliseconds updateShift, uint32_t maxFrameSkip)
	{
		auto k = mCtx->timer.toMS(mCtx->timer.getTimeSinceStart());
		for (uint32_t i = 0; mCtx->timer.toMS(mCtx->timer.getTimeSinceStart()) >= nextGameUpdate && i < maxFrameSkip && mRunning; i++)
		{
			// Queue events in main dispatcher.
			mWindow->pollInput();

			// Dispatch events.
			while (mCtx->mEventDispatcher->hasEvents())
			{
				// handle window close event.
				auto e = mCtx->mEventDispatcher->getEvent();
				if (e->getEventType() == eventType::close)
				{
					mRunning = false;
				}

				mLayerStack->onEvent(e);
			}

			// run updates.
			mLayerStack->onUpdate();

			nextGameUpdate += updateShift;
		}
	}

	void application::onRender(std::chrono::milliseconds& nextRender, std::chrono::milliseconds renderShift)
	{
		if (mCtx->timer.toMS(mCtx->timer.getTimeSinceStart()) >= nextRender)
		{
			mLayerStack->onRender();
			mWindow->swapBuffers();
			nextRender += renderShift;
		}
	}

	application::application()
		:
		mErr(), mCtx(std::make_shared<context>(cfg<main>{})), mLayerStack(std::make_unique<layerStack>()), mWindow(nullptr), mRunning(false)
	{
		mErr = initApplication();
		if (mErr)
			return;

		mErr = createWindow();
		if (mErr)
			return;

		mErr = createLayerStack();
		if (mErr)
			return;

		app = this;
	}

	application::~application()
	{
#ifdef DEBUG
		DUMP_PROFILING("prof.json");
#endif // DEBUG
	}

	void application::pushLayer(std::unique_ptr<layer>&& l)
	{
		mLayerStack->pushLayer(std::move(l));
	}

	void application::pushOverlay(std::unique_ptr<layer>&& l)
	{
		mLayerStack->pushOverlay(std::move(l));
	}

	void application::run()
	{
		mRunning = true;

		std::chrono::milliseconds nextGameUpdate = mCtx->timer.toMS(mCtx->timer.getTimeSinceStart());
		uint32_t maxFrameSkip = mCtx->config.inner.gameLoop.gups / mCtx->config.inner.gameLoop.minimumFps;
		std::chrono::milliseconds updateShift = std::chrono::milliseconds(1000 / mCtx->config.inner.gameLoop.gups);

		std::chrono::milliseconds nextRender = mCtx->timer.toMS(mCtx->timer.getTimeSinceStart());
		std::chrono::milliseconds renderShift = std::chrono::milliseconds(1000 / mCtx->config.inner.gameLoop.fps);

		while (mRunning)
		{
			update(nextGameUpdate, updateShift, maxFrameSkip);
			onRender(nextRender, renderShift);
		}
	}

	void testVulkanAllocator(HWND hwnd)
	{
		vkb::InstanceBuilder builder;
		auto inst_ret = builder
			.set_app_name("Test")
			.request_validation_layers()
			.require_api_version(1, 1, 0)
			.build();

		if (!inst_ret) {
			LOGERROR("Failed to create Vulkan instance: {}", inst_ret.error().message());
			return;
		}

		vkb::Instance vkb_inst = inst_ret.value();
		VkInstance instance = vkb_inst.instance;

		VkWin32SurfaceCreateInfoKHR surface_info{};
		surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surface_info.hwnd = hwnd;
		surface_info.hinstance = GetModuleHandle(NULL);

		VkSurfaceKHR surface = VK_NULL_HANDLE;
		vkCreateWin32SurfaceKHR(instance, &surface_info, nullptr, &surface);

		vkb::PhysicalDeviceSelector selector{ vkb_inst };
		auto phys_ret = selector
			.set_surface(surface) // <- IMPORTANT
			.set_minimum_version(1, 1)
			.select();

		if (!phys_ret) {
			LOGERROR("Failed to select physical device: {}", phys_ret.error().message());
			return;
		}

		vkb::PhysicalDevice vkb_phys = phys_ret.value();

		vkb::DeviceBuilder dev_builder{ vkb_phys };
		auto dev_ret = dev_builder.build();

		if (!dev_ret) {
			LOGERROR("Failed to create logical device: {}", dev_ret.error().message());
			return;
		}

		vkb::Device vkb_device = dev_ret.value();
		VkDevice device = vkb_device.device;
		VkPhysicalDevice phys_device = vkb_phys.physical_device;

		// VMA
		VmaAllocator allocator{};
		VmaAllocatorCreateInfo allocator_info{};
		allocator_info.physicalDevice = phys_device;
		allocator_info.device = device;
		allocator_info.instance = instance;

		if (vmaCreateAllocator(&allocator_info, &allocator) != VK_SUCCESS) {
			LOGERROR("Failed to create VMA allocator");
			return;
		}

		// Create a test buffer
		VkBuffer buffer{};
		VmaAllocation allocation{};

		VkBufferCreateInfo buffer_info{};
		buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_info.size = 4096;
		buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

		VmaAllocationCreateInfo alloc_create_info{};
		alloc_create_info.usage = VMA_MEMORY_USAGE_CPU_ONLY;

		if (vmaCreateBuffer(allocator, &buffer_info, &alloc_create_info, &buffer, &allocation, nullptr) != VK_SUCCESS) {
			LOGERROR("Failed to create buffer with VMA");
			vmaDestroyAllocator(allocator);
			return;
		}

		LOGINFO("Vulkan + VMA buffer created successfully");

		// Cleanup
		vmaDestroyBuffer(allocator, buffer, allocation);
		vmaDestroyAllocator(allocator);
		vkDestroyDevice(device, nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);
	}

}