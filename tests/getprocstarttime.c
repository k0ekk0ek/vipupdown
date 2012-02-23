/* getprocstarttime.c - simple program to test get_proc_startime */

/* system includes */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

/* vipupdown includes */
#include "../utils.h"

void
usage (const char *prog)
{
  printf ("Usage: %s PID\n", prog);
  exit (1);
}

int
main (int argc, char *argv[])
{
  char *prog, *str;
  int orig_errno;
  pid_t pid;
  time_t starttime;
  struct tm *tm;

  if ((prog = strrchr (argv[0], '/')))
    prog++;
  else
    prog = argv[0];

  if (argc < 2)
    usage (prog);

  orig_errno = errno;
  errno = 0;
  if (! (pid = strtol (argv[1], NULL, 10)) && errno)
    usage (prog);
  errno = orig_errno;

  if ((starttime = get_proc_starttime (pid)) == (time_t)-1) {
    fprintf (stderr, "%s: get_proc_starttime: %s\n", prog, strerror (errno));
    exit (1);
  }

  printf ("proces running since %s\n", asctime (localtime (&starttime)));

  return 0;
}
