#pragma once

#include <pch.h>
#include "system.h"

#include <base/context/context.h>


namespace engine
{
	class gpuDrivenRenderSystem :
		public system
	{
	public:
		gpuDrivenRenderSystem(std::shared_ptr<context>);
	};
}
