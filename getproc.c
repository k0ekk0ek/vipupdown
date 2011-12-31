/* system includes */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* vipupdown includes */
#include "utils.h"

void
usage (void)
{
  fprintf (stderr, "Usage: getproc PATH [OPTIONS]\n");
  exit (1);
}

int
main (int argc, char *argv[])
{
  pid_t pid;

  if (argc < 2)
    usage ();

  cmd_t cmd;
  cmd.argv = argv+1;
  cmd.argc = argc-1;

  pid = getpidbycmd (&cmd);
  if (pid < 1)
    printf ("process not found\n");
  else
    printf ("process identifier %d\n", pid);
  return 0;
}

