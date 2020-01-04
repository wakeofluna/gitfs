#ifndef FS_ROOT_H_
#define FS_ROOT_H_

#include "fs_pseudo_directory.h"

class GitRepository;
class FSRoot : public FSPseudoDirectory
{
public:
	FSRoot(GitRepository & repo);

	static const int Type;
	int type() const override;

	std::string_view name() const override;
	int getChild(std::string_view & name, std::shared_ptr<FSEntry> & target, bool allowUnlinked) const override;

	void rebuildRefs();

private:
	GitRepository & repository;
};

#endif // FS_ROOT_H_
