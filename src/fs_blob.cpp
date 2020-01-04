#include "fs_blob.h"
#include <cstring>

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

	if (mMode == GIT_FILEMODE_BLOB_EXECUTABLE)
		st->st_mode = 0777 | S_IFREG;
	else if (mMode == GIT_FILEMODE_BLOB)
		st->st_mode = 0666 | S_IFREG;
	else
		st->st_mode = mMode;

	return 0;
}

int FSBlob::read(char * buffer, size_t bufsize, off_t offset) const
{
	const void * content = mBlob.content();
	size_t contentSize = mBlob.size();

	if (!content || offset < 0)
		return -EIO;

	if (size_t(offset) >= contentSize)
		return 0;

	size_t contentLen = contentSize - offset;
	if (contentLen > bufsize)
		contentLen = bufsize;

	std::memcpy(buffer, (const char *)content + offset, contentLen);
	return contentLen;
}
