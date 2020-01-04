#include "git_wrappers.h"
#include <git2.h>

GitReference GitRepositoryView::head() const
{
	GitReference ref;
	if (data)
		git_repository_head(ref.fill(), data);
	return ref;
}

GitReference GitRepositoryView::resolveReference(const char *shorthand) const
{
	GitReference ref;
	if (data && shorthand)
		git_reference_dwim(ref.fill(), data, shorthand);
	return ref;
}

GitBlob GitRepositoryView::resolveBlob(const git_oid * oid) const
{
	GitBlob blob;
	if (data && oid)
		git_blob_lookup(blob.fill(), data, oid);
	return blob;
}

GitCommit GitRepositoryView::resolveCommit(const git_oid * oid) const
{
	GitCommit commit;
	if (data && oid)
		git_commit_lookup(commit.fill(), data, oid);
	return commit;
}

GitCommit GitRepositoryView::resolveCommit(const git_oid * oid, size_t oidSize) const
{
	GitCommit commit;
	if (data && oid && oidSize)
		git_commit_lookup_prefix(commit.fill(), data, oid, oidSize);
	return commit;
}

GitObject GitRepositoryView::resolveObject(const git_oid * oid, git_object_t type) const
{
	GitObject object;
	if (data && oid)
		git_object_lookup(object.fill(), data, oid, type);
	return object;
}

GitObject GitRepositoryView::resolveObject(const git_oid * shortOid, size_t oidSize, git_object_t type) const
{
	GitObject object;
	if (data && shortOid && oidSize)
		git_object_lookup_prefix(object.fill(), data, shortOid, oidSize, type);
	return object;
}

GitTag GitRepositoryView::resolveTag(const git_oid * oid) const
{
	GitTag tag;
	if (data && oid)
		git_tag_lookup(tag.fill(), data, oid);
	return tag;
}

GitTree GitRepositoryView::resolveTree(const git_oid * oid) const
{
	GitTree tree;
	if (data && oid)
		git_tree_lookup(tree.fill(), data, oid);
	return tree;
}

int GitRepositoryView::forEachReference(const std::function<int(GitReference &)> & func) const
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

int GitRepositoryView::forEachReference(const std::function<int(const char *)> & func) const
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

int GitRepositoryView::targetByName(git_oid * oid, const char *name) const
{
	return ((data && oid && name) ? git_reference_name_to_id(oid, data, name) : GIT_ENOTFOUND);
}

GitReference GitReferenceView::dup() const
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

GitReference::Type GitReferenceView::type() const
{
	return (data ? static_cast<GitReference::Type>(git_reference_type(data)) : GitReference::INVALID);
}

GitReference GitReferenceView::resolve() const
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

const char *GitReferenceView::name() const
{
	return (data ? git_reference_name(data) : nullptr);
}

const git_oid *GitReferenceView::target() const
{
	return (data ? git_reference_target(data) : nullptr);
}

GitRepositoryView GitBlobView::owner() const
{
	return (data ? git_blob_owner(data) : 0);
}

const git_oid *GitBlobView::id() const
{
	return (data ? git_blob_id(data) : 0);
}

off_t GitBlobView::size() const
{
	return (data ? git_blob_rawsize(data) : 0);
}

const void *GitBlobView::content() const
{
	return (data ? git_blob_rawcontent(data) : nullptr);
}

GitRepositoryView GitCommitView::owner() const
{
	return (data ? git_commit_owner(data) : nullptr);
}

const git_oid *GitCommitView::id() const
{
	return (data ? git_commit_id(data) : nullptr);
}

const git_oid *GitCommitView::parentId(unsigned int parentNr) const
{
	return (data ? git_commit_parent_id(data, parentNr) : nullptr);
}

unsigned int GitCommitView::parentCount() const
{
	return (data ? git_commit_parentcount(data) : 0);
}

GitObject GitCommitView::object() const
{
	GitObject object;
	if (data)
		git_object_lookup(object.fill(), git_commit_owner(data), git_commit_id(data), GIT_OBJECT_COMMIT);
	return object;
}

git_time_t GitCommitView::time() const
{
	return (data ? git_commit_time(data) : git_time_t());
}

GitTree GitCommitView::tree() const
{
	GitTree tree;
	if (data)
		git_commit_tree(tree.fill(), data);
	return tree;
}

GitRepositoryView GitTreeView::owner() const
{
	return (data ? git_tree_owner(data) : nullptr);
}

const git_oid *GitTreeView::id() const
{
	return (data ? git_tree_id(data) : nullptr);
}

size_t GitTreeView::entryCount() const
{
	return (data ? git_tree_entrycount(data) : 0);
}

GitTreeEntryView GitTreeView::byIndex(size_t index) const
{
	return (data ? GitTreeEntryView(git_tree_entry_byindex(data, index)) : GitTreeEntryView());
}

GitTreeEntry GitTreeView::byPath(const char *path) const
{
	GitTreeEntry entry;
	if (data)
		git_tree_entry_bypath(entry.fill(), data, path);
	return entry;
}

const git_oid *GitTreeEntryView::id() const
{
	return (data ? git_tree_entry_id(data) : nullptr);
}

const char *GitTreeEntryView::name() const
{
	return (data ? git_tree_entry_name(data) : nullptr);
}

git_object_t GitTreeEntryView::type() const
{
	return (data ? git_tree_entry_type(data) : GIT_OBJECT_INVALID);
}

git_filemode_t GitTreeEntryView::mode() const
{
	return (data ? git_tree_entry_filemode(data) : GIT_FILEMODE_UNREADABLE);
}

const git_oid *GitObjectView::id() const
{
	return (data ? git_object_id(data) : nullptr);
}

std::string GitObjectView::shortId() const
{
	git_buf buf = {};
	if (data)
		git_object_short_id(&buf, data);
	return (buf.size > 0 ? std::string(buf.ptr, buf.size) : std::string());
}

git_object_t GitObjectView::type() const
{
	return (data ? git_object_type(data) : GIT_OBJECT_INVALID);
}

GitObject GitObjectView::peel(git_object_t type) const
{
	GitObject object;
	if (data)
		git_object_peel(object.fill(), data, type);
	return object;
}
