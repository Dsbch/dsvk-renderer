#include <application/application.h>
#include <co/all.h>
#include "sandbox.h"

int main(int argc, char* argv[])
{
	flag::parse(argc, argv);

	// On main thread only app.Run.
	{
		engine::application app;

		engine::error err = app.checkError();
		if (err)
		{
			LOGERROR(err.err());
			return 0;
		}

		auto ss = std::make_shared<sandbox::sandboxSystem>(app.getAppContext());

		app.addUserSystem(ss);
		err = app.checkError();
		if (err)
		{
			LOGERROR(err.err());
			return 0;
		}

		app.run();

		err = app.checkError();
		if (err)
		{
			LOGERROR(err.err());
			return 0;
		}
	}

	// Wait for all background threads to finish.
	waitDone();

	return 0;
}
