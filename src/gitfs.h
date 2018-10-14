#ifndef GITFS_H_
#define GITFS_H_

#define GITFS_VERSION_MAJOR 1
#define GITFS_VERSION_MINOR 0
#define GITFS_VERSION ((GITFS_VERSION_MAJOR << 8) | GITFS_VERSION_MINOR)

void print_version(void);

enum mount_option_flag
{
	/* Mount options that require an argument */
	MOUNT_REPOPATH,
	MOUNT_MOUNTPOINT,
	MOUNT_OPTION_MAX_VALUE,

	/* Mount options that do not have an argument (flags) */
	MOUNT_DEBUG,
	MOUNT_FOREGROUND,
	MOUNT_READONLY,
	MOUNT_READWRITE,
	MOUNT_OPTION_MAX
};

struct mount_options
{
	char *value[MOUNT_OPTION_MAX_VALUE];
	char flag[MOUNT_OPTION_MAX];
};

struct gitfs_function
{
	const char *description;
	int (*main)(int argc, char **argv);
	void (*help)(const char *command);
};

extern struct gitfs_function gitfs_mount;
extern struct gitfs_function gitfs_umount;

#endif // GITFS_H_
