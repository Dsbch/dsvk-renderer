#pragma once

#include <pch.h>
#include "../core/config/config.h"
#include "../core/timer/timer.h"

namespace engine {
	class context {
	private:
		core::timer mTimer;
	public:
		context() = default;
		core::timer getTimer();
	};
}