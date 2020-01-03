#include "fs_pseudo_entry.h"

namespace
{

constexpr FSPseudoEntry::InodeType mPseudoMask = 1ULL << (sizeof(FSPseudoEntry::InodeType) * 8 - 1);

}

FSPseudoEntry::InodeType FSPseudoEntry::mLastInode;

FSPseudoEntry::FSPseudoEntry()
{
	mInode = ++mLastInode | mPseudoMask;
	mUnlinked = false;
}

FSPseudoEntry::~FSPseudoEntry()
{
}

FSPseudoEntry::InodeType FSPseudoEntry::inode() const
{
	return mInode;
}

bool FSPseudoEntry::isUnlinked() const
{
	return mUnlinked;
}

int FSPseudoEntry::setUnlinked(bool unlinked)
{
	mUnlinked = unlinked;
	return 0;
}
