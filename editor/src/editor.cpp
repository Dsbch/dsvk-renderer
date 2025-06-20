#include <application/application.h>

namespace editor
{
	// TODO: implement me.
	class editor : public engine::application
	{
	public:
		editor() : engine::application()
		{
			LOGDEBUG("TODO: implement me");
		}
	};
}

int main(int argc, char* argv[])
{
	try
	{
		editor::editor e;
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