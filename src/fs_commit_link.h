#ifndef FS_COMMIT_LINK_H_
#define FS_COMMIT_LINK_H_

#include "fs_pseudo_entry.h"
#include "git_wrappers.h"
#include <string>

class FSCommitLink : public FSPseudoEntry
{
public:
	FSCommitLink(std::string name, unsigned int depth);
	~FSCommitLink();

	static const int Type;
	int type() const override;

	std::string_view name() const override;
	int fillStat(struct stat *st) const override;
	int readLink(char * buffer, size_t bufsize) const override;

	void updateFromCommit(const GitCommit & commit, int parent = -1);

private:
	std::string mName;
	std::string mLink;
	unsigned int mDepth;
};

#endif // FS_COMMIT_LINK_H_
