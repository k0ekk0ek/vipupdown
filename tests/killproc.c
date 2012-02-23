/* killproc.c - simple program to test kill_proc */

/* system includes */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* vipupdown includes */
#include "../utils.h"

void
usage (const char *prog)
{
  printf ("Usage: %s PID [SIGNAL] [RETRY]\n", prog);
  exit (1);
}

int
main (int argc, char *argv[])
{
  char *prog;
  int orig_errno;
  int nrtys, rty, sig;
  pid_t pid;

  if ((prog = strrchr (argv[0], '/')))
    prog++;
  else
    prog = argv[0];

  if (argc < 2 || argc > 4)
    usage (prog);

  orig_errno = errno;
  errno = 0;

  if (! (pid = strtol (argv[1], NULL, 10)) && errno)
    usage (prog);

  sig = 15;
  rty = 5;

  if (argc > 2 && ! (sig = strtol (argv[2], NULL, 10)) && errno)
    usage (prog);
  if (argc > 3 && ! (rty = strtol (argv[3], NULL, 10)) && errno)
    usage (prog);

  errno = orig_errno;

  nrtys = kill_proc (pid, sig, rty);
  if (nrtys < 0)
    fprintf (stderr, "%s: kill_proc: %s\n", prog, strerror (errno));
  else if (nrtys <= rty)
    fprintf (stderr, "killed process %u\n", pid);
  else
    fprintf (stderr, "process %u refused to die\n", pid);

  return 0;
}

