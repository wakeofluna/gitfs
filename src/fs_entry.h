#ifndef FS_ENTRY_H_
#define FS_ENTRY_H_

#include <functional>
#include <map>
#include <memory>
#include <string_view>
#include <sys/types.h>
#include <sys/stat.h>
#include "git_wrappers.h"

class FSEntry
{
public:
	using EnumerateFunction = std::function<int(const char *, off_t, struct stat *)>;
	using InodeType = decltype(stat::st_ino);

	static InodeType inodeFromOid(const git_oid * oid);

public:
	inline FSEntry() {}
	virtual ~FSEntry() {}

	template <typename T>
	inline T *cast()
	{
		return type() == T::Type ? static_cast<T*>(this) : nullptr;
	}

	template <typename T>
	inline const T* cast() const
	{
		return type() == T::Type ? static_cast<const T*>(this) : nullptr;
	}

	/* Filesystem requirements */
	virtual int type() const = 0;
	virtual std::string_view name() const = 0;
	virtual int fillStat(struct stat *st) const = 0;

	/* Optional support for unlinking entries */
	virtual bool isUnlinked() const;
	virtual int setUnlinked(bool unlinked);
	virtual size_t purgeUnlinked();

	/* Optional support for child entries */
	virtual int getChild(std::string_view & name, std::shared_ptr<FSEntry> & target) const;
	virtual int addChild(const std::shared_ptr<FSEntry> & entry, bool allowReplace = false);
	virtual int removeChild(const std::string_view & name);
	virtual int enumerateChildren(const EnumerateFunction & callback, off_t start, struct stat *st) const;

	/* Optional support for symbolic links */
	virtual int readLink(char * buffer, size_t bufsize) const;
};

using FSEntryPtr = std::shared_ptr<FSEntry>;

constexpr FSEntry::InodeType FSPseudoBit = 1ULL << (sizeof(FSEntry::InodeType) * 8 - 1);
constexpr FSEntry::InodeType FSRealMask = ~FSPseudoBit;

#endif // FS_ENTRY_H_
