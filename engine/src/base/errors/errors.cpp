#include <pch.h>
#include "errors.h"

namespace engine
{
	error::error() : mValue(), mErrorCode(0) {}

	error::error(uint32_t errCode) : mValue(), mErrorCode(errCode) {}

	bool error::is(uint32_t code)
	{
		return mErrorCode == code;
	}

	std::string error::err() const
	{
		return mValue;
	}

	error::operator bool() const
	{
		return err().size() != 0 || mErrorCode != 0;
	}
}