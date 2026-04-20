#pragma once

#include <pch.h>

namespace engine 
{
	class executionTimer
	{
	private:
		std::string mName;
		std::chrono::steady_clock::time_point mStart;
		long long mMinToLog;
	public:
		executionTimer(std::string&& name, long long minToLog = 0)
			:
			mStart(std::chrono::high_resolution_clock::now()),
			mName(std::move(name)),
			mMinToLog(minToLog)
		{
		}

		~executionTimer()
		{
			auto end = std::chrono::high_resolution_clock::now();

			auto count = std::chrono::duration_cast<std::chrono::microseconds>(end - mStart).count();

			if (count >= mMinToLog)
				LOGDEBUG("{} took: {}", mName, count);
		}
	};

	class timer
	{
	private:
		std::chrono::steady_clock::time_point mStartTime;
	public:
		timer();
		std::chrono::steady_clock::duration getTimeSinceStart() const;
		std::chrono::steady_clock::time_point getStartTime() const;
	};
}