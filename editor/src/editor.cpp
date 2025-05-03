#include <application/application.h>
#include <base/logger/logger.h>
#include <core/layers/layer.h>

class editorLayer : public engine::layer
{
public:
	editorLayer(engine::context ctx)
		:
		engine::layer(ctx)
	{

	}

	bool onEvent(std::shared_ptr<engine::baseEvent> e)
	{
		return false;
	}

	void onRender()
	{
	}

	engine::error checkError() const { return {}; };
};

class editor : public engine::application
{
public:
	editor() : engine::application()
	{
		if (mErr)
			return;

		pushOverlay(std::make_shared<editorLayer>(mCtx));
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
		LOGERROR("exception was caught in run");
	}

	return 0;
}