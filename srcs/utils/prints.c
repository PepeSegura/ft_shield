#include "ft_shield.h"

#ifdef ENABLE
# include <stdarg.h>
# include <time.h>
#endif

void ft_dprintf(int fd, const char *format, ...)
{
	#ifdef ENABLE
		time_t now = time(NULL);
		struct tm *t = localtime(&now);
		dprintf(fd, "[%02d:%02d:%02d] ", t->tm_hour, t->tm_min, t->tm_sec);
		
		va_list args;
		va_start(args, format);
		vdprintf(fd, format, args);
		va_end(args);
	#else
		(void)fd, (void)format;
	#endif
}

void ft_perror(char *str)
{
	#ifdef ENABLE
		time_t now = time(NULL);
		struct tm *t = localtime(&now);
		dprintf(2, "[%02d:%02d:%02d] ", t->tm_hour, t->tm_min, t->tm_sec);
		perror(str);
	#else
		(void)str;
	#endif
}