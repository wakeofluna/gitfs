#ifndef FS_BRANCH_H_
#define FS_BRANCH_H_

#include "fs_pseudo_directory.h"

class GitRepository;
class FSBranch : public FSPseudoDirectory
{
public:
	FSBranch(std::string name);
	~FSBranch();

	static const int Type;
	int type() const override;

	std::string_view name() const override;
	int enumerateChildren(const EnumerateFunction & callback, off_t start, struct stat *st) const override;

	int setBranch(GitRepository & repo, const char *branch);

private:
	std::string mName;
	std::string mBranch;
	GitCommit mHead;
};

#endif // FS_BRANCH_H_
