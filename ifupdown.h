#ifndef IFUPDOWN_H_INCLUDED
#define IFUPDOWN_H_INCLUDED

/* ifupdown.h exposes some variables and functions in ifupdown that are needed
   by vipupdown */

#include "ifupdown/header.h"

/* default location for interfaces file is a char * in the main function of
   ifupdown, therefore I simply cannot have a reference to it here */
#define INTERFACES "/etc/network/interfaces"

/* extern int no_act; defined in header.h */
/* extern int verbose; defined in header.h */
extern char **environ;
extern char *statefile;
extern char *tmpstatefile;

char *setlocalenv(char *, char *, char *);
int doit(char *);
const char *read_state(const char *, const char *);
void free_interfaces (interfaces_file *);

#endif
