#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

/* system includes */
#include <sys/types.h>

/* ifupdown includes */
#include "ifupdown.h"

pid_t get_proc_by_args (char **);
time_t get_proc_starttime (pid_t);
interface_defn *getifbyattr (interface_defn *, const char *, const char *);
char *getifattrbyname (interface_defn *, const char *);

#endif
