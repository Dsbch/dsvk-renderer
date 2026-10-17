#include <pch.h>
#include <random>
#include <array>

namespace engine
{
	typedef uint64_t UID;

    inline UID crc64(const uint8_t* data, size_t len)
    {
        static const std::array<UID, 256> table = [] {
            std::array<UID, 256> t{};

            for (UID i = 0; i < 256; i++)
            {
                UID c = i;
                for (int j = 0; j < 8; j++)
                {
                    if (c & 1)
                        c = 0xC96C5795D7870F42ull ^ (c >> 1);
                    else
                        c >>= 1;
                }
                t[i] = c;
            }

            return t;
            }();

        UID crc = ~0ull;
        for (size_t i = 0; i < len; i++)
            crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);

        return ~crc;
    }

    inline UID genUID()
    {
        thread_local std::mt19937_64 gen = [] {
            std::random_device rd;
            std::seed_seq seq{ rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd() };
            return std::mt19937_64(seq);
            }();

        return gen();
    }

	inline UID mergeCrc64(const std::vector<UID>& crc)
	{
		return crc64(reinterpret_cast<const uint8_t*>(crc.data()), crc.size() * sizeof(UID));
	}
}