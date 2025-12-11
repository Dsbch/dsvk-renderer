#pragma once

#include <pch.h>
#include "platform/renderer/vertex.h"

namespace engine
{
	withError<lodMesh> loadMesh(const std::string& path, size_t maxVert, size_t maxTriangles, float coneWieght);

	withError<lodMesh> loadMeshTest(const std::string& path, size_t maxVert, size_t maxTriangles, float coneWieght);
}