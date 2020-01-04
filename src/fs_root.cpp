#include "fs_root.h"
#include "fs_branch.h"
#include "fs_commit.h"
#include <iostream>

const int FSRoot::Type = 0x9d23a;

FSRoot::FSRoot(GitRepository & repo) : repository(repo)
{

}

int FSRoot::type() const
{
	return Type;
}

std::string_view FSRoot::name() const
{
	static const char *empty = "";
	return std::string_view(empty, 0);
}

int FSRoot::getChild(std::string_view & name, std::shared_ptr<FSEntry> & target) const
{
	std::lock_guard<std::recursive_mutex> guard(gLock);

	auto nextSep = name.find('/');
	std::string_view segment = name.substr(0, nextSep);

	git_oid oid;
	if (git_oid_fromstrn(&oid, segment.data(), segment.length()) == 0)
	{
		GitCommit commit = repository.resolveCommit(&oid, segment.length());
		if (commit)
		{
			name = (nextSep == name.npos ? std::string_view() : name.substr(nextSep+1));
			target = std::make_shared<FSCommit>(std::move(commit));
			return 0;
		}
	}

	return FSPseudoDirectory::getChild(name, target);
}

void FSRoot::rebuildRefs()
{
	std::lock_guard<std::recursive_mutex> guard(gLock);

	setUnlinked(true);
	setUnlinked(false);

	repository.forEachReference([this] (const char *refname) -> int
	{
		std::string_view ref(refname);
		if (ref.substr(0, 5) != std::string_view("refs/", 5))
			return 0;

		ref = ref.substr(5);
		if (ref.empty())
			return 0;

		FSEntry * current = this;
		unsigned int depth = 0;
		while (true)
		{
			++depth;

			auto nextSep = ref.find('/');
			std::string_view segment = ref.substr(0, nextSep);
			ref = (nextSep == ref.npos ? std::string_view() : ref.substr(nextSep+1));

			FSEntryPtr nextEntry;
			int retval = current->getChild(segment, nextEntry);
			if (retval == -ENOENT)
			{
				nextEntry = std::make_shared<FSBranch>(std::string(segment), depth);
				current->addChild(nextEntry);
				retval = 0;
			}

			if (retval != 0)
			{
				std::cerr << "Name collision when building reference directory structure" << std::endl;
				return 0;
			}

			current = nextEntry.get();
			current->setUnlinked(false);
			if (ref.empty())
			{
				FSBranch *branch = current->cast<FSBranch>();
				if (!branch)
				{
					std::cerr << "Type error when building reference directory structure" << std::endl;
					return 0;
				}

				branch->setBranch(repository, refname);
				break;
			}
		}

		return 0;
	});

	purgeUnlinked();
}
