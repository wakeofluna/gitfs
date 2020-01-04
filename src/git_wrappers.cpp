#include "git_wrappers.h"
#include <git2.h>

GitReference GitRepository::head() const
{
	GitReference ref;
	if (data)
		git_repository_head(ref.fill(), data);
	return ref;
}

GitReference GitRepository::resolveReference(const char *shorthand) const
{
	GitReference ref;
	if (data && shorthand)
		git_reference_dwim(ref.fill(), data, shorthand);
	return ref;
}

GitBlob GitRepository::resolveBlob(const git_oid * oid) const
{
	GitBlob blob;
	if (data && oid)
		git_blob_lookup(blob.fill(), data, oid);
	return blob;
}

GitCommit GitRepository::resolveCommit(const git_oid * oid) const
{
	GitCommit commit;
	if (data && oid)
		git_commit_lookup(commit.fill(), data, oid);
	return commit;
}

GitObject GitRepository::resolveObject(const git_oid * oid, git_object_t type) const
{
	GitObject object;
	if (data && oid)
		git_object_lookup(object.fill(), data, oid, type);
	return object;
}

GitTag GitRepository::resolveTag(const git_oid * oid) const
{
	GitTag tag;
	if (data && oid)
		git_tag_lookup(tag.fill(), data, oid);
	return tag;
}

GitTree GitRepository::resolveTree(const git_oid * oid) const
{
	GitTree tree;
	if (data && oid)
		git_tree_lookup(tree.fill(), data, oid);
	return tree;
}

int GitRepository::forEachReference(const std::function<int(GitReference &)> & func) const
{
	if (!data)
		return GIT_ENOTFOUND;

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
	if (!data)
		return GIT_ENOTFOUND;

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
	return ((data && oid && name) ? git_reference_name_to_id(oid, data, name) : GIT_ENOTFOUND);
}

GitReference GitReference::dup() const
{
	GitReference ref;

	if (data)
	{
		int giterr = git_reference_dup(ref.fill(), data);
		if (giterr)
			throw giterr;
	}

	return ref;
}

GitReference::Type GitReference::type() const
{
	return (data ? static_cast<GitReference::Type>(git_reference_type(data)) : GitReference::INVALID);
}

GitReference GitReference::resolve() const
{
	GitReference ref;

	if (data)
	{
		int giterr = git_reference_resolve(ref.fill(), data);
		if (giterr)
			throw giterr;
	}

	return ref;
}

const char *GitReference::name() const
{
	return (data ? git_reference_name(data) : nullptr);
}

const git_oid *GitReference::target() const
{
	return (data ? git_reference_target(data) : nullptr);
}

git_repository *GitCommit::owner() const
{
	return (data ? git_commit_owner(data) : nullptr);
}

const git_oid *GitCommit::id() const
{
	return (data ? git_commit_id(data) : nullptr);
}

const git_oid *GitCommit::parentId(unsigned int parentNr) const
{
	return (data ? git_commit_parent_id(data, parentNr) : nullptr);
}

unsigned int GitCommit::parentCount() const
{
	return (data ? git_commit_parentcount(data) : 0);
}

GitObject GitCommit::object() const
{
	GitObject object;
	if (data)
		git_object_lookup(object.fill(), git_commit_owner(data), git_commit_id(data), GIT_OBJECT_COMMIT);
	return object;
}

git_time_t GitCommit::time() const
{
	return (data ? git_commit_time(data) : git_time_t());
}

const git_oid *GitObject::id() const
{
	return (data ? git_object_id(data) : nullptr);
}

std::string GitObject::shortId() const
{
	git_buf buf = {};
	if (data)
		git_object_short_id(&buf, data);
	return (buf.size > 0 ? std::string(buf.ptr, buf.size) : std::string());
}

git_object_t GitObject::type() const
{
	return (data ? git_object_type(data) : GIT_OBJECT_INVALID);
}

GitObject GitObject::peel(git_object_t type) const
{
	GitObject object;
	if (data)
		git_object_peel(object.fill(), data, type);
	return object;
}
