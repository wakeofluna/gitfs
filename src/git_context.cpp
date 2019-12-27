#include <utility>
#include "git_context.h"
#include "mount_context.h"

GitContext::GitContext(MountContext & mountcontext)
{
	std::swap(repository, mountcontext.repository);
	branch.swap(mountcontext.branch);
	commit.swap(mountcontext.commit);
	debug = mountcontext.debug;
	writable = mountcontext.readwrite;
}

GitContext::~GitContext()
{
	// TODO Auto-generated destructor stub
}

