/* getprocbyargs.c - simple program to test get_proc_by_args */

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
  printf ("Usage: %s ARGS\n", prog);
  exit (1);
}

int
main (int argc, char *argv[])
{
  char **args;
  char *prog;
  int argno, nargs;
  int res = 0;
  pid_t pid;

  if ((prog = strrchr (argv[0], '/')))
    prog++;
  else
    prog = argv[0];

  if (argc < 2)
    usage (prog);

  nargs = argc - 1;
  if (! (args = calloc (nargs + 1, sizeof (char *)))) {
    fprintf (stderr, "%s: calloc: %s", prog, strerror (errno));
    res = 1;
    goto cleanup;
  }

  for (argno = 0; argno < nargs; argno++) {
    if (! (args[argno] = strdup (argv[argno + 1]))) {
      fprintf (stderr, "%s: strdup: %s\n", prog, strerror (errno));
      res = 1;
      goto cleanup;
    }
  }

  if ((pid = get_proc_by_args (args)) != (pid_t)-1)
    printf ("process identifier is %u\n", pid);
  else
    printf ("unknown process, %s\n", strerror (errno));

cleanup:
  if (args) {
    for (argno = 0; argno < nargs; argno++) {
      if (args[argno])
        free (args[argno]);
    }
    free (args);
  }

  return res;
}
