/* system includes */
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* third party includes */
#include "interfaces.h"

/* prototypes */
void usage (const char *);
void version (const char *);

void
usage (const char *program)
{
  char *fmt =
  "Usage: %s <options> <ifaces...>\n"
  "\n"
  "Options:\n"
  "\t-h, --help\t\tthis help\n"
  "\t-V, --versions\t\tcopyright and version information\n"
  "\t-i, --interfaces FILE\tuse FILE for interface definitions\n";

  printf (fmt, program);
  exit (0);
}

void
version (const char *program)
{
  const char *fmt =
  "%s version %s\n"
  "Copyright (c) 2011 Jeroen Koekkoek\n"
  "\n"
  "This program is free software; you can redistribute it and/or modify\n"
  "it under the terms of the GNU General Public License as published by\n"
  "the Free Software Foundation; either version 2 of the License, or (at\n"
  "your option) any later version.\n";

  printf (fmt, program, "0.1");
  exit (0);
}


int
main (int argc, char *argv[])
{
  char *cmd, *path;
  char *opts = "i:hV";
  char *interfaces = "/etc/network/interfaces";
  interfaces_file *defs;
  struct option long_opts[] = {
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'V'},
    {"interfaces", required_argument, NULL, 'i'}
  };

  path = argv[0];
  if ((cmd = strrchr (path, '/'))) {
    cmd++;
  } else {
    cmd = path;
  }

  if (strcmp (cmd, "vipup") != 0 && strcmp (cmd, "vipdown") != 0) {
    fprintf (stderr, "This command should be called as vipup or vipdown\n");
    exit (1);
  }

  /* parse command line options */
  int c;
  for (; (c = getopt_long (argc, argv, opts, long_opts, NULL)) != EOF ;) {
    switch (c) {
      case 'i':
        interfaces = strdup (optarg);
        break;
      case 'b':
        break;
      default:
        usage (argv[0]);
        break; /* never reached */
    }
  }

  /* parse interfaces file */
  if (! (defs = read_interfaces (interfaces))) {
    fprintf (stderr, "%s: couldn't read interfaces file \"%s\"\n", path, interfaces);
    exit (1);
  }

  //
  // which interfaces should be parsed
  //
  interface_defn *iface, *ifaces;
  ifaces = defs->ifaces;

  variable *ifopt, *ifopts;
  int i, nifopts;

  /*
   * print all interfaces to start with!
   */
//  for (iface = ifaces; iface; iface = iface->next) {
//    printf ("logical_iface: %s, real_iface: %s\n", iface->logical_iface, iface->real_iface);
//
//    ifopts = iface->option;
//    nifopts = iface->n_options;
//
//    for (i = 0; i < nifopts; i++) {
//      ifopt = ifopts+i;
//      printf ("\toption: %s, value: %s\n", ifopt->name, ifopt->value);
//    }
//  }

  // now export the right environment variables
  // I think I can just use the setlocalenv function
  // and then just call some function like execute and run the ucarp script
  // in /etc/network/if-up.d/ucarp


  char *instance, *prefix;

  for (argn = optind



  /* find the right interface */
  for (iface = ifaces; iface; iface = iface->next) {
    if (strcmp (iface->logical_iface, prefix) == 0)
      break;
  }


  char *env_vars;
  int nenv_vars = 8;

  for (iface = ifaces; iface; iface = iface->next) {
    ifopts = iface->option;
    nifopts = iface->n_options;

    for (i = 0; i < nifopts; i++) {
      
    }
  }


  return 0;

  // 1. right... parse command line options... use some code from ifupdown!
  //    as an example... or just copy!
  // >>>> REMEMBER... THINK OF WHAT OPTIONS SHOULD BE SUPPORTED FIRST!!!
  //    remember... they should only be able to specify ucarp interfaces... this
  //    program is called VIPUPDOWN
  // 2. parse the interfaces file


  // ... the interfaces on which the virtual interface is bound should of couse
  //     be up!
  //     ... right continue etc
}










































