#ifndef COMMAND_LINE_H_
#define COMMAND_LINE_H_

#include <functional>

struct fuse_args;

class CommandLine
{
public:
	using Callback = std::function<int(int key, const std::string_view & argument, const std::string_view & value, void *context)>;

public:
	CommandLine(int argc, char **argv);
	~CommandLine();

	void pushArg(const char *arg);
	struct fuse_args * args() const;

	void add(int key, const char *option);
	int parse(const Callback & callback, void *context);

	inline bool hasHelp() const { return mOptionHelp; }

private:
	static int parseCallback(void *data, const char *arg, int key, struct fuse_args *outargs);

	struct Private;
	Private *mPrivate;
	int mMaxKey;

	bool mOptionHelp;
};

#endif // COMMAND_LINE_H_
