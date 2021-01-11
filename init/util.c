#include "util.h"
#include "stdio.h"

extern int tty;

static int strcmp(const char * s1, const char *s2)
{
    if ((s1 == 0) || (s2 == 0)) { /* for robustness */
        return (s1 - s2);
    }

    const char * p1 = s1;
    const char * p2 = s2;

    for (; *p1 && *p2; p1++,p2++) {
        if (*p1 != *p2) {
            break;
        }
    }

    return (*p1 - *p2);
}

int listdir(const char* dirname) {
  unsigned int entry[3] = {0};
  char name[13];
  int state;

  write(tty, "\n", 1);
  while ((state = readdir(dirname, entry, name)) == 1) {
    write(tty, name, strlen(name));
    write(tty, "\n", 1);
  }
  return state;
}

int fat_opendir(char * dirname) {
    char name[256] = {0};
    char *prefix = "fat0/";
    int prefix_len = strlen(prefix);
    memcpy(name, prefix, prefix_len + 1);
    memcpy(name + prefix_len, dirname, strlen(dirname) + 1);
    return opendir(name);
}

static int m_findfile(const char *filename, char path[256]) {
    printf("enter m_findfile: %s\n", path);
    unsigned int entry[3] = {0};
    char name[13];
    int found = 0;
    while (readdir(".", entry, name) == 1) {
        printf("found file: %s\n", name);
        if (strcmp(name, filename) == 0) {
            printf("%s\\%s\n", path, name);
            found = 1;
        }
        if (fat_opendir(name) == 1) {
            int len = strlen(path);
            path[len] = '\\';
            memcpy(path + len + 1, name, strlen(name) + 1);

            found = found || m_findfile(filename, path);

            path[len] = 0;
        }
    }
    return found;
}

int findfile(const char* filename) {
    char path[256] = {0};

    write(tty, "\n", 1);
    return m_findfile(filename, path);
}
