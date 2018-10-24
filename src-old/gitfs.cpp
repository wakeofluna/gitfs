#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "gitfs.h"
#include "utils.h"

struct gitfs_command
{
	const char *name;
	const struct gitfs_function *function;
};

static const struct gitfs_command commands[] = {
		{ "mount", &gitfs_mount },
		{ "umount", &gitfs_umount },
		{ NULL }
};

static void print_usage(const char *argv0)
{
	const struct gitfs_command *command;
	int maxlen = 0;

	print_version();
	std::printf("\nUsage:\n");
	std::printf("    %s -h [command]\n", argv0);
	std::printf("    %s <command> -h\n", argv0);
	std::printf("    %s <command> ...\n", argv0);
	std::printf("\nMount, unmount or interact with a git repository\n");
	std::printf("\nOptions:\n");
	std::printf("    -h  Display help about the specified command\n");
	std::printf("\nList of commands:\n");

	for (command = commands; command->name != NULL; ++command)
	{
		int len = std::strlen(command->name);
		if (len > maxlen)
			maxlen = len;
	}

	for (command = commands; command->name != NULL; ++command)
	{
		std::printf("    %-*s  %s\n", maxlen, command->name, command->function->description);
	}

	std::printf("\n");
}

static int main_normal(int argc, char **argv)
{
	const struct gitfs_command *command;
	int idx;
	int has_help = 0;

	for (idx = 1; idx < argc; ++idx)
	{
		if (argv[idx][0] != '-')
			break;

		if (std::strcmp(argv[idx], "-h") == 0 || std::strcmp(argv[idx], "--help") == 0)
		{
			has_help = idx;
		}
		else if (std::strcmp(argv[idx], "-V") == 0 || std::strcmp(argv[idx], "--version") == 0)
		{
			print_version();
			return EXIT_SUCCESS;
		}
		else
		{
			std::fprintf(stderr, "%s: invalid option -- '%s'\n", argv[0], argv[idx]);
			return EXIT_FAILURE;
		}
	}

	if (idx == argc)
	{
		print_usage(argv[0]);
		return EXIT_SUCCESS;
	}

	for (command = commands; command->name != NULL; ++command)
	{
		if (std::strcmp(argv[idx], command->name) == 0)
			break;
	}

	if (command->name == NULL)
	{
		std::fprintf(stderr, "gitfs: invalid command: %s\n", argv[idx]);
		return EXIT_FAILURE;
	}

	assert(command->function->main && "gitfs_command does not have a main callback");

	if (has_help)
	{
		char *tmp = argv[has_help];
		int i;

		for (i = has_help; i < idx; ++i)
			argv[i] = argv[i+1];
		argv[idx] = tmp;

		--idx;
	}

	return (*command->function->main)(argc - idx, &argv[idx]);
}

static int main_shortcircuit(int argc, char **argv, const char *function, const char *function_end)
{
	const struct gitfs_command *command;
	const int function_len = function_end - function;

	for (command = commands; command->name != NULL; ++command)
	{
		int len = std::strlen(command->name);
		if (len == function_len && std::memcmp(function, command->name, len) == 0)
			break;
	}

	if (command->name == NULL)
	{
		std::fprintf(stderr, "gitfs: invalid command: %.*s\n", function_len, function);
		return EXIT_FAILURE;
	}

	assert(command->function->main && "gitfs_command does not have a main callback");

	return (*command->function->main)(argc, argv);
}

int main(int argc, char **argv)
{
	const char *pos_start;
	const char *pos_dot;

	if (argc < 1)
		return EXIT_FAILURE;

	pos_start = std::strrchr(argv[0], '/');
	if (pos_start == NULL)
		pos_start = argv[0];
	else
		++pos_start;

	pos_dot = std::strchr(pos_start, '.');
	if (pos_dot)
		return main_shortcircuit(argc, argv, pos_start, pos_dot);

	return main_normal(argc, argv);
}
