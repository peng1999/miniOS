#pragma once
#include "stdio.h"
int listdir(const char* dirname);
int findfile(const char* filename);
int fat_opendir(char * dirname);
int fat_createdir(char *dirname);
int fat_deletedir(char *dirname);
int chdir(const char *path);

int fprintf(int fd, const char *fmt, ...);
int strcmp(const char *s1, const char *s2);
char* strrchr(char *s, int c);
extern char workdir[256];
