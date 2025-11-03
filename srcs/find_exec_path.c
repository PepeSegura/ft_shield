#include "ft_shield.h"

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
		int readed = readlink(paths[i], buffer, size_buffer);
		if (readed != -1)
		{
			ft_dprintf(2, "readlink: [%s]\n", buffer);
			return (i);
		}
	}
	return (-1);
}
