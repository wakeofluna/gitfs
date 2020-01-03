#ifndef FS_ROOT_H_
#define FS_ROOT_H_

#include "fs_pseudo_directory.h"

class GitRepository;
class FSRoot : public FSPseudoDirectory
{
public:
	static const int Type;
	int type() const override;

	std::string_view name() const override;

	void rebuildRefs(GitRepository & repo);
};

#endif // FS_ROOT_H_
