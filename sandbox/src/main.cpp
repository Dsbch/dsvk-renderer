#include <application/application.h>
#include <co/all.h>
#include "sandbox.h"

// Lightning:
// https://www.3dgep.com/volume-tiled-forward-shading/
// 
// 1. Can't implement volume tiled forward shading. Need depth prepass and I do not plan to use depth pre pass in my engine at all.
//    SSAO will not be used (I will use VCT).
//	  Also the cost of depth pre pass is too hight right now. Only geometry for 4 billion triangles takes 0.8ms pixel shader takes 1.2ms with 2X overdraw.
// 2. I will simply divide world space into cells each cell can have up to N lights. In two compute passes I need to put each light into a cell.
// 3. Render.
// 
// Shadow maps:
// Do research.
// 
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
