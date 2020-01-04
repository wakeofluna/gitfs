#include "fs_blob.h"

const int FSBlob::Type = 0x472bca9;

FSBlob::FSBlob(GitBlob && blob, git_filemode_t mode) : mBlob(std::move(blob)), mMode(mode)
{
	mInode = inodeFromOid(mBlob.id());
}

FSBlob::~FSBlob()
{
}

int FSBlob::type() const
{
	return Type;
}

std::string_view FSBlob::name() const
{
	return std::string_view();
}

int FSBlob::fillStat(struct stat *st) const
{
	st->st_nlink = 2;
	st->st_size = mBlob.size();
	st->st_ino = mInode;
	st->st_mode = mMode;
	return 0;
}
