#include "fs_entry.h"

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

int FSEntry::getChild(const std::string_view & name, std::shared_ptr<FSEntry> & target) const
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
