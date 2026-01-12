#pragma once
#include <pch.h>

namespace engine
{
	template<class key, class val>
	class lruCache
	{
	public:
		lruCache(uint32_t capacity) : mCapacity(capacity) {};
		~lruCache() {};
		void put(const key& k, const val& v);
		withError<val> get(const key& k);
		void clear();
	private:
		using listType = std::list<key>;
		using iteratorType = typename listType::iterator;

		uint32_t mCapacity;
		std::list<key> mList;
		std::map<key, std::pair<val, iteratorType>> mMap;
	};

	template<class key, class val>
	inline void lruCache<key, val>::put(const key& k, const val& v)
	{
		if (auto found = mMap.find(k); found != mMap.end())
		{
			mList.erase(found->second.second);
			mMap.erase(k);
		}

		if (mCapacity == mList.size())
		{
			auto lastElement = mList.end();
			lastElement--;

			mMap.erase(*lastElement);
			mList.erase(lastElement);
		}

		mList.push_front(k);
		mMap[k] = std::pair<val, std::list<key>::iterator>(v, mList.begin());
	}

	template<class key, class val>
	inline withError<val> lruCache<key, val>::get(const key& k)
	{
		if (auto found = mMap.find(k); found != mMap.end())
		{
			mList.erase(found->second.second);

			mList.push_front(k);

			found->second.second = mList.begin();

			return found->second.first;
		}

		return error{"element not found"};
	}

	template<class key, class val>
	inline void lruCache<key, val>::clear()
	{
		mMap.clear();
		mList.clear();
	}
}
