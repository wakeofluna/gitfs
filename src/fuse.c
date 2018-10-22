#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <fuse.h>
#include <git2.h>
#include "gitfs.h"
#include "utils.h"

struct gitfs_context
{
	struct git_repository *repository;

	char *branch;
	char *commit;
	int debug;
	int writable;

	struct stat root_stat_cache;
	time_t root_stat_refresh;
};

#define GET_CONTEXT(x) \
	struct gitfs_context *x; \
	x = (struct gitfs_context*)fuse_get_context()->private_data; \
	if (x == NULL) return -ENOENT

static void *gitfs_init(struct fuse_conn_info *conn)
{
	struct fuse_context *fuse_context = fuse_get_context();
	struct mount_context *mount_context = (struct mount_context *)fuse_context->private_data;
	struct gitfs_context *context;

	context = malloc(sizeof(struct gitfs_context));
	if (!context)
	{
		perror("init: malloc");
		fuse_exit(fuse_context->fuse);
		return NULL;
	}

	memset(context, 0, sizeof(*context));

#define TAKE(x, y) do { (x) = (y); (y) = NULL; } while (0)
	TAKE(context->repository, mount_context->repository);
	TAKE(context->branch, mount_context->branch);
	TAKE(context->commit, mount_context->commit);
	context->debug = mount_context->debug;
	context->writable = mount_context->readwrite;
#undef TAKE

	if (context->debug)
		printf("%s: created new context\n", __func__);

	return context;
}

static void gitfs_destroy(void *context_)
{
	struct gitfs_context *context = (struct gitfs_context *)context_;

	if (!context)
		return;

	if (context->debug)
		printf("%s: freeing context\n", __func__);

	git_repository_free(context->repository);
	free(context->branch);
	free(context->commit);
}

static int gitfs_getattr(const char *path, struct stat *st)
{
	time_t now;
	GET_CONTEXT(context);

	if (context->debug)
		printf("%s: request for path '%s'\n", __func__, path);

	now = get_current_time();
	if (now > context->root_stat_refresh)
	{
		if (stat(git_repository_path(context->repository), &context->root_stat_cache) == -1)
			return -errno;

		context->root_stat_refresh = now + 30;
	}

	if (strcmp(path, "/") == 0)
	{
		*st = context->root_stat_cache;
		if (!context->writable)
			st->st_mode &= ~(S_IWRITE | S_IWGRP | S_IWOTH);

		return 0;
	}

	return -ENOENT;
}

static int gitfs_opendir(const char *path, struct fuse_file_info *file_info)
{
	GET_CONTEXT(context);

	if (context->debug)
		printf("%s: request for path '%s'\n", __func__, path);

	return 0;
}

static int gitfs_readdir(const char *path, void *data, fuse_fill_dir_t filler, off_t filler_offset, struct fuse_file_info *file_info)
{
	GET_CONTEXT(context);

	if (context->debug)
		printf("%s: request for path '%s'\n", __func__, path);

	return 0;
}

static int gitfs_releasedir(const char *path, struct fuse_file_info *file_info)
{
	GET_CONTEXT(context);

	if (context->debug)
		printf("%s: request for path '%s'\n", __func__, path);

	return 0;
}

struct fuse_operations gitfs_operations =
{
		.init = &gitfs_init,
		.destroy = &gitfs_destroy,

		.getattr = &gitfs_getattr,

		.opendir = &gitfs_opendir,
		.readdir = &gitfs_readdir,
		.releasedir = &gitfs_releasedir,
};
