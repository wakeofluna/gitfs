#include "fs_pseudo_directory.h"

std::recursive_mutex FSPseudoDirectory::gLock;

FSPseudoDirectory::FSPseudoDirectory()
{
}

FSPseudoDirectory::~FSPseudoDirectory()
{
}

int FSPseudoDirectory::fillStat(struct stat *st) const
{
	st->st_ino = mInode;
	st->st_mode = 0777 | S_IFDIR;
	st->st_size = 0;
	return 0;
}

int FSPseudoDirectory::getChild(std::string_view & name, std::shared_ptr<FSEntry> & target, bool allowUnlinked) const
{
	std::lock_guard<std::recursive_mutex> guard(gLock);

	auto nextSep = name.find('/');
	std::string_view segment = name.substr(0, nextSep);

	auto iter = mEntries.find(segment);
	if (iter == mEntries.cend() || (!allowUnlinked && iter->second->isUnlinked()))
		return -ENOENT;

	name = (nextSep == name.npos ? std::string_view() : name.substr(nextSep+1));
	target = iter->second;
	return 0;
}

int FSPseudoDirectory::addChild(const std::shared_ptr<FSEntry> & entry, bool allowReplace)
{
	std::lock_guard<std::recursive_mutex> guard(gLock);

	std::string_view name = entry->name();
	auto iter = mEntries.find(name);
	if (iter == mEntries.end())
	{
		mEntries.insert(std::make_pair(name, entry));
		return 0;
	}
	else if (allowReplace)
	{
		iter->second = entry;
		return 0;
	}
	else
	{
		return -EEXIST;
	}
}

int FSPseudoDirectory::removeChild(const std::string_view & name)
{
	std::lock_guard<std::recursive_mutex> guard(gLock);

	auto iter = mEntries.find(name);
	if (iter == mEntries.end())
	{
		return -ENOENT;
	}
	else
	{
		mEntries.erase(iter);
		return 0;
	}
}

int FSPseudoDirectory::enumerateChildren(const EnumerateFunction & callback, off_t start, struct stat *st) const
{
	std::lock_guard<std::recursive_mutex> guard(gLock);

	auto iter = mEntries.cbegin();
	auto end = mEntries.cend();
	int retval = 0;
	off_t index = 0;

	while (iter != end && retval == 0)
	{
		if (!iter->second->isUnlinked())
		{
			++index;
			if (index > start)
			{
				iter->second->fillStat(st);
				retval = callback(iter->first.data(), index, st);
			}
		}
		++iter;
	}

	return 0;
}

int FSPseudoDirectory::setUnlinked(bool unlinked)
{
	mUnlinked = unlinked;
	if (unlinked)
	{
		std::lock_guard<std::recursive_mutex> guard(gLock);
		for (auto & iter : mEntries)
			iter.second->setUnlinked(true);
	}

	return 0;
}

size_t FSPseudoDirectory::purgeUnlinked()
{
	size_t count = 0;

	std::lock_guard<std::recursive_mutex> guard(gLock);

	auto next = mEntries.begin();
	auto end = mEntries.end();
	for (decltype(next) iter = next; iter != end; iter = next)
	{
		std::advance(next, 1);
		count += iter->second->purgeUnlinked();
		if (iter->second->isUnlinked())
		{
			mEntries.erase(iter);
			++count;
		}
	}

	return count;
}
