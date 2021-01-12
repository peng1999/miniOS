#include "util.h"

#define O_RDWR 2
#define PATH_DEL '\\'

int argc;
char argv[8][256];

extern int tty;

char workdir[256];

void parse_args(char *rbuf)
{
	argc = 0;
	char *iptr = rbuf;
	char *optr = argv[argc];
	while (*iptr != '\n')
	{
		if (*iptr != ' ')
		{
			*optr++ = *iptr++;
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
	if (!strcmp(argv[1], "."))
	{
		return;
	}
	char pathname[256];
	char fullpath[256];
	char wbuf[256];
	int len = strlen(workdir);
	strcpy(pathname, workdir);
	if (!strcmp(argv[1], ".."))
	{
		char *trunc = strrchr(pathname + 1, PATH_DEL);
		if (trunc)
		{
			*trunc = 0;
		}
		else
		{
			pathname[1] = 0;
		}
		strcpy(fullpath, "fat0/V:");
		strcpy(fullpath + 7, pathname);
		opendir(fullpath);
		strcpy(workdir, pathname);
		return;
	}
	if (pathname[len - 1] != PATH_DEL)
	{
		pathname[len++] = PATH_DEL;
	}
	strcpy(pathname + len, argv[1]);
	strcpy(fullpath, "fat0/V:");
	strcpy(fullpath + 7, pathname);
	if (opendir(fullpath) != 1)
	{
		write(tty, "ERROR: no such directory\n", 26);
		return;
	}
	strcpy(workdir, pathname);
}

void builtin_lsdir()
{
	char fullpath[256];
	strcpy(fullpath, "V:");
	strcpy(fullpath + 2, workdir);
	listdir(fullpath);
}

void builtin_mkdir()
{
	if (argc == 1)
	{
		write(tty, "mkdir: mkdir [dir]\n", 14);
		return;
	}
	char pathname[256];
	char fullpath[256];
	int len = strlen(workdir);
	strcpy(pathname, workdir);
	if (strchrs(argv[1], "<>:,*?/\\"))
	{
		write(tty, "ERROR: not a valid name\n", 25);
		return;
	}
	if (pathname[len - 1] != PATH_DEL)
	{
		pathname[len++] = PATH_DEL;
	}
	strcpy(pathname + len, argv[1]);
	strcpy(fullpath, "fat0/V:");
	strcpy(fullpath + 7, pathname);
	int state = createdir(fullpath);
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
		write(tty, "rmdir: rmdir [dir]\n", 14);
		return;
	}
	char pathname[256];
	char fullpath[256];
	int len = strlen(workdir);
	strcpy(pathname, workdir);
	if (pathname[len - 1] != PATH_DEL)
	{
		pathname[len++] = PATH_DEL;
	}
	strcpy(pathname, argv[1]);
	strcpy(fullpath, "fat0/V:");
	strcpy(fullpath + 7, pathname);
	int state = deletedir(fullpath);
	if (state != 1)
	{
		write(tty, "ERROR: directory not exists\n", 29);
	}
}

void main()
{
	// int state;
	// state = fat_createdir("V:\\a");
	// state = fat_opendir("V:\\a");
	// state = fat_opendir("V:");
	// state = fat_createdir("V:\\a\\b");
	// state = fat_opendir("V:\\a\\b");
	// state = fat_opendir("V:");
	// state = fat_createdir("V:\\a\\b\\c");
	// state = fat_opendir("V:\\a\\b\\c");
	// while (1);
	tty = open("dev_tty0", O_RDWR);
	char rbuf[256];
	workdir[0] = PATH_DEL;
	while (1)
	{
		printf("miniOS:%s $ ", workdir);
		int len = read(tty, rbuf, 255);
		rbuf[len] = 0;
		parse_args(rbuf);
		if (!strcmp(argv[0], "cd"))
		{
			builtin_chdir();
		}
		if (!strcmp(argv[0], "ls"))
		{
			builtin_lsdir();
		}
		if (!strcmp(argv[0], "mkdir"))
		{
			builtin_mkdir();
		}
		if (!strcmp(argv[0], "pwd"))
		{
			builtin_pwd();
		}
		if (!strcmp(argv[0], "rmdir"))
		{
			builtin_rmdir();
		}
	}
}