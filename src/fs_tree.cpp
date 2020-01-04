#include "fs_tree.h"
#include "fs_blob.h"

const int FSTree::Type = 0xe561ffae;

FSTree::FSTree(GitTree && tree) : mTree(std::move(tree))
{
	const git_oid * oid = mTree.id();
	mInode = inodeFromOid(oid);
}

FSTree::~FSTree()
{
}

int FSTree::type() const
{
	return Type;
}

std::string_view FSTree::name() const
{
	return std::string_view();
}

int FSTree::fillStat(struct stat *st) const
{
	st->st_ino = mInode;
	st->st_mode = 0777 | S_IFDIR;
	st->st_size = 0;
	st->st_nlink = 1;
	return 0;
}

int FSTree::getChild(std::string_view & name, std::shared_ptr<FSEntry> & target, bool allowUnlinked) const
{
	int retval = -ENOENT;

	GitTreeEntry entry = mTree.byPath(name.data());
	if (entry)
	{
		git_filemode_t mode = entry.mode();
		switch (mode)
		{
			case GIT_FILEMODE_UNREADABLE:
			case GIT_FILEMODE_COMMIT:
				retval = -EIO;
				break;
			case GIT_FILEMODE_TREE:
			{
				GitTree tree = mTree.owner().resolveTree(entry.id());
				name = std::string_view();
				target = std::make_shared<FSTree>(std::move(tree));
				retval = 0;
				break;
			}
			case GIT_FILEMODE_BLOB:
			case GIT_FILEMODE_BLOB_EXECUTABLE:
			{
				GitBlob blob = mTree.owner().resolveBlob(entry.id());
				name = std::string_view();
				target = std::make_shared<FSBlob>(std::move(blob), mode);
				retval = 0;
				break;
			}
			case GIT_FILEMODE_LINK:
				retval = -EIO;
				break;
		}
	}

	return retval;
}

int FSTree::addChild(const std::shared_ptr<FSEntry> & entry, bool allowReplace)
{
	return -EPERM;
}

int FSTree::removeChild(const std::string_view & name)
{
	return -EPERM;
}

int FSTree::enumerateChildren(const EnumerateFunction & callback, off_t start, struct stat *st) const
{
	off_t index = 0;

	size_t lastId = mTree.entryCount();
	for (size_t id = 0; id < lastId; ++id)
	{
		GitTreeEntryView entry = mTree.byIndex(id);
		git_filemode_t mode = entry.mode();

		switch (mode)
		{
			case GIT_FILEMODE_UNREADABLE:
			case GIT_FILEMODE_COMMIT:
				st->st_mode = 0;
				break;
			case GIT_FILEMODE_TREE:
				st->st_mode = 0777 | S_IFDIR;
				break;
			case GIT_FILEMODE_BLOB:
				st->st_mode = 0666 | S_IFREG;
				break;
			case GIT_FILEMODE_BLOB_EXECUTABLE:
				st->st_mode = 0777 | S_IFREG;
				break;
			case GIT_FILEMODE_LINK:
				st->st_mode = 0666 | S_IFLNK;
				break;
		}

		if (st->st_mode != 0)
		{
			++index;
			if (index > start)
			{
				st->st_ino = inodeFromOid(entry.id());
				switch (mode)
				{
					case GIT_FILEMODE_BLOB:
					case GIT_FILEMODE_BLOB_EXECUTABLE:
					case GIT_FILEMODE_LINK:
					{
						GitBlob blob = mTree.owner().resolveBlob(entry.id());
						st->st_nlink = 2;
						st->st_size = blob.size();
						break;
					}
					default:
						st->st_nlink = 1;
						st->st_size = 0;
						break;
				}
				callback(entry.name(), index, st);
			}
		}
	}

	return 0;
}
