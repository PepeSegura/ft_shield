#include "ft_shield.h"

void	create_service(void)
{
	ft_dprintf(2, "creating service\n");
	int	fd_config = open(DEST_SERVICE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd_config == -1) {
		ft_perror(DEST_SERVICE);
		exit(1);
	}

	const char *service = "";
}

void	start_service(void)
{
	ft_dprintf(2, "starting service\n");
}
