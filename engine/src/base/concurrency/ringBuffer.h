#pragma once

#include <pch.h>

#include <queue>

namespace engine
{
	struct empty {};

	// Ring buffer is concurrent safe (only for 1 consumer and 1 poducer).
	// When reader calls recieve he will be blocked until new value is pushed or return if value was already pushed/close was called.
	// Buffer has capacity when on write capacity is reached, it will overrite old value with new.
	// When recieve returns false, it means close was called and caller shouldn't use that ring buffer anymore.
	template<class T, size_t capacity>
	class ringBuffer
	{
	public:
		ringBuffer();
		ringBuffer(ringBuffer&) = delete;

		bool recieve(T& val);
		bool push(T val);
		void close();
	private:
		co::chan<empty> mWriteEvent;
		std::atomic<int> mReadIdx;
		std::atomic<int> mWriteIdx;
		std::atomic<int> mToRead;
		std::array<T, capacity> mBuffer;
	};

	template<class T, size_t capacity>
	inline ringBuffer<T, capacity>::ringBuffer()
		: mReadIdx(0), mWriteIdx(), mToRead(), mWriteEvent({ capacity })
	{
	}

	template<class T, size_t capacity>
	inline bool ringBuffer<T, capacity>::recieve(T& val)
	{
		empty e{};
		mWriteEvent >> e;

		if (!mWriteEvent)
			return false;

		val = mBuffer[mReadIdx];
		mReadIdx = (mReadIdx + 1) % capacity;
		mToRead--;

		return true;
	}

	template<class T, size_t capacity>
	inline bool ringBuffer<T, capacity>::push(T val)
	{
		if (!mWriteEvent)
			return false;

		if (mReadIdx == mWriteIdx && mToRead != 0)
		{
			// Write to last idx.
			int last = (mWriteIdx == 0 ? capacity - 1 : mWriteIdx - 1);
			mBuffer[last] = val;
			return true;
		}

		mBuffer[mWriteIdx] = val;

		mWriteIdx = (mWriteIdx + 1) % capacity;
		mToRead++;

		empty e{};
		mWriteEvent << e;

		return true;
	}

	template<class T, size_t capacity>
	inline void ringBuffer<T, capacity>::close()
	{
		mWriteEvent.close();
	}
}