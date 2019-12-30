#include <memory>
#include <utility>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstring>

#include <git2.h>
#include <fuse.h>
#include <sys/stat.h>
#include <unistd.h>

#include "git_context.h"
#include "mount_context.h"
#include "logger.h"

namespace
{

constexpr fuse_operations _operations = {
	.init = &GitContext::_fuse_init,
	.destroy = &GitContext::_fuse_destroy,
	.getattr = &GitContext::_fuse_getattr,
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

} // namespace

GitContext::GitContext(MountContext & mountcontext, const fuse_conn_info *info, const fuse_config *config) : repository(mountcontext.repository)
{
	mountcontext.repository = nullptr;

	_fuse_conn_info = new fuse_conn_info(*info);
	_fuse_config = new fuse_config(*config);

	branch.swap(mountcontext.branch);
	commit.swap(mountcontext.commit);
	debug = mountcontext.debug;
	pathPseudoKey = 0;

	// TODO check capabilities CAP_SETUID, CAP_SETGID
	uid = config->set_uid ? config->uid : geteuid();
	gid = config->set_gid ? config->gid : getegid();
	umask = mountcontext.readwrite ? 0 : 0222;
	if (config->set_mode)
		umask |= config->umask;
	else
		umask |= 022;

	atime = 0;
	time(&atime);

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

std::pair<GitCommit, std::string_view> GitContext::decypherPath(const std::string_view & path)
{
	GitCommit commit;
	std::string_view subpath = path;

	return std::make_pair(std::move(commit), subpath);
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
	//if (sizeof(stat::st_ino) == 8)
	//	cfg->use_ino = 1;
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
	else
	{
		GitCommit commit;
		std::string_view subpath;
		std::tie(commit, subpath) = decypherPath(path);

		if (commit)
		{

		}
		else
		{
			st->st_uid = uid;
			st->st_gid = gid;
			st->st_nlink = 1;
			st->st_atime = atime;
			st->st_ctime = atime;
			st->st_mtime = atime;

			int result;

			if (subpath == "/")
			{
				st->st_mode = (0777 & umask) | 0100 | S_IFDIR;
				result = 1;
			}
			else
			result = repository.forEachReference([&] (const char * refname) -> int
			{
				std::string_view ref(refname);
				if (ref.compare(0, 5, "refs/") != 0)
					return 0;

				if (ref.compare(ref.size() - 5, 5, "/HEAD") == 0)
					ref = ref.substr(0, ref.size() - 5);

				ref = ref.substr(4);
				if (ref.size() < subpath.size() && subpath.compare(0, ref.size(), ref) == 0)
				{
					std::string_view rempath = subpath.substr(ref.size());
					if (rempath.front() != '/')
						return 0;

					if (rempath == "/HEAD")
					{
						st->st_mode = (0444 & umask) | 0400 | S_IFLNK;
						return 1;
					}

					return 0;
				}

				if (ref.compare(0, subpath.size(), subpath) == 0)
				{
					ref = ref.substr(subpath.size());
					if (ref == "/HEAD")
					{
						st->st_mode = (0444 & umask) | 0400 | S_IFLNK;
						return 1;
					}
					else if (ref.empty() || ref.front() == '/')
					{
						st->st_mode = (0777 & umask) | 0100 | S_IFDIR;
						return 1;
					}

					return 0;
				}

				return 0;
			});

			if (result > 0)
				retval = 0;
		}
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

	GitCommit commit;
	std::string_view subpath;
	std::tie(commit, subpath) = decypherPath(path);

	if (commit)
	{

	}
	else if (!subpath.empty())
	{
		// Validate against the known refs
		repository.forEachReference([&] (const char * refname) -> int {
			std::string_view ref(refname);
			if (ref.compare(0, 5, "refs/") == 0 && ref.compare(4, subpath.size(), subpath) == 0)
			{
				retval = 0;
				return 1;
			}
			return 0;
		});

		std::unique_lock guard(pathMutex);
		auto found = pathMap.find(subpath);
		if (retval == 0)
		{
			if (found == pathMap.end())
			{
				++pathPseudoKey;
				auto [entry,inserted] = pathInfoMap.insert(std::make_pair(pathPseudoKey, subpath));
				std::tie(found, std::ignore) = pathMap.insert(std::make_pair(std::string_view(entry->second), entry->first));
			}
			fi->fh = found->second;
		}
		else if (found != pathMap.end())
		{
			// Remove entry from cache
			auto entry = pathInfoMap.find(found->second);
			if (entry != pathInfoMap.end())
				pathInfoMap.erase(entry);
			pathMap.erase(found);
		}
		guard.unlock();
	}

	log << " handle=" << fi->fh;
	return retval;
}

int GitContext::fuse_readdir(void *fusebuf, fuse_fill_dir_t fillfunc, off_t offset, struct fuse_file_info *fi, fuse_readdir_flags flags)
{
	int retval = -ENOENT;

	Logger log(retval, debug);
	log << "readdir: handle=" << fi->fh << " offset=" << offset << Logger::retval;

	std::string_view path;

	std::unique_lock guard(pathMutex);
	auto found = pathInfoMap.find(fi->fh);
	if (found != pathInfoMap.end())
		path = found->second;
	guard.unlock();

	log << Logger::removeRetval << " path=" << path << Logger::retval;
	if (!path.empty())
	{
		fillfunc(fusebuf, ".", nullptr, 0, fuse_fill_dir_flags(0));
		fillfunc(fusebuf, "..", nullptr, 0, fuse_fill_dir_flags(0));

		struct stat st = {};
		st.st_uid = uid;
		st.st_gid = gid;
		st.st_atime = atime;
		st.st_mtime = atime;
		st.st_ctime = atime;

		if (path.front() == '/')
			path = path.substr(1);

		std::string lastEntry;
		retval = repository.forEachReference([&] (const char * refname) -> int
		{
			std::string_view ref(refname);
			if (ref.compare(0, 5, "refs/") == 0 && ref.compare(5, path.size(), path) == 0)
			{
				ref = ref.substr(5 + path.size());
				if (ref.empty())
				{
					st.st_mode = (0777 & umask) | 0400 | S_IFLNK;
					st.st_size = GIT_OID_HEXSZ;
					fillfunc(fusebuf, "HEAD", &st, 0, fuse_fill_dir_flags(0));
				}
				else if (ref.front() == '/' || path.empty())
				{
					if (path.empty())
						ref = ref.substr(0, ref.find('/'));
					else
						ref = ref.substr(1, ref.find('/', 1) - 1);
					if (ref != lastEntry)
					{
						lastEntry = ref;
						if (lastEntry == "HEAD")
						{
							st.st_mode = (0777 & umask) | 0400 | S_IFLNK;
							st.st_size = GIT_OID_HEXSZ;
						}
						else
						{
							st.st_mode = (0777 & umask) | 0100 | S_IFDIR;
							st.st_size = 0;
						}
						fillfunc(fusebuf, lastEntry.c_str(), &st, 0, fuse_fill_dir_flags(0));
					}
				}
			}
			return 0;
		});
	}

	return retval;
}

int GitContext::fuse_releasedir(struct fuse_file_info *fi)
{
	int retval = -ENOENT;

	Logger log(retval, debug);
	log << "releasedir: handle=" << fi->fh << Logger::retval;

	if (fi->fh == 1)
		retval = 0;

	return retval;
}
