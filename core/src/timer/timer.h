#pragma once

#include <chrono>

namespace core {
	class timer {
	private:
		std::chrono::steady_clock::time_point mStartTime;
	public:
		timer();
		std::chrono::steady_clock::duration getTimeSinceStart() const;
		std::chrono::steady_clock::time_point getStartTime() const;
		
		static std::chrono::milliseconds toMS(std::chrono::steady_clock::duration& d);
	};
}