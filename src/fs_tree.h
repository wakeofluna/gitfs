#ifndef FS_TREE_H_
#define FS_TREE_H_

#include "fs_entry.h"
#include "git_wrappers.h"

class FSTree : public FSEntry
{
public:
	FSTree(GitTree && tree);
	~FSTree();

	static const int Type;
	int type() const override;

	std::string_view name() const override;
	int fillStat(struct stat *st) const override;

	int getChild(std::string_view & name, std::shared_ptr<FSEntry> & target, bool allowUnlinked) const override;
	int addChild(const std::shared_ptr<FSEntry> & entry, bool allowReplace = false) override;
	int removeChild(const std::string_view & name) override;
	int enumerateChildren(const EnumerateFunction & callback, off_t start, struct stat *st) const override;

protected:
	InodeType mInode;
	GitTree mTree;
};

#endif // FS_TREE_H_
