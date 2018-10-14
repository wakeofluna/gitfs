#include <stdlib.h>
#include <stdio.h>
#include "gitfs.h"

static void umount_help(const char *command)
{
	print_version();
	printf("\n%s: %s\n", command, gitfs_umount.description);
	printf("\nUsage:\n");
	printf(" %s -h\n", command);
	printf(" %s <mountpoint>\n", command);
}

static int umount_main(int argc, char **argv)
{
	return EXIT_SUCCESS;
}

struct gitfs_function gitfs_umount = {
		.description = "Unmount a previously mounted git repository",
		.main = &umount_main,
		.help = &umount_help
};
