#include <application/application.h>
#include "sandbox.h"

/*
	Implemented:
		1. Full mesh shader geometry pass with instancing, and simple culling - DONE.
			Added culling types:
				1.1. Backface culling first in task shader next in mesh shader.
				1.2. Frustum culling in task shader.
		2. Integraion with ECS.
		3. Added material proccessing for PBR metallic workflow - DONE.
		4. Added OIT algoritm - DONE.

	To implement:
	
	Core engine systems:
		1. Added animation system - in development.
		2. Add VCT for global illumination and soft shadows - on hold.
			3.1. Optional: add shadow mapping for hard shadows - on hold.
		3. Occlussion culling - on hold.
		4. Add postproccessing like bloom, focus etc - on hold.
		5. Get your bsdf and brdf together - on hold.

	Physics:
		1. Add jolt CPU side physics - on hold.

	Asset manager:
		1. Own file format. Ser/Dser of whole ECS - on hold.
		2. Own save files - on hold.

	Editor:
		1. Simple debug window - on hold.
		2. Add guismos - on hold.

	Current development TODO:
		1. Figure out how to handle animations updates. For now I need more flat structure for joints parent to child. Also look for copy leaks on each frame.
		 1.1. It should handle at least 1K animated objects before starting to lag.
		2. Frustum culling for animated meshes doesn't work. Need to fix it.
*/

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

		auto ss = std::make_shared<sandbox::sandboxSystem>(app.getAppContext());

		app.addUserSystem(ss);
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
