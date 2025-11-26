#include "ft_shield.h"

int	shell_function(t_server *server, int index)
{
	const int	client_fd = server->clients[index].fd;
	pid_t		pid;
	int			inpipe_fds[2], outpipe_fds[2];

	ft_dprintf(2, "clientfd: %d\n", client_fd);

	memset(&server->clients[index], 0, sizeof(t_client));

	pipe(inpipe_fds);
	pipe(outpipe_fds); //TODO maybe error check??
	server->clients[index].inpipe_fd = inpipe_fds[1]; //write end for server, we read from 0 here in stdin
	server->clients[index].outpipe_fd = outpipe_fds[0]; //read end for server, we write in 1 here
	pid = fork();
	if (pid < 0) {
		perror("fork");
	}
	else if (pid == 0) {
		setsid();

		close(server->fd);
		dup2(inpipe_fds[0], STDIN_FILENO);
		dup2(outpipe_fds[1], STDERR_FILENO);
		dup2(outpipe_fds[1], STDOUT_FILENO);
		close(outpipe_fds[0]);
		close(inpipe_fds[1]);
		close(client_fd);

		char *args[] = {"/bin/bash", "-i", NULL};
		setenv("TERM", "xterm-256color", 1);
		execv("/bin/bash", args);
		exit(1);
	}
	else if (pid > 0) {
		ft_lstadd_back(&server->pids, ft_lstnew((void *)(long)pid));
	}
	return (1);
}
