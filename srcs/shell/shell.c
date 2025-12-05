#include "ft_shield.h"

#include <fcntl.h>
#include <sys/ioctl.h>

int	shell_function(t_server *server, int index)
{
	const int	client_fd = server->clients[index].fd;
	pid_t		pid;
	int			inpipe_fds[2], outpipe_fds[2];

	ft_dprintf(2, "clientfd: %d\n", client_fd);

	pipe(inpipe_fds);
	pipe(outpipe_fds); //TODO maybe error check??
	server->clients[index].inpipe_fd = inpipe_fds[WRITE_END]; //write end for server, we read from 0 here in stdin
	server->clients[index].outpipe_fd = outpipe_fds[READ_END]; //read end for server, we write in 1 here
	
	pid = fork();
	if (pid < 0) {
		perror("fork");

	}
	else if (pid == 0) {
		setsid();
		
		close(server->fd);
		close(inpipe_fds[WRITE_END]);
		close(outpipe_fds[READ_END]);
		dup2(inpipe_fds[READ_END], STDIN_FILENO);
		dup2(outpipe_fds[WRITE_END], STDERR_FILENO);
		dup2(outpipe_fds[WRITE_END], STDOUT_FILENO);
		close(inpipe_fds[READ_END]);
		close(outpipe_fds[WRITE_END]);
		close(client_fd);

		char *args[] = {"/bin/bash", "-i", NULL};
		setenv("TERM", "xterm-256color", 1);
		execv("/bin/bash", args);
		exit(1);
	}
	else if (pid > 0) {
		ft_lstadd_back(&server->pids, ft_lstnew((void *)(long)pid));
		close(inpipe_fds[READ_END]);
		close(outpipe_fds[WRITE_END]);
	}
	return (1);
}
