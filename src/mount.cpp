#include <cstdlib>
#include <iostream>
#include <git2.h>
#include <fuse.h>
#include "gitfs.h"
#include "command_line.h"
#include "mount_context.h"
#include "git_context.h"

namespace
{

enum cmdline_option_keys
{
	KEY_FUSE_NONOPT  = FUSE_OPT_KEY_NONOPT,
	KEY_FUSE_OPT     = FUSE_OPT_KEY_OPT,
	KEY_FOREGROUND   = 0,
	KEY_DEBUG_GITFS,
	KEY_DEBUG_FUSE,
	KEY_BRANCH,
	KEY_COMMIT,
	KEY_READONLY,
	KEY_READWRITE,
};

int mount_main_cmdline(int key, const std::string_view & argument, const std::string_view & value, void *data)
{
	struct MountContext *context = reinterpret_cast<MountContext*>(data);

	switch (cmdline_option_keys(key))
	{
		case KEY_FUSE_OPT:
			return 1;

		case KEY_FUSE_NONOPT:
			if (context->repopath.empty())
			{
				char * repopath = realpath(value.data(), nullptr);
				if (!repopath)
				{
					std::perror("gitfs mount: repopath invalid");
					return -1;
				}
				context->repopath = repopath;
				free(repopath);
				return 0;
			}
			return 1;

		case KEY_FOREGROUND:
			context->foreground = true;
			return 1;

		case KEY_DEBUG_GITFS:
			context->debug = true;
			return 0;

		case KEY_DEBUG_FUSE:
			context->debug = true;
			context->foreground = true;
			return 1;

		case KEY_BRANCH:
			context->branch = value;
			return 0;

		case KEY_COMMIT:
			context->commit = value;
			return 0;

		case KEY_READONLY:
			context->readwrite = false;
			return 1;

		case KEY_READWRITE:
			context->readwrite = true;
			return 1;
	}

	return 1;
}

void mount_main_cmdhelp()
{
	std::cout << "usage: gitfs mount <path/to/git/repo> <mountpoint> [options]" << std::endl
			<< std::endl
			<< "general options:" << std::endl
			<< "    -h   --help            print help" << std::endl
			<< "    -f                     run in foreground" << std::endl
			<< "    -d                     enable debugging (implies -f)" << std::endl
			<< "    -o opt[,opt...]        mount options" << std::endl
			<< std::endl
			<< "GITFS options:" << std::endl
			<< "    -o branch=STR          mount the tip of a specific branch" << std::endl
			<< "    -o commit=STR          mount a specific commit or tag" << std::endl;
}

int mount_main(int argc, char **argv)
{
	CommandLine cmdline(argc, argv);
	MountContext mountcontext;

	cmdline.add(KEY_FOREGROUND, "-f");
	cmdline.add(KEY_DEBUG_GITFS, "-d");
	cmdline.add(KEY_DEBUG_FUSE, "debug");
	cmdline.add(KEY_READONLY, "ro");
	cmdline.add(KEY_READWRITE, "rw");
	cmdline.add(KEY_BRANCH, "branch=");
	cmdline.add(KEY_COMMIT, "commit=");
	cmdline.parse(&mount_main_cmdline, &mountcontext);

	if (cmdline.hasHelp())
	{
		std::cout << argv[0] << " - " << gitfs_mount.description << std::endl << std::endl;
		mount_main_cmdhelp();

		std::cout << std::endl << "FUSE options:" << std::endl;
		fuse_lib_help(cmdline.args());

		return EXIT_SUCCESS;
	}

	if (mountcontext.repopath.empty())
	{
		std::cerr << "No repository supplied to mount" << std::endl;
		return EXIT_SUCCESS;
	}

	if (mountcontext.debug && !mountcontext.foreground)
	{
		mountcontext.foreground = true;
		cmdline.pushArg("-f");
	}
	cmdline.pushArg("-odefault_permissions");
	cmdline.pushArg("-onoatime");
	cmdline.pushArg("-oauto_unmount");

	if (!git_libgit2_init())
	{
		std::cerr << "error initializing libgit" << std::endl;
		return EXIT_FAILURE;
	}

	int giterr = git_repository_open(&mountcontext.repository, mountcontext.repopath.c_str());
	if (giterr != 0)
	{
		if (giterr == GIT_ENOTFOUND)
			std::cerr << "gitfs mount: no git repository found at location" << std::endl;
		else
			std::cerr << "gitfs mount: error opening git repository at location" << std::endl;

		return EXIT_FAILURE;
	}

	if (mountcontext.debug)
		std::cout << "mount: located and opened at " << git_repository_path(mountcontext.repository) << std::endl;

	int result = fuse_main(cmdline.args()->argc, cmdline.args()->argv, GitContext::fuseOperations(), &mountcontext);
	if (result == 2) // No mount point specified
		mount_main_cmdhelp();

	git_libgit2_shutdown();
	return result;
}

} // namespace

struct gitfs_function gitfs_mount =
{
		.description = "Mount a local git repository as a filesystem",
		.main = &mount_main
};
