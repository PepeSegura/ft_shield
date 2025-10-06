#include "ft_shield.h"



void	ft_shield()
{
	uid_t	uid = geteuid();

	printf("UID: %d\n", uid);
}


void	create_server()
{
	int socketFd = socket(AF_INET, SOCK_STREAM, 0);
	if (socketFd == -1)
	{
		perror("socket");
		exit(EXIT_FAILURE);
	}


	int reuse = 1;
	if (setsockopt(socketFd, SOL_SOCKET, SO_REUSEADDR, (const char *)&reuse, sizeof(reuse)) < 0)
	{
		perror("setsockopt(SO_REUSEADDR)");
		exit(EXIT_FAILURE);
	}


	struct sockaddr_in server_addr;

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(8080);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(socketFd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
	{
		perror("bind");
		exit(EXIT_FAILURE);
	}

	if (listen(socketFd, 16) == -1)
	{
		perror("Listening failed");
		exit(EXIT_FAILURE);
	}
}


int main(int argc, char **argv)
{
	(void)argc,(void)argv;

	char path[255] = {0};

	printf("RET: %d\n", find_own_path(path, sizeof(path)));
	if (strcmp(DEST_PATH, dirname(path)) == 0)
	{
		printf("We are in the infected path: %s\n", path);
	}
	else
	{
		printf("We are safe: %s\n", path);
		// create_server();
	}

	return (0);
}
