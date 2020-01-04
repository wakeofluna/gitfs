#include "fs_pseudo_entry.h"

FSPseudoEntry::InodeType FSPseudoEntry::mLastInode;

FSPseudoEntry::FSPseudoEntry()
{
	mInode = ++mLastInode | FSPseudoBit;
	mUnlinked = false;
}

FSPseudoEntry::~FSPseudoEntry()
{
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
