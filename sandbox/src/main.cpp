#include <application/application.h>
#include <co/all.h>
#include "sandbox.h"

/*
	DONE:
		1. Full mesh shader geometry pass with instancing, and simple culling.
			Added culling types:
				1.1. Backface culling first in task shader next in mesh shader.
				1.2. Frustum culling in task shader.
		2. Integraion with ECS.
		3. Added material proccessing for PBR metallic workflow.
		4. Added OIT algoritm.
		5. Optimized OIT.
		6. Added concurrency library to the project libcoost.
		7. Added simple GPU profiling window.
	
	IN DEVELOPMENT:
		1. Add animation system, with skinning in compute shader.
	
	BUGS:
		1. Problem with flickering on new instance.
		2. Low performance on accumilaton pass.

	TODO:
		1. Two phase HZB occlision culling.
		2. Figure out how to do shadows, my goal is good hard and soft shadows.
		3. Global illumination and reflections with radiance cascades.
		4. Add jolt CPU side physics.
		5. Add postproccessing like bloom, focus etc.
		6. Get your bsdf and brdf together. Should use disney.
	
		Optional:
			1. Own file format. Ser/Dser of whole ECS.
			2. Own save files.
			3. Add guismos.
*/
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
