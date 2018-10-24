#include <stdio.h>
#include <time.h>
#include "gitfs.h"
#include "utils.h"

void print_version(void)
{
	printf("gitfs version: %d.%d\n", GITFS_VERSION_MAJOR, GITFS_VERSION_MINOR);
}

time_t get_current_time(void)
{
	static int time_method = 0;
	struct timespec tspec;

	if (time_method == 0)
	{
		if (clock_gettime(CLOCK_MONOTONIC, &tspec) == 0)
			return tspec.tv_sec;

		time_method = 1;
	}

	if (time_method == 1)
	{
		if (clock_gettime(CLOCK_REALTIME, &tspec) == 0)
			return tspec.tv_sec;

		time_method = 2;
	}

	return time(NULL);
}
