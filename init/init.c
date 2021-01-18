// added by rzr, pgw, 2020
#include "util.h"

#define O_CREAT 1
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

void builtin_cat()
{
	if (argc == 1)
	{
		write(tty, "cat: cat [file]\n", 17);
		return;
	}
	int total;
	int len;
	char fullpath[256];
	strcpy(fullpath, "fat0/V:");
	strcpy(fullpath + 7, workdir);
	len = strlen(fullpath);
	if (fullpath[len - 1] != PATH_DEL)
	{
		fullpath[len++] = PATH_DEL;
	}
	strcpy(fullpath + len, argv[1]);
	char buf[512];
	int fd = open(fullpath, O_RDWR);
	if (fd <= 0)
	{
		write(tty, "ERROR: file not exists\n", 24);
		return;
	}
	total = 0;
	len = read(fd, buf, 512);
	while (len > 0)
	{
		total += len;
		write(tty, buf, len);
		len = read(fd, buf, 512);
	}
	fprintf(tty, "cat: read %d bytes\n", total);
	close(fd);
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
		//opendir(fullpath);
		chdir(fullpath);
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
	//if (opendir(fullpath) != 1)
	if (chdir(fullpath) != 1)
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

void builtin_find()
{
    if (argc == 1)
    {
        fprintf(tty, "find: find [filename]\n");
        return;
    }
    findfile(argv[1]);
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
	getcwd(wbuf, 256);
	strcpy(wbuf, wbuf + 2);
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
	strcpy(pathname + len, argv[1]);
	strcpy(fullpath, "fat0/V:");
	strcpy(fullpath + 7, pathname);
	int state = deletedir(fullpath);
	if (state != 1)
	{
		write(tty, "ERROR: directory not exists\n", 29);
	}
}

void write_test()
{
	fprintf(tty, "FAT32 write test start\n");
	char buf[9000];
	char *optr = buf;
	int len;
	for (int i = 1; i <= 2000; i++)
	{
		len = sprintf(optr, "%d\n", i);
		optr += len;
	}
	int total;
	total = optr - buf;
	optr = buf;
	createdir("fat0/b");
	chdir("fat0/b");
	int fd;
	fd = open("fat0/2000.txt", O_RDWR | O_CREAT);
	len = 512;
	while (optr < buf + total)
	{
		if (optr + len > buf + total)
		{
			len = buf + total - optr;
		}
		write(fd, optr, len);
		optr += len;
	}
	close(fd);
	fprintf(tty, "write %d bytes\n", total);
	fprintf(tty, "FAT32 write test finish\n");
}

void fat32_test()
{
	fprintf(tty, "FAT32 test begin\n");
	int pid_child = fork();
	int buf[256];
	int fd;
	int i = 0;
	if (!pid_child)
	{
		getcwd(buf, 256);
		fprintf(tty, "child cwd is: %s\n", buf);
		createdir("fat0/child");
		chdir("fat0/child");
		getcwd(buf, 256);
		fprintf(tty, "child cwd changes to: %s\n", buf);
		fd = open("fat0/path.txt", O_RDWR | O_CREAT);
		write(fd, buf, strlen(buf));
		close(fd);
		fprintf(tty, "child process exit\n");
		while (1);
	}
	getcwd(buf, 256);
	fprintf(tty, "parent cwd is: %s\n", buf);
	createdir("fat0/parent");
	chdir("fat0/parent");
	getcwd(buf, 256);
	fprintf(tty, "parent cwd changes to: %s\n", buf);
	fd = open("fat0/path.txt", O_RDWR | O_CREAT);
	write(fd, buf, strlen(buf));
	close(fd);
	fprintf(tty, "parent process exit\n");
	while (1);
}

void builtin_tee()
{
	if (argc == 1)
	{
		write(tty, "tee: tee [file]\n", 17);
		return;
	}
	int len;
	char fullpath[256];
	strcpy(fullpath, "fat0/V:");
	strcpy(fullpath + 7, workdir);
	len = strlen(fullpath);
	if (fullpath[len - 1] != PATH_DEL)
	{
		fullpath[len++] = PATH_DEL;
	}
	strcpy(fullpath + len, argv[1]);
	char buf[512];
	int fd = open(fullpath, O_RDWR | O_CREAT);
	if (fd <= 0)
	{
		write(tty, "ERROR: file not exists\n", 24);
		return;
	}
	len = read(tty, buf, 512);
	write(fd, buf, len);
	close(fd);
}

void main()
{
	tty = open("dev_tty0", O_RDWR);
	char rbuf[256];
	strcpy(workdir, "\\");
	//进程工作目录初始化
	chdir("fat0/V:\\");
	while (1)
	{
		fprintf(tty, "miniOS:%s $ ", workdir);
		int len = read(tty, rbuf, 255);
		rbuf[len] = 0;
		parse_args(rbuf);
		if (!strcmp(argv[0], "cat"))
		{
			builtin_cat();
		}
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
		if (!strcmp(argv[0], "find"))
        {
		    builtin_find();
        }
		if (!strcmp(argv[0], "tee"))
		{
			builtin_tee();
		}
		if (!strcmp(argv[0], "fat32_test"))
		{
			fat32_test();
		}
		if (!strcmp(argv[0], "write_test"))
		{
			write_test();
		}
	}
}
