#pragma once
#include "stdio.h"
int listdir(const char* dirname);
int findfile(const char* filename);
int fat_opendir(char * dirname);
int fat_createdir(char *dirname);
int fat_deletedir(char *dirname);

int strcmp(const char *s1, const char *s2);
char* strrchr(char *s, int c);