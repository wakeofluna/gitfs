#include <memory>
#include <utility>
#include <vector>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstring>

#include <git2.h>
#include <fuse.h>
#include <sys/stat.h>
#include <unistd.h>

#include "fs_root.h"
#include "git_context.h"
#include "mount_context.h"
#include "logger.h"

using FSEntryVector = std::vector<FSEntryPtr>;

struct FileInfo
{
	FSEntryVector stack;
};

namespace
{

constexpr fuse_operations _operations = {
	.init = &GitContext::_fuse_init,
	.destroy = &GitContext::_fuse_destroy,
	.getattr = &GitContext::_fuse_getattr,
	.readlink = &GitContext::_fuse_readlink,
	.opendir = &GitContext::_fuse_opendir,
	.readdir = &GitContext::_fuse_readdir,
	.releasedir = &GitContext::_fuse_releasedir,
};

template <typename ...ARGS>
int inContext(int (GitContext::*func)(ARGS ...args), ARGS ...args)
{
	int retval;

	GitContext * ctx = GitContext::get();
	if (ctx == nullptr)
	{
		std::cerr << "FUSE context gone?" << std::endl;
		return -ENOENT;
	}

	retval = -EIO;
	try
	{
		retval = (ctx->*func)(args...);
	}
	catch (std::exception & e)
	{
		std::cerr << "Internal error: " << e.what() << std::endl;
	}
	catch (const char * e)
	{
		std::cerr << "Internal error: " << e << std::endl;
	}
	catch (int e)
	{
		std::cerr << "GIT error: " << e << std::endl;
	}
	catch (...)
	{
		std::cerr << "Unknown internal error, such fail :(" << std::endl;
	}
	return retval;
}

int resolvePath(const FSEntryPtr & root, std::string_view path, FSEntryVector & stack)
{
	if (!root)
		return -EINVAL;

	stack.clear();
	stack.reserve(8);
	stack.push_back(root);

	int retval = 0;

	while (!path.empty())
	{
		auto nextSep = path.find('/');
		std::string_view segment = path.substr(0, nextSep);

		std::shared_ptr<FSEntry> nextEntry;
		retval = stack.back()->getChild(segment, nextEntry);
		if (retval)
			break;

		stack.push_back(nextEntry);
		path = (nextSep == path.npos ? std::string_view() : path.substr(nextSep+1));
	}

	return retval;
}

} // namespace

GitContext::GitContext(MountContext & mountcontext, const fuse_conn_info *info, const fuse_config *config) : repository(mountcontext.repository)
{
	mountcontext.repository = nullptr;

	_fuse_conn_info = new fuse_conn_info(*info);
	_fuse_config = new fuse_config(*config);

	branch.swap(mountcontext.branch);
	commit.swap(mountcontext.commit);
	debug = mountcontext.debug;

	// TODO check capabilities CAP_SETUID, CAP_SETGID
	uid = config->set_uid ? config->uid : geteuid();
	gid = config->set_gid ? config->gid : getegid();
	umask = mountcontext.readwrite ? 0 : 0222;
	if (config->set_mode)
		umask |= (config->umask & ACCESSPERMS);
	else
		umask |= 022;

	atime = 0;
	time(&atime);

	root = std::make_shared<FSRoot>();
	root->rebuildRefs(repository);
	fileInfoKey = 0;

	if (debug)
		std::cout << "Listing files with uid=" << uid << " gid=" << gid << " umask=" << std::oct << std::setw(4) << std::setfill('0') << umask << std::endl;

	// Convenience
	umask = ~umask;
}

GitContext::~GitContext()
{
	delete _fuse_conn_info;
	delete _fuse_config;
}

GitContext * GitContext::get()
{
	fuse_context *fuseContext = fuse_get_context();

	if (!fuseContext || !fuseContext->private_data)
		throw -ENOENT;

	return reinterpret_cast<GitContext *>(fuseContext->private_data);
}

const fuse_operations * GitContext::fuseOperations()
{
	return &_operations;
}

void * GitContext::_fuse_init(fuse_conn_info *conn, fuse_config *cfg)
{
	fuse_context *fuseContext = fuse_get_context();
	MountContext *mountContext = reinterpret_cast<MountContext *>(fuseContext->private_data);

	// Don't need path information for opened files, we'll have a SHA1
	cfg->nullpath_ok = 1;

	// Disable caching if we're mounting the tip of a specific branch so underlying commits can happen
	cfg->kernel_cache = mountContext->branch.empty() ? 0 : 1;

	// Honour inodes on 64 bit systems, the first 8 bytes (16 hex) of SHA1 hashes are unique enough
	if (sizeof(stat::st_ino) == 8)
		cfg->use_ino = 1;

	// Always attempt to prefill the inode cache from a directory listing
	cfg->readdir_ino = 1;

	return new GitContext(*mountContext, conn, cfg);
}

void GitContext::_fuse_destroy(void *private_data)
{
	if (private_data)
	{
		GitContext *gitContext = reinterpret_cast<GitContext*>(private_data);
		delete gitContext;
	}
}

int GitContext::_fuse_getattr(const char *path, struct stat *st, struct fuse_file_info *fi)
{
	if ((!path && !fi) || !st)
		return -EINVAL;

	return inContext(&GitContext::fuse_getattr, path ? std::string_view(path) : std::string_view(), st, fi);
}

int GitContext::fuse_getattr(std::string_view path, struct stat *st, struct fuse_file_info *fi)
{
	int retval = -ENOENT;

	Logger log(retval, debug);
	log << "getattr:";
	if (fi)
		log << " handle=" << fi->fh;
	else
		log << " path=" << path;
	log << Logger::retval;

	if (fi)
	{
	}
	else if (!path.empty() && path.front() == '/')
	{
		FSEntryVector entries;
		retval = resolvePath(root, path.substr(1), entries);
		if (retval == 0)
		{
			st->st_uid = uid;
			st->st_gid = gid;
			st->st_nlink = 1;
			st->st_atime = atime;
			st->st_ctime = atime;
			st->st_mtime = atime;
			entries.back()->fillStat(st);
			st->st_mode &= umask;
		}
	}

	return retval;
}

int GitContext::_fuse_readlink(const char *path, char *buf, size_t bufsize)
{
	if (!path || !buf || !bufsize)
		return -EINVAL;

	return inContext(&GitContext::fuse_readlink, std::string_view(path), buf, bufsize);
}

int GitContext::fuse_readlink(std::string_view path, char *buf, size_t bufsize)
{
	int retval = -ENOENT;

	Logger log(retval, debug);
	log << "readlink: path=" << path << " bufsize=" << bufsize << Logger::retval;

	if (!path.empty() && path.front() == '/')
	{
		FSEntryVector entries;
		retval = resolvePath(root, path.substr(1), entries);
		if (retval == 0)
			retval = entries.back()->readLink(buf, bufsize);
	}

	return retval;
}

int GitContext::_fuse_opendir(const char *path, struct fuse_file_info *fi)
{
	if (!path || !fi)
		return -EINVAL;

	return inContext(&GitContext::fuse_opendir, std::string_view(path), fi);
}

int GitContext::_fuse_readdir(const char *path, void *fusebuf, fuse_fill_dir_t fillfunc, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags)
{
	if (!fusebuf || !fillfunc || !fi)
		return -EINVAL;

	return inContext(&GitContext::fuse_readdir, fusebuf, fillfunc, offset, fi, flags);
}

int GitContext::_fuse_releasedir(const char *path, struct fuse_file_info *fi)
{
	if (!fi)
		return -EINVAL;

	return inContext(&GitContext::fuse_releasedir, fi);
}

int GitContext::fuse_opendir(std::string_view path, struct fuse_file_info *fi)
{
	int retval = -ENOENT;

	Logger log(retval, debug);
	log << "opendir: path=" << path << Logger::retval;

	if (!path.empty() && path.front() == '/')
	{
		FSEntryVector entries;
		retval = resolvePath(root, path.substr(1), entries);
		if (retval == 0)
		{
			struct stat st;
			entries.back()->fillStat(&st);
			if (!S_ISDIR(st.st_mode))
				retval = -ENOTDIR;
		}
		if (retval == 0)
		{
			std::shared_ptr<FileInfo> info = std::make_shared<FileInfo>();
			info->stack.swap(entries);

			std::lock_guard<std::mutex> guard(fileInfoLock);
			++fileInfoKey;
			fileInfo.insert(std::make_pair(fileInfoKey, info));
			fi->fh = fileInfoKey;
		}
	}

	log << " handle=" << fi->fh;
	return retval;
}

int GitContext::fuse_readdir(void *fusebuf, fuse_fill_dir_t fillfunc, off_t offset, struct fuse_file_info *fi, fuse_readdir_flags flags)
{
	int retval = -EINVAL;

	Logger log(retval, debug);
	log << "readdir: handle=" << fi->fh << " offset=" << offset << Logger::retval;

	std::shared_ptr<FileInfo> info;

	std::unique_lock<std::mutex> guard(fileInfoLock);
	auto iter = fileInfo.find(fi->fh);
	if (iter != fileInfo.end())
		info = iter->second;
	guard.unlock();

	if (info)
	{
		retval = 0;

		struct stat st = {};
		st.st_uid = uid;
		st.st_gid = gid;
		st.st_nlink = 1;
		st.st_atime = atime;
		st.st_ctime = atime;
		st.st_mtime = atime;

		off_t index = offset;
		if (index == 0)
		{
			info->stack.back()->fillStat(&st);
			st.st_mode &= umask;
			fillfunc(fusebuf, ".", &st, 1, fuse_fill_dir_flags(FUSE_FILL_DIR_PLUS));
			index = 1;
		}

		if (index == 1)
		{
			if (info->stack.size() >= 2)
			{
				info->stack[info->stack.size() - 2]->fillStat(&st);
				st.st_mode &= umask;
				fillfunc(fusebuf, "..", &st, 2, fuse_fill_dir_flags(FUSE_FILL_DIR_PLUS));
			}
			else
			{
				fillfunc(fusebuf, "..", nullptr, 2, fuse_fill_dir_flags(0));
			}
			index = 2;
		}

		retval = info->stack.back()->enumerateChildren([&](const char *name, off_t idx, struct stat *st) -> int
		{
			st->st_mode &= umask;
			return fillfunc(fusebuf, name, st, idx + 2, fuse_fill_dir_flags(FUSE_FILL_DIR_PLUS));
		}, index - 2, &st);
	}

	return retval;
}

int GitContext::fuse_releasedir(struct fuse_file_info *fi)
{
	int retval = -EINVAL;

	Logger log(retval, debug);
	log << "releasedir: handle=" << fi->fh << Logger::retval;

	std::shared_ptr<FileInfo> info;

	std::lock_guard<std::mutex> guard(fileInfoLock);
	auto iter = fileInfo.find(fi->fh);
	if (iter != fileInfo.end())
	{
		fileInfo.erase(iter);
		retval = 0;
	}

	return retval;
}
