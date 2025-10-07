#ifndef FT_SHIELD_H
# define FT_SHIELD_H

# include <fcntl.h>
# include <libgen.h>
# include <limits.h>
# include <stdbool.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <arpa/inet.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/select.h>
# include <sys/un.h>

#define DEST_PATH "/workspaces"
#define LISTENING_PORT 4242
#define MAX_CONECTIONS 10
#define MAX_NBR_CLIENTS 2
#define PASSCODE "4242\n"

#define TRIM_CHARS "\f\n\r\t\v "

#define HELP "\
?     - Shows help\n\
shell - Spawn shell in port 4243\n\
"
#define REMOTE_OPENED "New remote shell opened in port 4243\n"


int	find_own_path(char *buffer, size_t size_buffer);

/* utils.c */
char	*ft_strtrim(char const *s1, char const *set);

#endif
