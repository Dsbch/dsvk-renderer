#pragma once

#include <pch.h>

namespace engine {
	class error 
	{
	private:
		std::string mValue;
	public:
		template <typename... T>
		error(const std::string& fmtStr, T&&... args);

		error();

		std::string err() const;
		operator bool() const;
	};

	template<typename ...T>
	inline error::error(const std::string& fmtStr, T&&... args)
		: mValue(fmt::format(fmt::runtime(fmtStr), std::forward<T>(args)...))
	{}

	template<typename T>
	class withError
	{
	public:
		withError(const T& value)
		{
			new (&mStorage.value) T(value);
			mHasValue = true;
		}

		withError(T&& value)
		{
			new (&mStorage.value) T(std::move(value));
			mHasValue = true;
		}

		withError(const error& err)
		{
			new (&mStorage.err) error(err);
			mHasValue = false;
		}

		withError(error&& err)
		{
			new (&mStorage.err) error(std::move(err));
			mHasValue = false;
		}

		withError(const withError& other)
		{
			mHasValue = other.mHasValue;
			if (mHasValue)
				new (&mStorage.value) T(other.mStorage.value);
			else
				new (&mStorage.err) error(other.mStorage.err);
		}

		withError(withError&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
		{
			mHasValue = other.mHasValue;
			if (mHasValue)
				new (&mStorage.value) T(std::move(other.mStorage.value));
			else
				new (&mStorage.err) error(std::move(other.mStorage.err));
		}

		withError& operator=(const withError& other)
		{
			if (this == &other) return *this;

			// Clean up current value
			destroy();

			mHasValue = other.mHasValue;
			if (mHasValue)
				new (&mStorage.value) T(other.mStorage.value);
			else
				new (&mStorage.err) error(other.mStorage.err);

			return *this;
		}

		withError& operator=(withError&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
		{
			if (this == &other) return *this;

			// Clean up current value
			destroy();

			mHasValue = other.mHasValue;
			if (mHasValue)
				new (&mStorage.value) T(std::move(other.mStorage.value));
			else
				new (&mStorage.err) error(std::move(other.mStorage.err));

			return *this;
		}

		~withError()
		{
			destroy();
		}

		bool has_value() const noexcept { return mHasValue; }
		operator bool() const noexcept { return has_value(); }

		T& value()
		{
			if (!mHasValue) throw std::logic_error("Accessing value when error is present");
			return mStorage.value;
		}

		const T& value() const 
		{
			if (!mHasValue) throw std::logic_error("Accessing value when error is present");
			return mStorage.value;
		}

		std::string& errorString() const
		{
			if (mHasValue) throw std::logic_error("Accessing error when value is present");
			return mStorage.err.err();
		}

		error& err()
		{
			if (mHasValue) throw std::logic_error("Accessing error when value is present");
			return mStorage.err;
		}

		const error& err() const 
		{
			if (mHasValue) throw std::logic_error("Accessing error when value is present");
			return mStorage.err;
		}

	private:
		void destroy()
		{
			if (mHasValue)
				mStorage.value.~T();
			else
				mStorage.err.~error();
		}

		union Storage {
			T value;
			error err;

			Storage()
			{}
			~Storage()
			{}
		} mStorage;

		bool mHasValue = false;
	};
}
