#include <application/application.h>
#include <co/all.h>
#include "sandbox.h"

// Current todo list:
// Things that have to be unique per frame:
// 1. Command buffers.
// 2. Joint buffer.
// 3. Visability buffer.
// 4. Draw UBO buffer.
// 5. Pre intsance buffer (transform updates).
// Write more optimized animation system with co::corutines.
// Do not block game thread on asset load.
int main(int argc, char* argv[])
{
	flag::parse(argc, argv);

	// On main thread only app.Run.
	{
		engine::application app{};

		engine::error err = app.checkError();
		if (err)
		{
			LOGERROR(err.err());
			return 0;
		}

		app.addUserSystem(std::make_unique<sandbox::sandboxSystem>(app.getAppContext()));
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

	return 0;
}
