/* system includes */
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* vipupdown includes */
#include "vipupdown.h"

/* prototypes */
void help (void);
void usage (void);
void version (void);

void
help (void)
{
  const char *fmt =
  "Usage: vipif <options> <virtual ifaces...>\n"
  "\n"
  "\t-h, --help\t\tthis help\n"
  "\t-V, --version\t\tcopyright and version information\n"
  "\t-i, --interfaces FILE\tuse FILE for interface definitions\n";

  printf (fmt);
  exit (0);
}

void
usage (void)
{
	fprintf(stderr, "vipif: Use --help for help\n");
	exit(1);
}

void
version (void)
{
  const char *fmt =
  "vipif version %s\n"
  "Copyright (c) 2011 Jeroen Koekkoek\n"
  "\n"
  "This program is free software; you can redistribute it and/or modify\n"
  "it under the terms of the GNU General Public License as published by\n"
  "the Free Software Foundation; either version 2 of the License, or (at\n"
  "your option) any later version.\n";

  printf (fmt, VIPUPDOWN_VERSION);
  exit (0);
}

int
main (int argc, char *argv[])
{
  char *path = VIPUPDOWN_INTERFACES;
  char *vip;
  int niface_opts;
  int c, i, j;
  interfaces_file *iface_defs;
  interface_defn *iface, *ifaces, *ucarp_iface, *vip_iface;
  variable *iface_opts;

  struct option long_opts[] = {
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'V'},
    {"interfaces", required_argument, NULL, 'i'},
    {0, 0, 0, 0}
  };

  for (;;) {
    if ((c = getopt_long (argc, argv, "i:hV", long_opts, NULL)) == EOF)
      break;

    switch (c) {
      case 'h':
        help ();
        break;
      case 'i':
        path = optarg;
        break;
      case 'V':
        version ();
        break;
    }
  }

  iface_defs = read_interfaces (path);
  if (! iface_defs) {
    fprintf (stderr, "vipif: couldn't read interfaces file \"%s\"\n", path);
    exit (1);
  }

  /* need at least one virtual IP address */
  if (optind == argc)
    usage ();

  /* user is allowed to specify multiple virtual IP addresses */
  for (i = optind; i < argc; i++) {
    vip = argv[i];
    vip_iface = NULL;
    ucarp_iface = NULL;

    ifaces = iface_defs->ifaces;
    for (iface = ifaces; iface; iface = iface->next) {
      iface_opts = iface->option;
      niface_opts = iface->n_options;

      for (j = 0; j < niface_opts; j++) {
        if (strcmp (iface_opts[j].name, "address") == 0) {
          if (strcmp (iface_opts[j].value, vip) == 0) {
            vip_iface = iface;
            if (vip_iface && ucarp_iface)
              break;
          }
        } else if (strncmp (iface_opts[j].name, "ucarp-vip", 9) == 0) {
          if (strcmp (iface_opts[j].value, vip) == 0) {
            ucarp_iface = iface;
            if (vip_iface && ucarp_iface)
              break;
          }
        }
      }
    }

    /* address must be defined as "ucarp-vip" in the interface UCARP binds to,
       and as "address" in the actual interface */
    if (vip_iface && ucarp_iface) {
      printf ("%s\n", vip_iface->logical_iface);
    } else {
      printf ("\n");
    }
  }

  return 0;
}

