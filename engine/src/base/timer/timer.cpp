#include <pch.h>
#include "timer.h"

namespace engine
{
	timer::timer() : mStartTime(std::chrono::steady_clock::now()) {}

	std::chrono::steady_clock::time_point timer::getStartTime() const
	{
		return mStartTime;
	}

	std::chrono::steady_clock::duration timer::getTimeSinceStart() const
	{
		auto end = std::chrono::steady_clock::now();

		return end - mStartTime;
	}
}