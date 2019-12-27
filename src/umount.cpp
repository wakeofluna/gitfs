#include <cstdlib>
#include "gitfs.h"

namespace
{

int umount_main(int argc, char **argv)
{
	return EXIT_SUCCESS;
}

} // namespace

struct gitfs_function gitfs_umount =
{
		.description = "Unmount a previously mounted git repository",
		.main = &umount_main
};
