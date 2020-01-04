#include "fs_entry.h"

FSEntry::InodeType FSEntry::inodeFromOid(const git_oid * oid)
{
	return (oid ? *((const FSEntry::InodeType*)oid) & FSRealMask : 0);
}

bool FSEntry::isUnlinked() const
{
	return false;
}

int FSEntry::setUnlinked(bool unlinked)
{
	return -EPERM;
}

size_t FSEntry::purgeUnlinked()
{
	return 0;
}

int FSEntry::getChild(std::string_view & name, std::shared_ptr<FSEntry> & target, bool allowUnlinked) const
{
	target.reset();
	return -ENOTDIR;
}

int FSEntry::addChild(const std::shared_ptr<FSEntry> & entry, bool allowReplace)
{
	return -ENOTDIR;
}

int FSEntry::removeChild(const std::string_view & name)
{
	return -ENOTDIR;
}

int FSEntry::enumerateChildren(const EnumerateFunction & callback, off_t start, struct stat *st) const
{
	return -ENOTDIR;
}

int FSEntry::readLink(char * buffer, size_t bufsize) const
{
	return -EINVAL;
}

int FSEntry::read(char * buffer, size_t bufsize, off_t offset) const
{
	return -EINVAL;
}
