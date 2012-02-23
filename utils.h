#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

/* system includes */
#include <sys/types.h>

#define KILL_PROC_INTVL (1) /* seconds */

pid_t get_proc_by_args (char **);
time_t get_proc_starttime (pid_t);
int kill_proc (pid_t, int, int);

#endif
