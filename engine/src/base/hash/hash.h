#include <pch.h>
#include <random>

namespace engine
{
	inline uint32_t crc32(const uint8_t* data, size_t len)
	{
		static uint32_t table[256];
		static bool initialized = false;

		if (!initialized)
		{
			for (uint32_t i = 0; i < 256; i++)
			{
				uint32_t c = i;
				for (int j = 0; j < 8; j++)
				{
					if (c & 1)
					{
						c = 0xEDB88320u ^ (c >> 1);
					}
					else
					{
						c >>= 1;
					}
				}
				table[i] = c;
			}
			initialized = true;
		}

		uint32_t crc = 0xFFFFFFFFu;
		for (size_t i = 0; i < len; i++)
		{
			crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
		}

		return crc ^ 0xFFFFFFFFu;
	}

	inline uint32_t genUID()
	{
		static std::random_device rd;
		static std::mt19937 gen(rd());
		static std::uniform_int_distribution<uint32_t> distrib(0, UINT32_MAX);

		return distrib(gen);
	}

	inline uint32_t mergeCrc32(const std::vector<uint32_t> crc)
	{
		return crc32(reinterpret_cast<const uint8_t*>(crc.data()), crc.size() * sizeof(uint32_t));
	}
}