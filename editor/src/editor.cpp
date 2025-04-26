#include <application/application.h>
#include <base/logger/logger.h>
#include <core/layers/layer.h>

class testOverlay : public engine::layer
{
public:
	testOverlay(engine::context ctx)
		:
		engine::layer(ctx)
	{

	}

	bool onEvent(std::shared_ptr<engine::baseEvent> e)
	{
		LOGINFO("testOverlay got event");

		return false;
	}

	void onRender()
	{
		LOGINFO("testOverlay render");
	}
};

class editor : public engine::application
{
public:
	editor() : engine::application()
	{
		if (mErr)
			return;

		pushOverlay(std::make_shared<testOverlay>(mCtx));
	}
};

int WINAPI WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR     lpCmdLine,
	_In_ int       nCmdShow
)
{
	try
	{
		editor e;
		if (auto err = e.checkError(); err)
		{
			LOGERROR(err.err());
			return 0;
		}

		e.run();
	}
	catch (const std::exception& exc)
	{
		LOGERROR("exception was caught in run std::exception: {}", exc.what());
	}
	catch (...)
	{

	}

	return 0;
}