#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <string_view>
#include "gitfs.h"

namespace
{

struct gitfs_command
{
	std::string_view name;
	const struct gitfs_function *function;

	bool operator== (const std::string_view & search) const
	{
		return name == search;
	}
};

constexpr gitfs_command commands[] = {
		{ "mount", &gitfs_mount },
		{ "umount", &gitfs_umount },
};

constexpr size_t nrCommands = sizeof(commands) / sizeof(*commands);
const gitfs_command * noCommand = commands + nrCommands;

}

int main(int argc, char **argv)
{
	std::string_view called_name(argv[0]);
	int result = EXIT_SUCCESS;

	size_t pos_start = called_name.rfind('/');
	++pos_start;

	size_t pos_end = called_name.find('.', pos_start);
	if (pos_end != called_name.npos)
	{
		std::string_view shortcut = called_name.substr(pos_start, pos_end - pos_start);
		const gitfs_command * command = std::find(commands, noCommand, shortcut);
		if (command != noCommand)
			return (*command->function->main)(argc, argv);
	}

	if (argc > 1)
	{
		const gitfs_command * command = std::find(commands, noCommand, argv[1]);
		if (command != noCommand)
			return (*command->function->main)(argc - 1, argv + 1);

		std::cout << "ERROR: unknown command: " << argv[1] << std::endl << std::endl;
		result = EXIT_FAILURE;
	}

	std::cout << "gitfs v" << GITFS_VERSION_MAJOR << '.' << GITFS_VERSION_MINOR << " - mounting a git repository as a filesystem" << std::endl;
	std::cout << "Subcommands:" << std::endl;

	size_t cmdlen = std::accumulate(commands, noCommand, size_t(0), [] (size_t lhs, const auto & rhs) { return std::max(lhs, rhs.name.size()); });
	for (const auto & command : commands)
		std::cout << "    " << std::setw(cmdlen) << command.name << " : " << command.function->description << std::endl;

	return result;
}
