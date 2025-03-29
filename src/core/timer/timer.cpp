#include <pch.h>
#include "time.h"
#include "timer.h"

core::timer::timer() : mStartTime(std::chrono::high_resolution_clock::now())
{}

std::chrono::steady_clock::time_point core::timer::getStartTime() const
{
	return mStartTime;
}

std::chrono::milliseconds core::timer::toMS(std::chrono::steady_clock::duration& d)
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(d);
}

std::chrono::steady_clock::duration core::timer::getTimeSinceStart() const
{
	auto end = std::chrono::high_resolution_clock::now();

	return end - mStartTime;
}
