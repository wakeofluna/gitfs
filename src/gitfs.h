#ifndef GITFS_H_
#define GITFS_H_

#define GITFS_VERSION_MAJOR 1
#define GITFS_VERSION_MINOR 0
#define GITFS_VERSION ((GITFS_VERSION_MAJOR << 8) | GITFS_VERSION_MINOR)

struct git_repository;

void print_version(void);

struct mount_context
{
	/* Mount parameters */
	char *repopath;
	char *mountpoint;
	char *branch;
	char *commit;

	int skip_check;
	int foreground;
	int debug;
	int readwrite;

	/* Runtime information */
	struct git_repository *repository;
};

struct gitfs_function
{
	const char *description;
	int (*main)(int argc, char **argv);
};

struct fuse_operations;
extern struct fuse_operations gitfs_operations;
extern struct gitfs_function gitfs_mount;
extern struct gitfs_function gitfs_umount;

#endif // GITFS_H_
