#pragma once

#include <pch.h>
#include "../core/config/config.h"
#include "../core/timer/timer.h"
#include "../core/concurrency/concurrency.h"
#include "../events/dispatcher.h"

namespace engine {
	class context {
	private:
		core::timer mTimer;
		core::threadPool mThreadPool;
		std::shared_ptr<engine::eventDispatcher> mDispatcher;
	public:
		context();
		core::timer getTimer();
		std::shared_ptr<engine::eventDispatcher> getDispatcher();
		core::threadPool getThreadPool();
	};
}