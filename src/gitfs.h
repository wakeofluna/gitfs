#ifndef GITFS_H_
#define GITFS_H_

#include <string_view>
#include <string>

#define GITFS_VERSION_MAJOR 1
#define GITFS_VERSION_MINOR 0
#define GITFS_VERSION ((GITFS_VERSION_MAJOR << 8) | GITFS_VERSION_MINOR)

struct gitfs_function
{
	std::string_view description;
	int (*main)(int argc, char **argv);
};

extern struct gitfs_function gitfs_mount;
extern struct gitfs_function gitfs_umount;

#endif // GITFS_H_
