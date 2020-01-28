#include <cstdlib>
#include <iostream>
#include "gitfs.h"

namespace
{

int umount_main(int argc, char **argv)
{
	std::cerr << "Not implemented yet. Use \"fusermount3 -u path\"" << std::endl;
	return EXIT_FAILURE;
}

} // namespace

struct gitfs_function gitfs_umount =
{
		.description = "Unmount a previously mounted git repository",
		.main = &umount_main
};
