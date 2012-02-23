/* system includes */
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ifupdown includes */
#include "ifupdown.h"

/* prototypes */
void help (const char *);
void usage (const char *);
void version (const char *, const char *);

#define VERSION "0.1"

void
help (const char *prog)
{
  const char *fmt =
  "Usage: %s <options> IFACE ADDR\n"
  "\n"
  "\t-h, --help\t\tthis help\n"
  "\t-V, --version\t\tcopyright and version information\n"
  "\t-i, --interfaces FILE\tuse FILE for interface definitions\n";

  printf (fmt, prog);
  exit (0);
}

void
usage (const char *prog)
{
  char *fmt =
  "%s: Use --help for help\n";

  printf (fmt, prog);
  exit (1);
}

void
version (const char *prog, const char *version)
{
  const char *fmt =
  "%s version %s\n"
  "Copyright (c) 2011 Jeroen Koekkoek\n"
  "\n"
  "This program is free software; you can redistribute it and/or modify\n"
  "it under the terms of the GNU General Public License as published by\n"
  "the Free Software Foundation; either version 2 of the License, or (at\n"
  "your option) any later version.\n";

  printf (fmt, prog, version);
  exit (0);
}

int
main (int argc, char *argv[])
{
  char *prog;
  char *interfaces = NULL;
  char *ptr, *tail;
  char *ucarp_name;
  char *virt_addr, virt_name[80];
  int c;
  int cnt, optno;
  int res = 0;
  interfaces_file *iface_file = NULL;
  interface_defn *iface, *ifaces, *ucarp_iface;

  struct option long_opts[] = {
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'V'},
    {"interfaces", required_argument, NULL, 'i'},
    {0, 0, 0, 0}
  };

  if ((prog = strrchr (argv[0], '/')))
    prog++;
  else
    prog = argv[0];

  for (;;) {
    if ((c = getopt_long (argc, argv, "i:hV", long_opts, NULL)) == EOF)
      break;

    switch (c) {
      case 'h':
        help (prog);
        break; /* never reached */
      case 'V':
        version (prog, VERSION);
        break; /* never reached */
      case 'i':
        interfaces = strdup (optarg);
        break;
      default:
        usage (prog);
        break; /* never reached */
    }
  }

  if ((optind + 2) > argc)
    usage (prog);

  if (! interfaces)
    interfaces = strdup (INTERFACES);

  /* parse interfaces file */
  if (! (iface_file = read_interfaces (interfaces))) {
    fprintf (stderr, "%s: cannot read interfaces file \"%s\"\n",
      prog, interfaces);
    exit (1);
  }

  ucarp_iface = NULL;
  ucarp_name = argv[optind];
  virt_addr = argv[optind + 1];

  /* find interface ucarp binds to */
  ifaces = iface_file->ifaces;
  for (iface = ifaces; ! ucarp_iface && iface; iface = iface->next) {
    if (strcmp (iface->logical_iface, ucarp_name) == 0)
      ucarp_iface = iface;
  }

  ptr = NULL;
  tail = NULL;

  for (optno = 0; ! ptr && optno < ucarp_iface->n_options; optno++) {
    if (strncmp (ucarp_iface->option[optno].name, "ucarp-vip", 9) == 0 &&
        strcmp (ucarp_iface->option[optno].value, virt_addr) == 0)
      ptr = ucarp_iface->option[optno].name;
  }

  if (! ptr)
    goto cleanup;

  memset (virt_name, '\0', 80);
  if ((tail = strrchr (ptr, ':'))) {
    for (ptr = ++tail; isdigit (*ptr); ptr++)
      /* do noting */ ;
    if (*ptr)
      tail = NULL;
  }

  if (tail)
    cnt = snprintf (virt_name, 79, "%s:ucarp:%s", ucarp_name, tail);
  else
    cnt = snprintf (virt_name, 79, "%s:ucarp", ucarp_name);

  if (cnt < 0 || cnt > 79) {
    fprintf (stderr, "%s: snprintf: %s\n", prog, strerror (errno));
    goto error;
  }

  printf ("%s\n", virt_name);

  res = 0;
cleanup:
  if (interfaces)
    free (interfaces);
  if (iface_file)
    free_interfaces (iface_file);
  return res;
error:
  res = 1;
  goto cleanup;
}

