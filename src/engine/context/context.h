#pragma once

#include <pch.h>
#include "../core/config/config.h"
#include "../core/timer/timer.h"
#include "../events/dispatcher.h"

namespace engine {
	class context {
	private:
		core::timer mTimer;
		std::shared_ptr<engine::eventDispatcher> mDispatcher;
	public:
		context();
		core::timer getTimer();
		std::shared_ptr<engine::eventDispatcher> getDispatcher();
	};
}