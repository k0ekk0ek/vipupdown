#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED 1

#include <stdarg.h>
#include <sys/types.h>

typedef struct _cmd cmd_t;

struct _cmd {
  char **argv;
  int argc;
};

cmd_t *cmdfargs (const char *, const char *, ...);
void cmdfree (cmd_t *);
pid_t getpidbycmd (cmd_t *);
cmd_t *read_cmdline (const char *);

#endif

