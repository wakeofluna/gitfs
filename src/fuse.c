#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fuse.h>
#include <git2.h>
#include "gitfs.h"

struct gitfs_context
{
	git_repository *repository;

	char *branch;
	char *commit;

	int debug;
	int writable;
};

#define GET_CONTEXT(x) struct gitfs_context *x = (struct gitfs_context*)fuse_get_context()->private_data

static void *gitfs_init(struct fuse_conn_info *conn)
{
	struct fuse_context *fuse_context = fuse_get_context();
	struct mount_context *mount_context = (struct mount_context *)fuse_context->private_data;
	struct gitfs_context *context = malloc(sizeof(struct gitfs_context));

	if (!context)
	{
		perror("gitfs: malloc");
		fuse_exit(fuse_context->fuse);
	}

	memset(context, 0, sizeof(*context));

#define TAKE(x, y) do { (x) = (y); (y) = NULL; } while (0)
	TAKE(context->repository, mount_context->repository);
	TAKE(context->branch, mount_context->branch);
	TAKE(context->commit, mount_context->commit);
	context->debug = mount_context->debug;
	context->writable = mount_context->readwrite;

	if (context->debug)
		printf("%s: created new context\n", __func__);

	return context;
}

static void gitfs_destroy(void *context_)
{
	struct gitfs_context *context = (struct gitfs_context *)context_;

	if (context->debug)
		printf("%s: freeing context\n", __func__);

	git_repository_free(context->repository);
	free(context->branch);
	free(context->commit);
}

struct fuse_operations gitfs_operations =
{
		.init = &gitfs_init,
		.destroy = &gitfs_destroy,
};
