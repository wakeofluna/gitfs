#ifndef FS_PSEUDO_DIRECTORY_H_
#define FS_PSEUDO_DIRECTORY_H_

#include "fs_pseudo_entry.h"
#include <mutex>

class FSPseudoDirectory : public FSPseudoEntry
{
public:
	FSPseudoDirectory();
	~FSPseudoDirectory();

	int fillStat(struct stat *st) const override;

	int getChild(std::string_view & name, std::shared_ptr<FSEntry> & target, bool allowUnlinked) const override;
	int addChild(const std::shared_ptr<FSEntry> & entry, bool allowReplace = false) override;
	int removeChild(const std::string_view & name) override;
	int enumerateChildren(const EnumerateFunction & callback, off_t start, struct stat *st) const override;

	int setUnlinked(bool unlinked) override;
	size_t purgeUnlinked() override;

protected:
	static std::recursive_mutex gLock;
	std::map<std::string_view, std::shared_ptr<FSEntry>> mEntries;
};

#endif // FS_PSEUDO_DIRECTORY_H_
