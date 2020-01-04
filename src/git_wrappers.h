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
class GitObject;

#define WRAP(clz,typ,freefunc) \
public: \
	inline clz(typ * value = nullptr) : data(value) {} \
	clz(const clz &) = delete; \
	inline clz(clz && other) : data(other.data) { other.data = nullptr; } \
	inline ~clz() { if (data) freefunc(data); } \
	inline void free() { if (data) { freefunc(data); data = nullptr; } } \
	inline typ** fill() { free(); return &data; } \
	clz& operator= (const clz &) = delete; \
	inline clz& operator= (clz && other) { if (data) freefunc(data); data = other.data; other.data = nullptr; return *this; } \
	inline bool operator! () const { return data == nullptr; } \
	inline operator bool() const { return data != nullptr; } \
	inline operator typ*() { return data; } \
	inline operator const typ*() const { return data; } \
protected: \
	typ *data

class GitRepository
{
WRAP(GitRepository, git_repository, git_repository_free);
public:
	GitReference head() const;
	GitReference resolveReference(const char *shorthand) const;
	GitBlob resolveBlob(const git_oid * oid) const;
	GitCommit resolveCommit(const git_oid * oid) const;
	GitObject resolveObject(const git_oid * oid, git_object_t type = GIT_OBJECT_ANY) const;
	GitTag resolveTag(const git_oid * oid) const;
	GitTree resolveTree(const git_oid * oid) const;
	int forEachReference(const std::function<int(GitReference &)> & func) const;
	int forEachReference(const std::function<int(const char *)> & func) const;
	int targetByName(git_oid * oid, const char *name) const;
};

class GitReference
{
WRAP(GitReference, git_reference, git_reference_free);
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

class GitReferenceIterator
{
WRAP(GitReferenceIterator, git_reference_iterator, git_reference_iterator_free);
};

class GitBlob
{
WRAP(GitBlob, git_blob, git_blob_free);
};

class GitCommit
{
WRAP(GitCommit, git_commit, git_commit_free);
public:
	git_repository *owner() const;
	const git_oid *id() const;
	const git_oid *parentId(unsigned int parentNr = 0) const;
	unsigned int parentCount() const;
	GitObject object() const;
	git_time_t time() const;
};

class GitTag
{
WRAP(GitTag, git_tag, git_tag_free);
};

class GitTree
{
WRAP(GitTree, git_tree, git_tree_free);
};

class GitObject
{
WRAP(GitObject, git_object, git_object_free);
public:
	const git_oid *id() const;
	std::string shortId() const;
	git_object_t type() const;
	GitObject peel(git_object_t type) const;
};


#undef WRAP
#endif // GIT_WRAPPERS_H_
