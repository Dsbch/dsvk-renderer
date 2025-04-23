#include "time.h"
#include "timer.h"

namespace core
{
	timer::timer() : mStartTime(std::chrono::high_resolution_clock::now()) {}

	std::chrono::steady_clock::time_point timer::getStartTime() const
	{
		return mStartTime;
	}

	std::chrono::milliseconds timer::toMS(std::chrono::steady_clock::duration& d)
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(d);
	}

	std::chrono::steady_clock::duration timer::getTimeSinceStart() const
	{
		auto end = std::chrono::high_resolution_clock::now();

		return end - mStartTime;
	}
}