#ifndef FS_PSEUDO_ENTRY_H_
#define FS_PSEUDO_ENTRY_H_

#include "fs_entry.h"

class FSPseudoEntry : public FSEntry
{
public:
	FSPseudoEntry();
	FSPseudoEntry(const FSPseudoEntry & other) = delete;
	virtual ~FSPseudoEntry();

	InodeType inode() const override;

	bool isUnlinked() const override;
	int setUnlinked(bool unlinked) override;

protected:
	InodeType mInode;
	bool mUnlinked;

private:
	static InodeType mLastInode;
};

#endif // FS_PSEUDO_ENTRY_H_
