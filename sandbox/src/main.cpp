#include <platform/window/windowFactory.h>
#include <application/application.h>
int main(int argc, char* argv[])
{
	try
	{
		auto ctx = std::make_shared<engine::context>(engine::cfg<engine::main>{});
		std::unique_ptr<engine::window> window = nullptr;

		engine::cond cv;
		ctx->mThreadPool->start(
			[&]() -> void {
				window = engine::windowFactory::createWindow(ctx, ctx->config.inner.wnd.name, ctx->config.inner.wnd.width, ctx->config.inner.wnd.height, ctx->config.inner.wnd.isFullscreen, ctx->config.inner.app.name, ctx->config.inner.wnd.showCursor);
				if (auto err = window->checkError(); err)
					return;

				cv.notifyOne();

				window->startPolling();
			}
		);

		cv.wait([&] { return window.get(); });

		if (auto err = window->checkError(); err)
			LOGERROR("wnd err: {}", err.err());

		LOGINFO("window created");
		
		engine::testVulkanAllocator(static_cast<engine::winApiWindow*>(window.get())->getHandle());

		std::this_thread::sleep_for(std::chrono::seconds(2));

		LOGINFO("exiting");
	}
	catch (const std::exception& exc)
	{
		LOGERROR("exception was caught in run std::exception: {}", exc.what());
	}
	catch (...)
	{
		LOGERROR("exception was caught in run");
	}

	return 0;
}
