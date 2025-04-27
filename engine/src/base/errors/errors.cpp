#include <pch.h>
#include "errors.h"

namespace engine
{
	error::error() : mValue() {}

	std::string error::err() const
	{
		return mValue;
	}

	error::operator bool() const
	{
		return err().size() != 0;
	}
}