#include "stdio.h"
#include "util.h"
#define O_RDWR 2

int argc;
char argv[8][256];


char workdir[256];

void parse_args(char *rbuf)
{
	argc = 0;
	char *iptr = rbuf;
	char *optr = argv[argc];
	while (*iptr)
	{
		if (*iptr != ' ')
		{
			*optr++ == *iptr++;
			continue;
		}
		*optr = 0;
		++iptr;
		optr = argv[++argc];
	}
}

int strcmp(const char *s1, const char *s2)
{
	while (*s1 && *s2)
	{
		if (*s1 == *s2)
		{
			++s1;
			++s2;
		}
		return *s1 - *s2;
	}
}

//int strlen(const char *s)
//{
//	char *r = s;
//	while (*r++);
//	return r - s;
//}

void builtin_chdir()
{
	if (argc == 1)
	{
		write(tty, "cd: cd [dir]\n", 14);
		return;
	}

}

void builtin_mkdir()
{
	if (argc == 1)
	{
		write(tty, "mkdir: md [dir]\n", 14);
		return;
	}
}

void builtin_pwd()
{
	
	write(tty, workdir, strlen(workdir));
}

void builtin_rmdir()
{
	if (argc == 1)
	{
		write(tty, "rmdir: rd [dir]\n", 14);
		return;
	}
}

void main()
{
	int tty = open("dev_tty0", O_RDWR);
	char rbuf[256];
	while (1)
	{
		int len = read(tty, rbuf, 255);
		rbuf[len] = 0;
		parse_args(rbuf);
		if (strcmp(argv[0], "cd"))
		{

		}
		if (strcmp(argv[0], "mkdir"))
		{

		}
		if (strcmp(argv[0], "pwd"))
		{

		}
		if (strcmp(argv[0], "rmdir"))
		{

		}
	}
}