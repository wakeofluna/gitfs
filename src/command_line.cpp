#include <fuse_opt.h>
#include "command_line.h"

namespace
{

struct CallbackContext
{
	CommandLine & cmdline;
	const CommandLine::Callback & function;
	void * context;
};

}

struct CommandLine::Private
{
	struct fuse_args args;
	std::vector<struct fuse_opt> options;
};

CommandLine::CommandLine(int argc, char **argv) : mPrivate(nullptr), mMaxKey(0), mOptionHelp(false)
{
	mPrivate = new Private();
	mPrivate->args = FUSE_ARGS_INIT(argc, argv);
	mPrivate->options.reserve(16);
}

CommandLine::~CommandLine()
{
	if (mPrivate)
	{
		fuse_opt_free_args(&mPrivate->args);
		delete mPrivate;
	}
}

void CommandLine::pushArg(const char *arg)
{
	fuse_opt_add_arg(&mPrivate->args, arg);
}

struct fuse_args * CommandLine::args() const
{
	return &mPrivate->args;
}

void CommandLine::add(int key, const char *option)
{
	mPrivate->options.push_back(FUSE_OPT_KEY(option, key));
	mMaxKey = std::max(mMaxKey, key);
}

int CommandLine::parse(const Callback & callback, void *context)
{
	mPrivate->options.push_back(FUSE_OPT_KEY("-h", mMaxKey+1));
	mPrivate->options.push_back(FUSE_OPT_KEY("--help", mMaxKey+1));
	mPrivate->options.push_back(FUSE_OPT_END);

	CallbackContext ctx{*this, callback, context};
	return fuse_opt_parse(&mPrivate->args, &ctx, mPrivate->options.data(), &parseCallback);
}

int CommandLine::parseCallback(void *data, const char *arg, int key, struct fuse_args *outargs)
{
	CallbackContext *context = reinterpret_cast<CallbackContext*>(data);

	if (key == context->cmdline.mMaxKey + 1)
	{
		context->cmdline.mOptionHelp = true;
		return 0;
	}

	if (key == FUSE_OPT_KEY_OPT)
	{
		return 1;
	}

	std::string_view stringarg(arg);
	if (key == FUSE_OPT_KEY_NONOPT)
	{
		return context->function(key, std::string_view(), stringarg, context->context);
	}

	size_t split = stringarg.find('=');
	if (split == stringarg.npos)
	{
		return context->function(key, stringarg, std::string_view(), context->context);
	}
	else
	{
		return context->function(key, stringarg.substr(0, split), stringarg.substr(split+1), context->context);
	}
}
