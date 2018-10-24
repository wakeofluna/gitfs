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
	git_oid oid = {};
	int len;
	GET_CONTEXT(context);

	if (context->debug)
		printf("%s: request for path '%s'\n", __func__, path ? path : "(null)");

	if (!path || path[0] != '/')
		return -ENOENT;

	now = get_current_time();
	if (now > context->root_stat_refresh)
	{
		if (stat(git_repository_path(context->repository), &context->root_stat_cache) == -1)
			return -errno;

		context->root_stat_cache.st_mode &= ~S_IFMT;
		if (!context->writable)
			context->root_stat_cache.st_mode &= ~(S_IWRITE | S_IWGRP | S_IWOTH);

		context->root_stat_refresh = now + 30;
	}

	if (path[1] == 0)
	{
		*st = context->root_stat_cache;
		st->st_mode |= S_IFDIR;
		return 0;
	}

	len = strlen(path+1);
	if (len >= GIT_OID_MINPREFIXLEN && git_oid_fromstrn(&oid, path+1, len) == 0)
	{
		git_object *object;

		if (context->debug)
			printf("%s: '%s' is a possible oid, checking...\n", __func__, path+1);

		if (git_object_lookup_prefix(&object, context->repository, &oid, len, GIT_OBJ_ANY) == 0)
		{
			git_otype otype;
			otype = git_object_type(object);

			if (context->debug)
			{
				char buf[GIT_OID_HEXSZ+1];
				git_oid_nfmt(buf, sizeof(buf), git_object_id(object));
				printf("%s: '%s' is object of type %d\n", __func__, buf, (int)otype);
			}

			if (otype == GIT_OBJ_BLOB)
				st->st_mode |= S_IFREG;
			else
				st->st_mode |= S_IFDIR;

			git_object_free(object);
			return 0;
		}
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

static int gitfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t filler_offset, struct fuse_file_info *file_info)
{
	GET_CONTEXT(context);

	if (context->debug)
		printf("%s: request for path '%s' with flags 0x%x\n", __func__, path, file_info->flags);

	(*filler)(buf, ".", NULL, 0);
	(*filler)(buf, "..", NULL, 0);

	if (strcmp(path, "/") == 0)
	{
		int retval;
		git_reference_iterator *iter = NULL;
		char *last_dir = NULL;
		size_t last_dir_len = 0;

		retval = git_reference_iterator_new(&iter, context->repository);
		if (retval != GIT_OK)
			return -EIO;

		while (1)
		{
			git_reference *ref;
			const char *shorthand;
			const char *slash;

			retval = git_reference_next(&ref, iter);
			if (retval != GIT_OK)
				break;

			shorthand = git_reference_shorthand(ref);
			slash = strchr(shorthand, '/');
			if (slash)
			{
				size_t len = slash - shorthand;
				if (last_dir == NULL || len != last_dir_len || memcmp(last_dir, shorthand, len) != 0)
				{
					free(last_dir);
					last_dir = strndup(shorthand, len);
					last_dir_len = len;
					(*filler)(buf, last_dir, NULL, 0);
					if (context->debug)
						printf("Adding %s as path %s\n", shorthand, last_dir);
				}
				else
				{
					if (context->debug)
						printf("Skipping %s, path already handled\n", shorthand);
				}
			}
			else
			{
				if (context->debug)
					printf("Adding %s\n", shorthand);
				(*filler)(buf, shorthand, NULL, 0);
			}

			git_reference_free(ref);
		}

		git_reference_iterator_free(iter);
		free(last_dir);

		if (retval != GIT_ITEROVER)
		{
			printf("%s: error iterating over branches: %s\n", __func__, giterr_last()->message);
			return -EIO;
		}

		return 0;
	}

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
