#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <fuse.h>
#include "gitfs.h"
#include "utils.h"

struct umount_options
{
	const char *mountpoint;
	int help;
	int version;
};

#define UMOUNT_OPT(n,k,v) { n, offsetof(struct umount_options, k), v }

static struct fuse_opt cmdline_options[] =
{
		UMOUNT_OPT("-h", help, 1),
		UMOUNT_OPT("--help", help, 1),
		UMOUNT_OPT("-V", version, 1),
		UMOUNT_OPT("--version", version, 1),
		{ nullptr }
};

static int umount_parse_opts(void *data, const char *arg, int key, struct fuse_args *outargs)
{
	struct umount_options *options = (struct umount_options *)data;

	switch (key)
	{
		default:
		case FUSE_OPT_KEY_OPT:
			std::fprintf(stderr, "gitfs umount: invalid option -- '%s'\n", arg);
			return -1;

		case FUSE_OPT_KEY_NONOPT:
			if (options->mountpoint == nullptr)
			{
				options->mountpoint = arg;
				return 0;
			}
			std::fprintf(stderr, "gitfs umount: invalid option -- '%s'\n", arg);
			return -1;
	}
}

static void umount_help(const char *command)
{
	std::fprintf(stderr, "%s: %s\n", command, gitfs_umount.description);
	std::fprintf(stderr,
			"\nusage: gitfs umount <mountpoint>\n"
			"\ngeneral options:\n"
			"    -h   --help            print help\n"
			"    -V   --version         print version\n"
			"\n");
}

static int umount_main(int argc, char **argv)
{
	struct umount_options options = {};
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	int retval = 0;

	if (retval == 0)
		retval = fuse_opt_parse(&args, &options, cmdline_options, &umount_parse_opts);

	if (retval == 0 && options.help)
	{
		umount_help(argv[0]);
		return EXIT_FAILURE;
	}

	if (retval == 0 && options.version)
	{
		print_version();
		return EXIT_FAILURE;
	}

	if (retval == 0 && !options.mountpoint)
	{
		std::fprintf(stderr, "gitfs umount: missing required argument: mountpoint\n");
		return EXIT_FAILURE;
	}

	if (retval == 0 && options.mountpoint)
	{
		static const char command_template[] = "fusermount -u %s";

		int len = std::strlen(options.mountpoint);
		const int command_len = len + sizeof(command_template);

		char *command = new char[command_len];
		if (command)
		{
			std::snprintf(command, command_len, command_template, options.mountpoint);
			command[command_len-1] = 0;

			retval = std::system(command);
			if (retval == -1)
				std::perror("gitfs umount: system");

			delete[] command;
		}
		else
		{
			std::perror("gitfs umount: malloc");
			retval = -1;
		}
	}

	return retval == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

struct gitfs_function gitfs_umount = {
		.description = "Unmount a previously mounted git repository",
		.main = &umount_main
};
