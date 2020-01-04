#include "fs_commit_link.h"
#include <cstring>

const int FSCommitLink::Type = 0x1b64fe;

FSCommitLink::FSCommitLink(std::string name, unsigned int depth)
{
	mName.swap(name);
	mDepth = depth;
}

FSCommitLink::~FSCommitLink()
{
}

int FSCommitLink::type() const
{
	return Type;
}

std::string_view FSCommitLink::name() const
{
	return std::string_view(mName);
}

int FSCommitLink::fillStat(struct stat *st) const
{
	st->st_ino = mInode;
	st->st_mode = 0444 | S_IFLNK;
	st->st_size = mLink.size();
	st->st_blocks = 1;
	st->st_blksize = 512;
	return 0;
}

int FSCommitLink::readLink(char * buffer, size_t bufsize) const
{
	if (!buffer || !bufsize)
		return -EINVAL;

	if (mLink.empty())
		return -EIO;

	size_t needed = mLink.size() + 1;
	std::memcpy(buffer, mLink.data(), std::min(needed, bufsize));
	buffer[bufsize-1] = 0;
	return 0;
}

void FSCommitLink::updateFromCommit(const GitCommit & commit, int parent)
{
	mLink.clear();
	mLink.reserve(64);

	const git_oid * oid = (parent == -1 ? commit.id() : commit.parentId(parent));
	if (oid)
	{
		// XXX do this through the classes and not by calling git functions directly
		GitObject object;
		git_object_lookup(object.fill(), commit.owner(), oid, GIT_OBJECT_COMMIT);

		for (unsigned int i = 0; i < mDepth; ++i)
			mLink += "../";
		mLink += object.shortId();

		setUnlinked(false);
	}
	else
	{
		setUnlinked(true);
	}
}
