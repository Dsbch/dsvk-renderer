#include <application/application.h>
#include <co/all.h>
#include "sandbox.h"


// Optimizations:
// 1. Fix UUID collision make uuid 64bit.
// 2. Add visability buffer for EACH meshlet.
// 3. Make compact buffer resizable need to figure out GPU readback after culling.
// 4. Go to TASK shaders and remove computeMeshletCulling shader.
// 5. Profile it!
// 6. Fix voxelization.

// Voxelization:
// 1. Calculate scene bounding box.
// 2. Write new voxel renderer, that will voxelize a scene (depth test off, culling off, ortho projection) into 3D texture.
// 2.1. Need to implement triangle swizling in mesh shader.
// 3. Try to visualize voxelized scene using raymarching.
// 4. Fix line renderer, lines should be added with renderPackage.
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
