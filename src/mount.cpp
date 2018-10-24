#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fuse.h>
#include <git2.h>
#include "gitfs.h"
#include "utils.h"

#define MOUNT_OPT(n,k,v) { n, offsetof(struct mount_context, k), v }

enum cmdline_option_keys
{
	KEY_FUSE_NONOPT  = FUSE_OPT_KEY_NONOPT,
	KEY_FUSE_OPT     = FUSE_OPT_KEY_OPT,
	KEY_HELP         = 0,
	KEY_VERSION,
	KEY_FOREGROUND,
	KEY_DEBUG_GITFS,
	KEY_DEBUG_FUSE,
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
		FUSE_OPT_KEY("-d", KEY_DEBUG_GITFS),
		FUSE_OPT_KEY("debug", KEY_DEBUG_FUSE),
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
	struct mount_context *context = (struct mount_context*)data;

	switch ((enum cmdline_option_keys)key)
	{
		case KEY_FUSE_OPT:
			return 1;

		case KEY_FUSE_NONOPT:
			if (!context->repopath)
			{
				context->repopath = realpath(arg, NULL);
				if (!context->repopath)
				{
					perror("gitfs mount: repopath invalid");
					return -1;
				}
				return 0;
			}
			return 1;

		case KEY_HELP:
			context->skip_check = 1;
			mount_help(outargs->argv[0]);
			return fuse_opt_add_arg(outargs, "-ho");

		case KEY_VERSION:
			context->skip_check = 1;
			print_version();
			return 1;

		case KEY_FOREGROUND:
			context->foreground = 1;
			return 1;

		case KEY_DEBUG_GITFS:
			context->debug = 1;
			context->foreground = 1;
			return fuse_opt_add_arg(outargs, "-f");

		case KEY_DEBUG_FUSE:
			context->debug = 1;
			context->foreground = 1;
			return 1;

		case KEY_READONLY:
			context->readwrite = 0;
			return 1;

		case KEY_READWRITE:
			context->readwrite = 1;
			return 1;
	}

	fprintf(stderr, "gitfs mount: unhandled option: (%d) %s\n", key, arg);
	return -1;
}

static int check_context(struct mount_context *context)
{
	int giterr;

	if (!context->repopath)
	{
		fprintf(stderr, "gitfs mount: missing required argument: /path/to/git/repo\n");
		return -1;
	}

	if (context->branch && context->commit)
	{
		fprintf(stderr, "gitfs mount: cannot provide both branch and commit\n");
		return -1;
	}

	if (context->debug)
		printf("mount: searching for git at '%s'...\n", context->repopath);

	giterr = git_repository_open_ext(&context->repository, context->repopath, 0, NULL);
	if (giterr != 0)
	{
		if (giterr == GIT_ENOTFOUND)
			fprintf(stderr, "gitfs mount: no git repository found at location\n");
		else
			fprintf(stderr, "gitfs mount: error opening git repository at location\n");
		return -1;
	}

	if (context->debug)
		printf("mount: located and opened at %s\n", git_repository_path(context->repository));

	return 0;
}

static void free_context(struct mount_context *context)
{
	git_repository_free(context->repository);

	free(context->repopath);
	free(context->branch);
	free(context->commit);
}

static int mount_main(int argc, char **argv)
{
	struct mount_context context = {};
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	int retval = 0;

	if (retval == 0)
		retval = fuse_opt_parse(&args, &context, cmdline_options, &mount_parse_opts);

	if (retval == 0 && !context.skip_check)
		retval = check_context(&context);

	if (retval == 0)
		retval = fuse_opt_add_arg(&args, "-osubtype=gitfs");

	if (retval == 0 && context.repopath != NULL)
	{
		int len = strlen(context.repopath);
		char *buf = (char*) malloc(len + 10);
		if (buf)
		{
			snprintf(buf, len + 10, "-ofsname=%s", context.repopath);
			retval = fuse_opt_add_arg(&args, buf);
			free(buf);
		}
	}

	if (retval == 0)
		retval = fuse_main(args.argc, args.argv, &gitfs_operations, &context);

	free_context(&context);
	return retval == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

struct gitfs_function gitfs_mount =
{
		.description = "Mount a local git repository as a filesystem",
		.main = &mount_main
};
