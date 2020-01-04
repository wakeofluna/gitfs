#include "fs_branch.h"
#include "fs_commit_link.h"
#include "git_wrappers.h"

const int FSBranch::Type = 0x710ffe;

FSBranch::FSBranch(std::string name, unsigned int depth)
{
	mName.swap(name);
	mDepth = depth;
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

int FSBranch::setBranch(GitRepository & repo, const char *branch)
{
	git_oid oid;
	int retval;

	mBranch.clear();
	mHead.free();

	retval = repo.targetByName(&oid, branch);
	if (retval == 0)
	{
		GitObject object = repo.resolveObject(&oid);
		GitObject peeled = object.peel(GIT_OBJECT_COMMIT);
		GitCommit commit = repo.resolveCommit(peeled.id());
		if (commit)
		{
			mBranch = branch;
			mHead = std::move(commit);
			updateHeads();
		}
		else
		{
			retval = GIT_ENOTFOUND;
		}
	}

	return retval;
}

void FSBranch::updateHeads()
{
	FSEntryPtr entry;
	FSCommitLink *link;

	const int nrParents = mHead.parentCount();
	for (int i = -1; i < nrParents; ++i)
	{
		std::string newName;
		newName.reserve(8);
		newName = "HEAD";
		if (i >= 0)
			newName += '^';
		if (i > 0)
			newName += (i + '1');

		std::string_view name(newName);
		getChild(name, entry);
		if (!entry)
		{
			entry = std::make_shared<FSCommitLink>(newName, mDepth);
			addChild(entry);
		}

		link = entry->cast<FSCommitLink>();
		if (link)
		{
			link->updateFromCommit(mHead, i);
		}
	}
}
