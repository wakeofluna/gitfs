#ifndef MOUNT_CONTEXT_H_
#define MOUNT_CONTEXT_H_

#include <string>
struct git_repository;

struct MountContext
{
	~MountContext();

	git_repository * repository = nullptr;
	std::string repopath;
	std::string branch;
	std::string commit;
	bool foreground = false;
	bool debug = false;
	bool readwrite = false;
};

#endif // MOUNT_CONTEXT_H_
