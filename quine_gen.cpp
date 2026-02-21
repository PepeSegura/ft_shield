#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <fcntl.h>

void print_code(const char *str, int fd)
{
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
}

int main(void)
{
	std::ifstream file("ft_shield.c");

    std::string text((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());

	int fd = open("ft_shield_quined.c", O_WRONLY | O_TRUNC | O_CREAT, 0777);

	dprintf(fd, "%s\n", text.c_str());
	dprintf(fd, "char *source_code = \"");

	text.append("\nchar *source_code = ");
    print_code(text.c_str(), fd);
	dprintf(fd, "\";\n");
}
