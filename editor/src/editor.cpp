#include <application/application.h>

namespace editor
{
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
	catch (...)
	{
		LOGERROR("exception was caught in run");
	}

	return 0;
}