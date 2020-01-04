#ifndef GIT_WRAPPERS_H_
#define GIT_WRAPPERS_H_

#include <functional>
#include <memory>
#include <git2.h>

class GitRepository;
class GitReference;
class GitReferenceIterator;
class GitBlob;
class GitCommit;
class GitTag;
class GitTree;
class GitTreeEntry;
class GitTreeEntryView;
class GitObject;

inline bool operator== (const git_oid & lhs, const git_oid & rhs)
{
	return git_oid_equal(&lhs, &rhs);
}

#define WRAPCOMMON(clz,typ) \
	inline bool operator! () const { return data == nullptr; } \
	inline bool operator== (const clz & other) const { return data == other.data; } \
	inline bool operator!= (const clz & other) const { return data != other.data; } \
	inline operator bool() const { return data != nullptr; } \
	inline operator const typ*() const { return data; }

#define WRAP(clz,typ) \
public: \
	inline clz(const typ * value = nullptr) : data(const_cast<typ*>(value)) {} \
	clz(const clz & other) : data(other.data) {}; \
	inline ~clz() {} \
	inline const typ* get() const { return data; } \
	clz& operator= (const clz & other) { data = other.data; return *this; } \
	WRAPCOMMON(clz,typ) \
protected: \
	typ *data

#define WRAPVIEW(clz,typ,freefunc) \
class clz : public clz ## View \
{ \
public: \
	inline explicit clz(typ * value = nullptr) : clz ## View(value) {} \
	clz(const clz &) = delete; \
	inline clz(clz && other) : clz ## View(other.data) { other.data = nullptr; } \
	inline ~clz() { if (data) freefunc(data); } \
	inline typ* get() { return data; } \
	inline void free() { if (data) { freefunc(data); data = nullptr; } } \
	inline typ** fill() { free(); return &data; } \
	clz& operator= (const clz &) = delete; \
	inline clz& operator= (clz && other) { if (data) freefunc(data); data = other.data; other.data = nullptr; return *this; } \
	inline operator typ*() { return data; } \
	WRAPCOMMON(clz,typ) \
}

class GitRepositoryView
{
WRAP(GitRepositoryView, git_repository);
public:
	GitReference head() const;
	GitReference resolveReference(const char *shorthand) const;
	GitBlob resolveBlob(const git_oid * oid) const;
	GitCommit resolveCommit(const git_oid * oid) const;
	GitCommit resolveCommit(const git_oid * oid, size_t oidSize) const;
	GitObject resolveObject(const git_oid * oid, git_object_t type = GIT_OBJECT_ANY) const;
	GitObject resolveObject(const git_oid * shortOid, size_t oidSize, git_object_t type = GIT_OBJECT_ANY) const;
	GitTag resolveTag(const git_oid * oid) const;
	GitTree resolveTree(const git_oid * oid) const;
	int forEachReference(const std::function<int(GitReference &)> & func) const;
	int forEachReference(const std::function<int(const char *)> & func) const;
	int targetByName(git_oid * oid, const char *name) const;
};
WRAPVIEW(GitRepository, git_repository, git_repository_free);

class GitReferenceView
{
WRAP(GitReferenceView, git_reference);
public:
	GitReference dup() const;
	enum Type
	{
		INVALID = GIT_REFERENCE_INVALID,
		DIRECT = GIT_REFERENCE_DIRECT,
		SYMBOLIC = GIT_REFERENCE_SYMBOLIC,
		ALL = GIT_REFERENCE_ALL
	};
	Type type() const;
	inline bool isInvalid() const { return type() == INVALID; }
	inline bool isDirect() const { return type() == DIRECT; }
	inline bool isSymbolic() const { return type() == SYMBOLIC; }
	GitReference resolve() const;
	const char *name() const;
	const git_oid *target() const;
};
WRAPVIEW(GitReference, git_reference, git_reference_free);

class GitReferenceIteratorView
{
WRAP(GitReferenceIteratorView, git_reference_iterator);
};
WRAPVIEW(GitReferenceIterator, git_reference_iterator, git_reference_iterator_free);

class GitBlobView
{
WRAP(GitBlobView, git_blob);
public:
	GitRepositoryView owner() const;
	const git_oid *id() const;
	off_t size() const;
	const void *content() const;
};
WRAPVIEW(GitBlob, git_blob, git_blob_free);

class GitCommitView
{
WRAP(GitCommitView, git_commit);
public:
	GitRepositoryView owner() const;
	const git_oid *id() const;
	const git_oid *parentId(unsigned int parentNr = 0) const;
	unsigned int parentCount() const;
	GitObject object() const;
	git_time_t time() const;
	GitTree tree() const;
};
WRAPVIEW(GitCommit, git_commit, git_commit_free);

class GitTagView
{
WRAP(GitTagView, git_tag);
};
WRAPVIEW(GitTag, git_tag, git_tag_free);

class GitTreeView
{
WRAP(GitTreeView, git_tree);
public:
	GitRepositoryView owner() const;
	const git_oid *id() const;
	size_t entryCount() const;
	GitTreeEntryView byIndex(size_t index) const;
	GitTreeEntry byPath(const char *path) const;
};
WRAPVIEW(GitTree, git_tree, git_tree_free);

class GitTreeEntryView
{
WRAP(GitTreeEntryView, git_tree_entry);
public:
	const git_oid *id() const;
	const char *name() const;
	git_object_t type() const;
	git_filemode_t mode() const;
};
WRAPVIEW(GitTreeEntry, git_tree_entry, git_tree_entry_free);

class GitObjectView
{
WRAP(GitObjectView, git_object);
public:
	const git_oid *id() const;
	std::string shortId() const;
	git_object_t type() const;
	GitObject peel(git_object_t type) const;
};
WRAPVIEW(GitObject, git_object, git_object_free);

#undef WRAP
#undef WRAPCOMMON
#undef WRAPVIEW

#endif // GIT_WRAPPERS_H_
