#ifndef FS_BLOB_H_
#define FS_BLOB_H_

#include "fs_entry.h"

class FSBlob : public FSEntry
{
public:
	FSBlob(GitBlob && blob, git_filemode_t mode);
	~FSBlob();

	static const int Type;
	int type() const override;

	std::string_view name() const override;
	int fillStat(struct stat *st) const override;
	int read(char * buffer, size_t bufsize, off_t offset) const override;

private:
	GitBlob mBlob;
	git_filemode_t mMode;
	InodeType mInode;
};

#endif // FS_BLOB_H_
