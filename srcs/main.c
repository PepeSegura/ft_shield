#include "ft_shield.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>

void	ft_shield()
{
	uid_t	uid = geteuid();

	printf("UID: %d\n", uid);
}

int	find_own_path(char *buffer, size_t size_buffer)
{
	const char *paths[] = {
		"/proc/self/exe",        // Linux
		"/proc/curproc/file",    // FreeBSD
        "/proc/self/path/a.out", // Solaris
        "/proc/curproc/exe",     // NetBSD
        NULL
    };

	for (int i = 0; paths[i]; ++i)
	{
		size_t readed = readlink(paths[i], buffer, size_buffer);
		if (readed != -1)
			return (i);
	}
	return (-1);
}

int main(int argc, char **argv)
{
	(void)argc,(void)argv;

	char path[255] = {0};

	printf("RET: %d\n", find_own_path(path, sizeof(path)));
	if (strcmp(DEST_PATH, dirname(path)) == 0)
	{
		printf("We are in the infected path: %s\n", path);
	}
	else
	{
		printf("We are safe: %s\n", path);
	}

	return (0);
}
