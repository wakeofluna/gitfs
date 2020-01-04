#include <algorithm>
#include <array>
#include <iostream>
#include <iterator>
#include <charconv>
#include <cstring>
#include "logger.h"

namespace
{
	template <typename T>
	void toString(Logger & logger, T num)
	{
		std::array<char,24> tmp;
		auto [ptr,ec] = std::to_chars(tmp.data(), tmp.data() + tmp.size(), num);
		if (ec == std::errc())
		{
			std::string_view result(tmp.data(), ptr - tmp.data());
			logger << result;
		}
	}
}

Logger::Logger(const int & retvalRef, bool autoOutput, size_t reserve) : retvalRef(retvalRef)
{
	buf.reserve(reserve);
	retvalVal = 0;
	retvalPrimed = false;
	needWrite = autoOutput;
}

Logger::~Logger()
{
	if (needWrite)
		operator>> (std::cout);
}

Logger& Logger::operator<< (char chr)
{
	if (retvalPrimed)
		outputRetval();

	if (retvalVal == 0)
		buf.push_back(chr);

	return *this;
}

Logger& Logger::operator<< (const char * str)
{
	if (retvalPrimed)
		outputRetval();

	if (retvalVal == 0)
	{
		if (str == nullptr)
			str = "(null)";

		for (const char *chr = str; *chr; ++chr)
			buf.push_back(*chr);
	}

	return *this;
}

Logger& Logger::operator<< (int num)
{
	if (retvalPrimed)
		outputRetval();

	if (retvalVal == 0)
		toString(*this, num);

	return *this;
}

Logger& Logger::operator<< (unsigned int num)
{
	if (retvalPrimed)
		outputRetval();

	if (retvalVal == 0)
		toString(*this, num);

	return *this;
}

Logger& Logger::operator<< (long num)
{
	if (retvalPrimed)
		outputRetval();

	if (retvalVal == 0)
		toString(*this, num);

	return *this;
}

Logger& Logger::operator<< (unsigned long num)
{
	if (retvalPrimed)
		outputRetval();

	if (retvalVal == 0)
		toString(*this, num);

	return *this;
}

Logger& Logger::operator<< (const std::string & str)
{
	if (retvalPrimed)
		outputRetval();

	if (retvalVal == 0 && !str.empty())
		std::copy(str.cbegin(), str.cend(), std::back_inserter(buf));

	return *this;
}

Logger& Logger::operator<< (const std::string_view & str)
{
	if (retvalPrimed)
		outputRetval();

	if (retvalVal == 0 && !str.empty())
		std::copy(str.cbegin(), str.cend(), std::back_inserter(buf));

	return *this;
}

Logger& Logger::operator<< (Retval)
{
	retvalPrimed = true;
	return *this;
}

Logger& Logger::operator<< (RemoveRetval)
{
	retvalPrimed = false;
	retvalVal = 0;
	return *this;
}

Logger& Logger::operator>> (std::ostream & stream)
{
	if (retvalPrimed)
		outputRetval();

	if (!buf.empty())
	{
		if (buf.back() != '\n')
			buf.push_back('\n');
		stream.write(buf.data(), buf.size());
	}

	needWrite = false;
	return *this;
}

void Logger::outputRetval()
{
	retvalPrimed = false;
	retvalVal = 0;

	if (retvalRef == 0)
		operator<< (std::string_view(" -> OK"));
	else if (retvalRef < 0)
		operator<< (std::string_view(" -> ")) << std::strerror(-retvalRef);
	else
		operator<< (std::string_view(" -> ")) << retvalRef;

	retvalVal = retvalRef < 0 ? retvalRef : 0;
}
