#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fuse.h>
#include <git2.h>
#include "gitfs.h"

#define MOUNT_OPT(n,k,v) { n, offsetof(struct mount_options, k), v }

enum cmdline_option_keys
{
	KEY_FUSE_NONOPT  = FUSE_OPT_KEY_NONOPT,
	KEY_FUSE_OPT     = FUSE_OPT_KEY_OPT,
	KEY_HELP         = 0,
	KEY_VERSION,
	KEY_FOREGROUND,
	KEY_DEBUG,
	KEY_READONLY,
	KEY_READWRITE
};

static struct fuse_opt cmdline_options[] =
{
		MOUNT_OPT("branch=%s", branch, 0),
		MOUNT_OPT("commit=%s", commit, 0),

		FUSE_OPT_KEY("-h", KEY_HELP),
		FUSE_OPT_KEY("--help", KEY_HELP),
		FUSE_OPT_KEY("-V", KEY_VERSION),
		FUSE_OPT_KEY("--version", KEY_VERSION),
		FUSE_OPT_KEY("-f", KEY_FOREGROUND),
		FUSE_OPT_KEY("-d", KEY_DEBUG),
		FUSE_OPT_KEY("debug", KEY_DEBUG),
		FUSE_OPT_KEY("ro", KEY_READONLY),
		FUSE_OPT_KEY("rw", KEY_READWRITE),

		{ NULL }
};

static void mount_help(const char *command)
{
	fprintf(stderr, "%s: %s\n", command, gitfs_mount.description);
	fprintf(stderr,
			"\nusage: gitfs mount <path/to/git/repo> <mountpoint> [options]\n"
			"\ngeneral options:\n"
			"    -o opt[,opt...]        mount options\n"
			"    -h   --help            print help\n"
			"    -V   --version         print version\n"
			"\nGITFS options:\n"
			"    -o branch=STR          mount the tip of a specific branch\n"
			"    -o commit=STR          mount a specific commit or tag\n"
			"\n");
}

static int mount_parse_opts(void *data, const char *arg, int key, struct fuse_args *outargs)
{
	struct mount_options *options = (struct mount_options*)data;

	switch ((enum cmdline_option_keys)key)
	{
		case KEY_FUSE_OPT:
			return 1;

		case KEY_FUSE_NONOPT:
			if (!options->repopath)
			{
				options->repopath = realpath(arg, NULL);
				if (!options->repopath)
				{
					perror("gitfs mount: repopath invalid");
					return -1;
				}
				return 0;
			}
			return 1;

		case KEY_HELP:
			options->skip_check = 1;
			mount_help(outargs->argv[0]);
			return fuse_opt_add_arg(outargs, "-ho");

		case KEY_VERSION:
			options->skip_check = 1;
			print_version();
			return 1;

		case KEY_FOREGROUND:
			options->foreground = 1;
			return 1;

		case KEY_DEBUG:
			options->debug = 1;
			options->foreground = 1;
			return 1;

		case KEY_READONLY:
			options->readwrite = 0;
			return 1;

		case KEY_READWRITE:
			options->readwrite = 1;
			return 1;
	}

	fprintf(stderr, "gitfs mount: unhandled option: (%d) %s\n", key, arg);
	return -1;
}

static int check_options(const struct mount_options *options)
{
	int retval = 0;

	if (!options->repopath)
	{
		fprintf(stderr, "gitfs mount: missing required argument: /path/to/git/repo\n");
		retval = -1;
	}

	return retval;
}

static void free_options(struct mount_options *options)
{
	free(options->repopath);
	free(options->branch);
	free(options->commit);
}

static int mount_main(int argc, char **argv)
{
	struct mount_options options = {};
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	int retval = 0;

	if (retval == 0)
		retval = fuse_opt_parse(&args, &options, cmdline_options, &mount_parse_opts);

	if (retval == 0 && !options.skip_check)
		retval = check_options(&options);

	if (retval == 0)
		retval = fuse_opt_add_arg(&args, "-osubtype=gitfs");

	if (retval == 0 && options.repopath != NULL)
	{
		int len = strlen(options.repopath);
		char *buf = malloc(len + 10);
		if (buf)
		{
			snprintf(buf, len + 10, "-ofsname=%s", options.repopath);
			retval = fuse_opt_add_arg(&args, buf);
			free(buf);
		}
	}

	if (retval == 0)
		retval = fuse_main(args.argc, args.argv, &gitfs_operations, &options);

	free_options(&options);
	return retval == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

struct gitfs_function gitfs_mount =
{
		.description = "Mount a local git repository as a filesystem",
		.main = &mount_main
};
