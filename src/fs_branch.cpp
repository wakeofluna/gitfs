#include "fs_branch.h"
#include "git_wrappers.h"

const int FSBranch::Type = 0x710ffe;

FSBranch::FSBranch(std::string name)
{
	mName.swap(name);
}

FSBranch::~FSBranch()
{

}

int FSBranch::type() const
{
	return Type;
}

std::string_view FSBranch::name() const
{
	return std::string_view(mName);
}

int FSBranch::enumerateChildren(const EnumerateFunction & callback, off_t start, struct stat *st) const
{
	return FSPseudoDirectory::enumerateChildren(callback, start, st);
}

int FSBranch::setBranch(GitRepository & repo, const char *branch)
{
	git_oid oid;
	int retval = repo.targetByName(&oid, branch);
	if (retval == 0)
	{
		GitCommit commit = repo.resolveCommit(&oid);
		if (commit)
		{
			mBranch = branch;
			mHead = std::move(commit);
		}
		else
		{
			retval = GIT_EINVALIDSPEC;
		}
	}
	return retval;
}
