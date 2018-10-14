#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "gitfs.h"

struct mount_flag
{
	const char *name;
	const char *description;
	enum mount_option_flag index;
};

static const struct mount_flag mount_flags[] = {
		{ "ro", "Mount the repository readonly (default)", MOUNT_READONLY },
		{ "rw", "Mount the repository readwrite", MOUNT_READWRITE },
		{ "branch", "Mount the tip of a specific branch", MOUNT_BRANCH },
		{ "commit", "Mount a specific commit or tag", MOUNT_COMMIT },
		{ NULL }
};

static void mount_help(const char *command)
{
	const struct mount_flag *flag;
	int maxlen = 0;

	print_version();
	printf("\n%s: %s\n", command, gitfs_mount.description);
	printf("\nUsage:\n");
	printf(" %s -h\n", command);
	printf(" %s <path-to-git-repo> <mountpoint> [-d] [-f] [-o options]\n", command);

	for (flag = mount_flags; flag->name != NULL; ++flag)
	{
		int len = strlen(flag->name) + (flag->index < MOUNT_OPTION_MAX_VALUE ? 2 : 0);
		if (maxlen < len)
			maxlen = len;
	}

	printf("\nOptions:\n");
	printf(" -d %-*s  Enable debugging output (implies -f)\n", maxlen, "");
	printf(" -f %-*s  Run in foreground\n", maxlen, "");
	printf("\nMount options:\n");

	for (flag = mount_flags; flag->name != NULL; ++flag)
	{
		if (flag->index < MOUNT_OPTION_MAX_VALUE)
		{
			int len = strlen(flag->name);
			printf(" -o %s=X %*s %s\n", flag->name, maxlen - len - 2, "", flag->description);
		}
		else
		{
			printf(" -o %-*s  %s\n", maxlen, flag->name, flag->description);
		}
	}

	printf("\n");
}

static int parse_option(struct mount_options *options, const char *key, int keylen, const char *value, int valuelen)
{
	const struct mount_flag *flag;

	if (keylen == 0)
		return 0;

	for (flag = mount_flags; flag->name != NULL; ++flag)
	{
		int len = strlen(flag->name);
		if (len == keylen && memcmp(key, flag->name, len) == 0)
			break;
	}

	if (flag->name == NULL)
	{
		fprintf(stderr, "unknown option: -o %.*s\n", keylen, key);
		return -1;
	}

	if (flag->index < MOUNT_OPTION_MAX_VALUE && valuelen == 0)
	{
		fprintf(stderr, "missing required argument to option '%.*s'\n", keylen, key);
		return -1;
	}

	if (flag->index >= MOUNT_OPTION_MAX_VALUE && valuelen > 0)
	{
		fprintf(stderr, "unexpected argument to option '%.*s': %.*s\n", keylen, key, valuelen, value);
		return -1;
	}

	options->flag[flag->index] = 1;

	if (flag->index < MOUNT_OPTION_MAX_VALUE)
	{
		free(options->value[flag->index]);
		options->value[flag->index] = NULL;

		if (valuelen > 0)
		{
			char *block = (char*) malloc(valuelen + 1);
			if (block == NULL)
			{
				fprintf(stderr, "out of memory copying option value for '%.*s'\n", keylen, key);
				return -1;
			}

			memcpy(block, value, valuelen);
			block[valuelen] = '\0';
		}
	}

	return 0;
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

static int check_options(const struct mount_options *options, const char *argv0)
{
	int retval = 0;

	if (!options->flag[MOUNT_REPOPATH] || !options->value[MOUNT_REPOPATH])
	{
		fprintf(stderr, "%s: missing required argument: path-to-git-repo\n", argv0);
		retval = -1;
	}

	if (!options->flag[MOUNT_MOUNTPOINT] || !options->value[MOUNT_MOUNTPOINT])
	{
		fprintf(stderr, "%s: missing required argument: mountpoint\n", argv0);
		retval = -1;
	}

	return retval;
}

static void free_options(struct mount_options *options)
{
	int i;

	for (i = 0; i < MOUNT_OPTION_MAX_VALUE; ++i)
		free(options->value[i]);
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
					options.flag[MOUNT_DEBUG] = 1;
					/* no break */
				case 'f':
					options.flag[MOUNT_FOREGROUND] = 1;
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

	if (fixed_args[0])
	{
		options.value[MOUNT_REPOPATH] = strdup(fixed_args[0]);
		options.flag[MOUNT_REPOPATH] = 1;
	}

	if (fixed_args[1])
	{
		options.value[MOUNT_MOUNTPOINT] = strdup(fixed_args[1]);
		options.flag[MOUNT_MOUNTPOINT] = 1;
	}

	if (check_options(&options, argv[0]) < 0)
	{
		free_options(&options);
		return EXIT_FAILURE;
	}

	if (options.flag[MOUNT_DEBUG])
		printf("Mounting '%s' on '%s'...\n", options.value[MOUNT_REPOPATH], options.value[MOUNT_MOUNTPOINT]);

	free_options(&options);
	return EXIT_SUCCESS;
}

struct gitfs_function gitfs_mount = {
		.description = "Mount a local git repository as a filesystem",
		.main = &mount_main,
		.help = &mount_help
};
