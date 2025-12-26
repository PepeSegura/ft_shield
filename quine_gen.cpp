#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>

void print_code(const char *str)
{
	printf("\"");
	for (int i = 0; str[i]; i++)
	{
		if (str[i] == '"')
			printf("\\\"");
		else if (str[i] == '\n')
			printf("\\n");
		else if (str[i] == '\\')
			printf("\\\\");
		else if (str[i] == '\t')
			printf("\\t");
		else
			printf("%c", str[i]);
	}
	printf("\";\n");
}

int main(void)
{
	std::ifstream file("ft_shield.c");

    std::string text((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
    
    print_code(text.c_str());
}
