#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "gitfs.h"

struct mount_options
{
	char *repopath;
	char *mountpoint;

	char debug;
	char foreground;
};

static void mount_help(const char *command)
{
	print_version();
	printf("\n%s: %s\n", command, gitfs_mount.description);
	printf("\nUsage:\n");
	printf(" %s -h\n", command);
	printf(" %s <path-to-git-repo> <mountpoint> [-d] [-f] [-o options]\n", command);
}

static int parse_option(struct mount_options *options, const char *key, int keylen, const char *value, int valuelen)
{
	if (keylen == 0)
		return 0;

	fprintf(stderr, "unknown option: -o %.*s\n", keylen, key);
	return -1;
}

static int parse_options(struct mount_options *options, const char *optionstring)
{
	const char *end;
	const char *key;
	const char *split;
	const char *key_end;
	const char *value;

	end = optionstring + strlen(optionstring);

	for (key = optionstring; key < end; key = split + 1)
	{
		split = memchr(key, ',', end - key);
		if (!split)
			split = end;

		key_end = memchr(key, '=', split - key);
		if (!key_end)
		{
			key_end = split;
			value = split;
		}
		else
		{
			value = key_end + 1;
		}

		if (parse_option(options, key, key_end - key, value, split - value) < 0)
			return -1;
	}

	return 0;
}

static int mount_main(int argc, char **argv)
{
	struct mount_options options = {};
	const char *optstring = "dhfo:";
	const char *fixed_args[3] = {};
	int has_help = 0;
	int opt;
	int i;

	for (i = 0; i < 3; ++i)
	{
		for (opt = getopt(argc, argv, optstring); opt != -1; opt = getopt(argc, argv, optstring))
		{
			switch (opt)
			{
				case 'd':
					options.debug = 1;
					options.foreground = 1;
					break;
				case 'f':
					options.foreground = 1;
					break;
				case 'h':
					has_help++;
					break;
				case 'o':
					if (parse_options(&options, optarg) == -1)
						return EXIT_FAILURE;
					break;
				case '?':
					return EXIT_FAILURE;
				default:
					assert(0 && "getopt switch case insufficient");
					return EXIT_FAILURE;
			}
		}

		if (optind < argc)
		{
			fixed_args[i] = argv[optind];
			++optind;
		}
	}

	if (has_help)
	{
		mount_help(argv[0]);
		return EXIT_SUCCESS;
	}

	if (!fixed_args[0])
		fprintf(stderr, "%s: missing required argument: path-to-git-repo\n", argv[0]);

	if (!fixed_args[1])
	{
		fprintf(stderr, "%s: missing required argument: mountpoint\n", argv[0]);
		return EXIT_FAILURE;
	}

	options.repopath = strdup(fixed_args[0]);
	options.mountpoint = strdup(fixed_args[1]);

	if (options.debug)
		printf("Mounting '%s' on '%s'...\n", options.repopath, options.mountpoint);

	free(options.mountpoint);
	free(options.repopath);

	return EXIT_SUCCESS;
}

struct gitfs_function gitfs_mount = {
		.description = "Mount a local git repository as a filesystem",
		.main = &mount_main,
		.help = &mount_help
};
