#include "errors.h"

core::error& core::error::operator=(const error& other)
{
	if (&other != this)
	{
		core::error tmp(other);
		this->mValue.swap(tmp.mValue);
	}

	return *this;
}

core::error& core::error::operator=(error&& other) noexcept
{
	mValue.swap(other.mValue);
	
	return *this;
}

core::error::error(error&& e) noexcept : mValue(std::move(e.mValue))
{
}

std::string core::error::err() const
{
	return mValue;
}

core::error::operator bool() const
{
	return err().size() != 0;
}
