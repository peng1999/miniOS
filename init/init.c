#include "util.h"
// extern int open(char*,int);
// extern int read(int,char*,int);
// extern int write(int,char*,int);
// extern int createdir(char *);
// extern int opendir(char *);
// extern int fat_opendir(char * dirname);
// extern int fat_createdir(char *dirname);
// extern int fat_deletedir(char *dirname);
// extern int strcmp(const char *s1, const char *s2);
// extern int strlen(const char *s);
// extern char* strcpy(char *dest, const char *src);
// extern char* strrchr(char *s, int c);

#define O_RDWR 2
#define PATH_DEL '\\'

int argc;
char argv[8][256];

int tty;

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
	*optr = 0;
	++argc;
}

char* strchrs(char *s1, char *s2)
{
	while (*s1)
	{
		char *r = s2;
		while (*r)
		{
			if (*s1 == *r)
			{
				return s1;
			}
			++r;
		}
		++s1;
	}
	return 0;
}

void builtin_chdir()
{
	if (argc == 1)
	{
		write(tty, "cd: cd [dir]\n", 14);
		return;
	}
	if (strcmp(argv[1], "."))
	{
		return;
	}
	char newdir[256];
	char wbuf[256];
	int len = strlen(workdir);
	strcpy(newdir, workdir);
	if (strcmp(argv[1], ".."))
	{
		char *trunc = strrchr(newdir + 1, PATH_DEL);
		if (trunc)
		{
			*trunc == 0;
		}
		else
		{
			newdir[1] = 0;
		}
		fat_opendir("V:");
		fat_opendir(newdir);
		strcpy(workdir, newdir);
		return;
	}
	newdir[len++] = PATH_DEL;
	strcpy(newdir + len, argv[1]);
	fat_opendir("V:");
	if (fat_opendir(newdir) != 1)
	{
		write(tty, "ERROR: no such directory\n", 26);
		return;
	}
	strcpy(workdir, newdir);
}

void builtin_mkdir()
{
	if (argc == 1)
	{
		write(tty, "mkdir: md [dir]\n", 14);
		return;
	}
	char newdir[256];
	int len = strlen(workdir);
	strcpy(newdir, workdir);
	if (strchrs(argv[1], "<>:,*?/\\"))
	{
		write(tty, "ERROR: not a valid name\n", 25);
		return;
	}
	newdir[len++] = PATH_DEL;
	strcpy(newdir + len, argv[1]);
	len += strlen(argv[1]);
	fat_opendir("V:");
	int state = fat_createdir(newdir);
	if (state != 1)
	{
		write(tty, "ERROR: directory exists\n", 25);
	}
	//fat_opendir(workdir);
}

void builtin_pwd()
{
	char wbuf[256];
	strcpy(wbuf, workdir);
	int len = strlen(wbuf);
	wbuf[len++] = '\n';
	write(tty, wbuf, len);
}

void builtin_rmdir()
{
	if (argc == 1)
	{
		write(tty, "rmdir: rd [dir]\n", 14);
		return;
	}
	char pathname[256];
	int len = strlen(workdir);
	strcpy(pathname, workdir);
	pathname[len++] = PATH_DEL;
	strcpy(pathname, argv[1]);
	fat_opendir("V:");
	int state = fat_deletedir(pathname);
	if (state != 1)
	{
		write(tty, "ERROR: directory not exists\n", 29);
	}
	//fat_opendir(workdir);
}

void main()
{
	tty = open("dev_tty0", O_RDWR);
	char rbuf[256];
	workdir[0] = PATH_DEL;
	while (1)
	{
		int len = read(tty, rbuf, 255);
		rbuf[len] = 0;
		parse_args(rbuf);
		if (strcmp(argv[0], "cd"))
		{
			builtin_chdir();
		}
		if (strcmp(argv[0], "mkdir"))
		{
			builtin_mkdir();
		}
		if (strcmp(argv[0], "pwd"))
		{
			builtin_pwd();
		}
		if (strcmp(argv[0], "rmdir"))
		{
			builtin_rmdir();
		}
	}
}