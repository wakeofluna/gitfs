#ifndef GIT_CONTEXT_H_
#define GIT_CONTEXT_H_

#include <string>

struct git_repository;
struct fuse_operations;
struct MountContext;

struct GitContext
{
	GitContext(MountContext & mountcontext);
	~GitContext();

	static GitContext * get();
	static fuse_operations * fuseOperations();

	git_repository *repository;
	std::string branch;
	std::string commit;
	bool debug;
	bool writable;

	static void * fuse_init(struct fuse_conn_info *conn, struct fuse_config *cfg);
	static void fuse_destroy(void *private_data);
};

#define GET_CONTEXT(x) \
	GitContext *x = GitContext::get(); \
	if (x == nullptr) return -ENOENT

#endif // GIT_CONTEXT_H_
