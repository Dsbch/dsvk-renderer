#include <application/application.h>
#include "sandbox.h"

int main(int argc, char* argv[])
{
	try
	{
		engine::application app;
		engine::error err = app.checkError();
		if (err)
		{
			LOGERROR(err.err());
			return 0;
		}

		auto ss = std::make_unique<sandbox::sandboxSystem>(app.getAppContext());

		app.addUserSystem(std::move(ss));
		err = app.checkError();
		if (err)
		{
			LOGERROR(err.err());
			return 0;
		}

		err = app.run();
		if (err)
		{
			LOGERROR(err.err());
			return 0;
		}
	}
	catch (...)
	{
		LOGERROR("exception was caught in run");
		return 0;
	}

	return 0;
}
