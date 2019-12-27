#include "mount_context.h"
#include <git2.h>

MountContext::~MountContext()
{
	if (repository)
		git_repository_free(repository);
}
