#include "git_wrappers.h"
#include <git2.h>

GitReference GitRepository::head() const
{
	GitReference ref;
	git_repository_head(ref.fill(), data);
	return ref;
}

GitReference GitRepository::resolve(const char *shorthand) const
{
	GitReference ref;
	git_reference_dwim(ref.fill(), data, shorthand);
	return ref;
}

GitBlob GitRepository::resolveBlob(const git_oid * oid) const
{
	GitBlob blob;
	git_blob_lookup(blob.fill(), data, oid);
	return blob;
}

GitCommit GitRepository::resolveCommit(const git_oid * oid) const
{
	GitCommit commit;
	git_commit_lookup(commit.fill(), data, oid);
	return commit;
}

GitTree GitRepository::resolveTree(const git_oid * oid) const
{
	GitTree tree;
	git_tree_lookup(tree.fill(), data, oid);
	return tree;
}

int GitRepository::forEachReference(const std::function<int(GitReference &)> & func) const
{
	int giterr;

	GitReferenceIterator iter;
	giterr = git_reference_iterator_new(iter.fill(), data);

	while (!giterr)
	{
		GitReference ref;
		giterr = git_reference_next(ref.fill(), iter);
		if (!giterr)
			giterr = func(ref);
	}

	return giterr == GIT_ITEROVER ? 0 : giterr;
}

int GitRepository::forEachReference(const std::function<int(const char *)> & func) const
{
	int giterr;

	GitReferenceIterator iter;
	giterr = git_reference_iterator_new(iter.fill(), data);

	while (!giterr)
	{
		const char *name = nullptr;
		giterr = git_reference_next_name(&name, iter);
		if (!giterr)
			giterr = func(name);
	}

	return giterr == GIT_ITEROVER ? 0 : giterr;
}

int GitRepository::targetByName(git_oid * oid, const char *name) const
{
	return git_reference_name_to_id(oid, data, name);
}

GitReference GitReference::dup() const
{
	GitReference ref;

	int giterr = git_reference_dup(ref.fill(), data);
	if (giterr)
		throw giterr;

	return ref;
}

GitReference::Type GitReference::type() const
{
	return static_cast<GitReference::Type>(git_reference_type(data));
}

GitReference GitReference::resolve() const
{
	GitReference ref;

	int giterr = git_reference_resolve(ref.fill(), data);
	if (giterr)
		throw giterr;

	return ref;
}

const char *GitReference::name() const
{
	return git_reference_name(data);
}

const git_oid *GitReference::target() const
{
	return git_reference_target(data);
}

git_time_t GitCommit::time() const
{
	return git_commit_time(data);
}
