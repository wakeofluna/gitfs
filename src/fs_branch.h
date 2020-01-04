#ifndef FS_BRANCH_H_
#define FS_BRANCH_H_

#include "fs_pseudo_directory.h"

class GitRepository;
class FSBranch : public FSPseudoDirectory
{
public:
	FSBranch(std::string name, unsigned int depth);
	~FSBranch();

	static const int Type;
	int type() const override;

	std::string_view name() const override;

	int setBranch(GitRepository & repo, const char *branch);

private:
	void updateHeads();

private:
	std::string mName;
	std::string mBranch;
	GitCommit mHead;
	unsigned int mDepth;
};

#endif // FS_BRANCH_H_
