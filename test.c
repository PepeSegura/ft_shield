#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

void copy_file(char *src, char *dst)
{
	int	fd_src = open(src, O_RDONLY);
	if (fd_src == -1) {
		perror(src);
		return ;
	}
	int	fd_dst = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0755);
	if (fd_dst == -1) {
		perror(dst);
		close(fd_src);
		return ;
	}

	char buffer[BUFFER_SIZE] = {0};

	while (1)
	{
		int ret_read = read(fd_src, buffer, BUFFER_SIZE);
		if (ret_read <= 0)
			break ;

		char	*write_buff = buffer;
		int		bytes_remaining = ret_read;

		while (bytes_remaining > 0)
		{
			int ret_write = write(fd_dst, write_buff, bytes_remaining);
			if (ret_write <= 0) break ;
			bytes_remaining -= ret_write;
			write_buff += ret_write;
		}
	}
	close(fd_src);
	close(fd_dst);
}

int main(int argc, char **argv)
{
    copy_file("./a.out", argv[1]);
}