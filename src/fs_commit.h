#ifndef FS_COMMIT_H_
#define FS_COMMIT_H_

#include "fs_tree.h"

class FSCommit : public FSTree
{
public:
	FSCommit(GitCommit && commit);
	~FSCommit();

	static const int Type;
	int type() const override;

private:
	GitCommit mCommit;
};

#endif // FS_COMMIT_H_
