#include "util.h"
#include "stdio.h"

extern int tty;

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

