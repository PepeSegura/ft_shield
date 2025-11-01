#include "ft_shield.h"

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
		strlcpy(final, &s1[i], size - i + 1);
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

#define BUFFER_SIZE 4096

void copy_file(char *src, char *dst)
{
	int	fd_src = open(src, O_RDONLY);
	if (fd_src == -1) {
		ft_perror(src);
		return ;
	}
	int	fd_dst = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0755);
	if (fd_dst == -1) {
		ft_perror(dst);
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
