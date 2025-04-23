#include <pch.h>
#include "errors.h"

namespace engineCore
{
	error& error::operator=(const error& other)
	{
		if (&other != this)
		{
			error tmp(other);
			this->mValue.swap(tmp.mValue);
		}

		return *this;
	}

	error& error::operator=(error&& other) noexcept
	{
		mValue.swap(other.mValue);

		return *this;
	}

	error::error(error&& e) noexcept : mValue(std::move(e.mValue))
	{
	}

	std::string error::err() const
	{
		return mValue;
	}

	error::operator bool() const
	{
		return err().size() != 0;
	}
}