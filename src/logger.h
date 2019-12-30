#ifndef LOGGER_H_
#define LOGGER_H_

#include <atomic>
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

class Logger
{
public:
	struct Retval {};
	static constexpr Retval retval;
	struct RemoveRetval {};
	static constexpr RemoveRetval removeRetval;

	Logger(const int & retvalRef, bool autoOutput = false, size_t reserve = 128);
	~Logger();

	Logger& operator<< (char chr);
	Logger& operator<< (int num);
	Logger& operator<< (unsigned int num);
	Logger& operator<< (long num);
	Logger& operator<< (unsigned long num);
	Logger& operator<< (const char * str);
	Logger& operator<< (const std::string & str);
	Logger& operator<< (const std::string_view & str);
	Logger& operator<< (Retval);
	Logger& operator<< (RemoveRetval);

	Logger& operator>> (std::ostream & stream);

	void outputRetval();

private:
	std::vector<char> buf;
	const int & retvalRef;
	int retvalVal;
	bool retvalPrimed;
	bool needWrite;
};

#endif // LOGGER_H_
