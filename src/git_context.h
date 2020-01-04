#ifndef GIT_CONTEXT_H_
#define GIT_CONTEXT_H_

#include <string>
#include <sys/types.h>
#include <map>
#include <memory>
#include <mutex>
#include "git_wrappers.h"

struct fuse_operations;
struct fuse_conn_info;
struct fuse_config;
struct MountContext;
class FSRoot;
struct FileInfo;

struct GitContext
{
	GitContext(MountContext & mountcontext, const fuse_conn_info *info, const fuse_config *config);
	~GitContext();

	static GitContext * get();
	static const fuse_operations* fuseOperations();

	fuse_conn_info *_fuse_conn_info;
	fuse_config *_fuse_config;

	GitRepository repository;
	std::string branch;
	std::string commit;
	bool debug;
	uid_t uid;
	gid_t gid;
	mode_t umask;
	time_t atime;

	std::shared_ptr<FSRoot> root;

	using FileInfoKey = decltype(fuse_file_info::fh);
	using FileInfoMap = std::map<FileInfoKey, std::shared_ptr<FileInfo>>;
	FileInfoMap fileInfo;
	FileInfoKey fileInfoKey;
	std::mutex fileInfoLock;

	static void* _fuse_init(fuse_conn_info *conn, fuse_config *cfg);
	static void _fuse_destroy(void *private_data);

	static int _fuse_getattr(const char *path, struct stat *st, fuse_file_info *fi);
	int fuse_getattr(std::string_view path, struct stat *st, fuse_file_info *fi);
	static int _fuse_readlink(const char *path, char *buf, size_t bufsize);
	int fuse_readlink(std::string_view path, char *buf, size_t bufsize);

	static int _fuse_opendir(const char *path, fuse_file_info *fi);
	static int _fuse_readdir(const char *path, void *fusebuf, fuse_fill_dir_t fillfunc, off_t offset, fuse_file_info *fi, fuse_readdir_flags flags);
	static int _fuse_releasedir(const char *path, struct fuse_file_info *fi);
	int fuse_opendir(std::string_view path, struct fuse_file_info *fi);
	int fuse_readdir(void *fusebuf, fuse_fill_dir_t fillfunc, off_t offset, struct fuse_file_info *fi, fuse_readdir_flags flags);
	int fuse_releasedir(struct fuse_file_info *fi);
};

#define IN_CONTEXT(x,func) \
	GitContext * x = GitContext::get(); \
	if (x == nullptr) return -ENOENT \
	return x->func

#endif // GIT_CONTEXT_H_
