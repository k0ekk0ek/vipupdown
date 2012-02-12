/* system includes */
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ifupdown includes */
#include "ifupdown.h"

/* vipupdown includes */
#include "utils.h"

/* prototypes */
void usage (const char *);
void version (const char *, const char *);
char **get_all_virt_names (const char *, interface_defn *);
char **get_deduped_virt_names (const char *, int, char **);
int vipup (interface_defn *, interface_defn *);
int vipdown (interface_defn *, interface_defn *);

char ucarp[] = "/usr/sbin/ucarp";
char ucarp_up[] = "/etc/network/if-up.d/ucarp";

void
usage (const char *prog)
{
  char *fmt =
  "Usage: %s <options> <ifaces...>\n"
  "\n"
  "Options:\n"
  "\t-h, --help\t\tthis help\n"
  "\t-V, --version\t\tcopyright and version information\n"
  "\t-a, --all\t\tde/configure all virtual interfaces\n"
  "\t-i, --interfaces FILE\tuse FILE for interface definitions\n"
  "\t-n, --no-act\t\tprint out what would happen, but don't do it\n"
  "\t-v, --verbose\t\tprint out what would happen before doing it\n";

  printf (fmt, prog);
  exit (0);
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

char **
get_all_virt_names (const char *prog, interface_defn *ifaces)
{
  interface_defn *iface;
  char *name, *ptr;
  char **tmp_names, **names = NULL;
  int nnames = 0;
  int i, len;
  int cnt;
  size_t size;

  for (iface = ifaces; iface; iface = iface->next) {
    /* search options for ucarp-vip */
    for (i = 0; i < iface->n_options; i++) {
      if (strncmp (iface->option[i].name, "ucarp-vip", 9) == 0) {
        /* find first position of trailing digits */
        if ((ptr = strrchr (iface->option[i].name, ':'))) {
          len = strlen (iface->logical_iface) + strlen (++ptr) + 8;
        } else {
          len = strlen (iface->logical_iface) + 7;
        }

        if (! (name = malloc (len + 1))) {
          fprintf (stderr, "%s: malloc: %s\n", prog, strerror (errno));
          goto error;
        }

        if (ptr) {
          cnt = snprintf (name, len, "%s:ucarp:%s", iface->logical_iface, ptr);
        } else {
          cnt = snprintf (name, len, "%s:ucarp", iface->logical_iface);
        }

        if (cnt < 0 || cnt > len) {
          fprintf (stderr, "%s: snprintf: %s\n", prog, strerror (errno));
          goto error;
        }

        size = (nnames + 2) * sizeof (char *);
        if (! (tmp_names = realloc (names, size))) {
          fprintf (stderr, "%s: realloc: %s\n", prog, strerror (errno));
          goto error;
        }
        names = tmp_names;
        names[ nnames++ ] = name;
        names[ nnames   ] = NULL;
      }
    }
  }

  return names;
error:
  if (names) {
    for (i = 0; names[i]; i++)
      free (names[i]);
    free (names);
  }
  return NULL;
}

char **
get_deduped_virt_names (const char *prog, int argc, char *argv[])
{
  char **tmp_names, **names = NULL;
  int nnames = 0;
  int i, j, already;
  size_t size;

  for (i = 0; i < argc; i++) {
    already = 0;

    /* ignore duplcites */
    for (j = 0; ! already && j < i; j++) {
      if (strcmp (argv[j], argv[i]) == 0)
        already = 1;
    }

    if (! already) {
      size = (nnames + 2) * sizeof (char *);
      if (! (tmp_names = realloc (names, size))) {
        fprintf (stderr, "%s: realloc: %s\n", prog, strerror (errno));
        goto error;
      }
      names = tmp_names;
      names[ nnames     ] = NULL;
      names[ nnames + 1 ] = NULL;
      if (! (names[nnames] = strdup (argv[i]))) {
        fprintf (stderr, "%s: strdup: %s\n", prog, strerror (errno));
        goto error;
      }
      nnames++;
    }
  }

  return names;
error:
  if (names) {
    for (i = 0; names[i]; i++)
      free (names[i]);
    free (names);
  }
  return NULL;
}

int
start_ucarp_daemon (const char *prog,
                    interface_defn *ucarp_iface,
                    interface_defn *virt_iface)
{
  char **env;
  char *postfix, *ptr;
  int i, nenv, export;

  /* directly copied from set_environ as implemented in execute.c */
  if (environ) {
    for (env = environ; *env; env++) {
      free (*env);
      *env = NULL;
    }
    free (environ);
    environ = NULL;
  }

  nenv = ucarp_iface->n_options + 8;

  if (! (environ = calloc (nenv + 1, sizeof (char *)))) {
    fprintf (stderr, "%s: calloc: %s", prog, strerror (errno));
    goto error;
  }

  env = environ;

  /* export environment variables needed to bring up ucarp */
  if ((postfix = strrchr (virt_iface->logical_iface, ':'))) {
    postfix++;
    if (! isdigit (*postfix))
      postfix = NULL;
  }

  for (i = 0; i < ucarp_iface->n_options; i++) {
    export = 0;

    if (strncmp (ucarp_iface->option[i].name, "ucarp-", 6) == 0) {
      ptr = strchr (ucarp_iface->option[i].name, ':');
      if ((ptr && postfix && strcmp (++ptr, postfix) == 0) || (! ptr && ! postfix))
        export = 1;
    } else if (strcmp(ucarp_iface->option[i].name, "pre-up")    != 0 &&
               strcmp(ucarp_iface->option[i].name, "up")        != 0 &&
               strcmp(ucarp_iface->option[i].name, "down")      != 0 &&
               strcmp(ucarp_iface->option[i].name, "post-down") != 0)
    {
      export = 1;
    }


    if (export)
      *(env++) = setlocalenv ("IF_%s=%s", ucarp_iface->option[i].name, ucarp_iface->option[i].value);
  }

  /* export generic environment variables */
  *(env++) = setlocalenv("%s=%s", "IFACE",     ucarp_iface->logical_iface);
  *(env++) = setlocalenv("%s=%s", "LOGICAL",   ucarp_iface->logical_iface);
  *(env++) = setlocalenv("%s=%s", "ADDRFAM",   ucarp_iface->address_family->name);
  *(env++) = setlocalenv("%s=%s", "METHOD",    ucarp_iface->method->name);
  *(env++) = setlocalenv("%s=%s", "MODE",      "start");
  *(env++) = setlocalenv("%s=%s", "PHASE",     "post-up");
  *(env++) = setlocalenv("%s=%s", "VERBOSITY", verbose ? "1" : "0");
  *(env++) = setlocalenv("%s=%s", "PATH",      "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin");
  *(env)   = NULL;

  if (! doit (ucarp_up)) {
    fprintf (stderr, "%s: failed to start ucarp for interface %s\n",
      prog, virt_iface->logical_iface);
  }

  for (env = environ; *env; env++) {
    free (*env);
    *env = NULL;
  }
  free (environ);
  environ = NULL;

  return 0;
error:
  return -1;
}

int
stop_ucarp_daemon (const char *prog,
                   pid_t pid,
                   interface_defn *ucarp_iface,
                   interface_defn *virt_iface)
{
  int j, orig_errno;
  time_t ucarp_starttime, starttime;

  if (! no_act) {
    ucarp_starttime = get_proc_starttime (pid);
    if (ucarp_starttime == (time_t)-1) {
      fprintf (stderr, "%s: failed to get ucarp starttime for interface %s: %s\n",
        prog, virt_iface->logical_iface, strerror (errno));
      goto error;
    }

    for (j = 0; j < 5; j++) {
      /* check if starttime of process to be killed is less than starttime of
         of ucarp daemon to make sure we're not killing some newly created
         process */
      errno = 0;
      starttime = get_proc_starttime (pid);
      if (starttime == (time_t)-1) {
        if (errno) {
          fprintf (stderr, "%s: failed to get ucarp starttime for interface: %s\n",
            prog, virt_iface->logical_iface, strerror (errno));
          goto error;
        }
        /* no starttime, process must have died */
        break;
      } else if (starttime < ucarp_starttime) {
        /* starttime is less than ucarp_starttime, that is impossible */
        break;
      }

      if (kill (pid, SIGTERM) < 0) {
        if (errno == ESRCH) {
          /* process must have died */
          break;
        } else {
          fprintf (stderr, "%s: failed to stop ucarp for interface %s: %s\n",
            prog, virt_iface->logical_iface, strerror (errno));
          goto error;
        }
      } else if (j > 0) {
        /* give ucarp daemon some time to terminate */
        sleep (1);
      }
    }

    starttime = get_proc_starttime (pid);
    if (starttime == (time_t)-1 || starttime < ucarp_starttime) {
      fprintf (stderr, "%s: interface %s unconfigured\n",
         prog, virt_iface->logical_iface);
    } else {
      fprintf (stderr, "%s: failed to stop ucarp for interface %s: %s\n",
        prog, virt_iface->logical_iface, strerror (errno));
    }
  } else {
    fprintf (stderr, "%s: stop ucarp for interface %s\n",
      prog, virt_iface->logical_iface);
  }

  errno = orig_errno;
  return 0;
error:
  return -1;
}

int
main (int argc, char *argv[])
{
  char *prog;
  char *args[4];
  char *opts = "i:hVanv";
  char *interfaces = INTERFACES;
  char *state;
  char *virt_addr;
  char *virt_name, **virt_names;
  char ucarp_name[80];
  char *ptr;
  int all, already;
  int c, i, j, len;
  int orig_errno;
  interfaces_file *defs;
  interface_defn *iface, *ifaces;
  interface_defn *virt_iface;
  interface_defn *ucarp_iface;
  struct option long_opts[] = {
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'V'},
    {"all", no_argument, NULL, 'a'},
    {"interfaces", required_argument, NULL, 'i'},
    {"no-act", no_argument, NULL, 'n'},
    {"verbose", no_argument, NULL, 'v'}
  };

  environ = NULL;
  pid_t pid;

  if ((prog = strrchr (argv[0], '/'))) {
    prog++;
  } else {
    prog = argv[0];
  }

  if (strcmp (prog, "vipup") == 0) {
    //action = &vipup;
  } else if (strcmp (prog, "vipdown") == 0) {
    //action = &vipdown;
  } else {
    fprintf (stderr, "%s: invoke as vipup or vipdown\n", prog);
    exit (1);
  }

  no_act = 0;
  verbose = 0;

  /* parse command line options */
  all = 0;
  for (; (c = getopt_long (argc, argv, opts, long_opts, NULL)) != EOF ;) {
    switch (c) {
      case 'a':
        all = 1;
        break;
      case 'i':
        interfaces = strdup (optarg);
        break;
      case 'b':
        break;
      case 'n':
        no_act = 1;
        break;
      case 'v':
        verbose = 1;
        break;
      default:
        usage (prog);
        break; /* never reached */
    }
  }

  /* at least one interface should be given if -a wasn't specified! */
  if (! all && optind == argc) {
    usage (prog);
  }

  /* parse interfaces file */
  if (! (defs = read_interfaces (interfaces))) {
    fprintf (stderr, "%s: cannot read interfaces file \"%s\"\n",
      prog, interfaces);
    exit (1);
  }

  ifaces = defs->ifaces;

  /* decide which ucarp interfaces should be brought up */
  if (all) {
    virt_names = get_all_virt_names (prog, ifaces);
  } else {
    virt_names = get_deduped_virt_names (prog, argc - optind, argv + optind);
  }

  if (! virt_names) {
    if (all)
      return 0;
    usage (prog);
  }

  for (i = 0; virt_names[i]; i++) {
    virt_name = virt_names[i];
    virt_iface = NULL;

    if (! (ptr = strstr (virt_name, ":ucarp"))) {
      fprintf (stderr, "%s: ignoring interface %s\n",
        prog, virt_name);
      continue;
    }

    /* retrieve interface definition */
    for (iface = ifaces; ! virt_iface && iface; iface = iface->next) {
      if (strcmp (iface->logical_iface, virt_name) == 0)
        virt_iface = iface;
    }

    if (! virt_iface) {
      fprintf (stderr, "%s: ignoring unknown interface %s\n",
        prog, virt_name);
      continue;
    }

    virt_addr = getifattrbyname (virt_iface, "address");

    if (! virt_addr) {
      fprintf (stderr, "%s: address missing for interface %s\n",
        prog, virt_name);
      continue;
    }

    /* retrieve parent interface definition */
    (void)strncpy (ucarp_name, virt_name, (size_t)(ptr - virt_name));
    ucarp_iface = NULL;

    for (iface = ifaces; ! ucarp_iface && iface; iface = iface->next) {
      if (strcmp (iface->logical_iface, ucarp_name) == 0)
        ucarp_iface = iface;
    }

    if (! ucarp_iface) {
      fprintf (stderr, "%s: unknown ucarp interface %s\n",
        prog, ucarp_name);
      continue;
    }

    /* ucarp daemon is considered not to be running if ucarp interface is
       down. virtual interface is skipped in both cases, since there's nothing
       we can do. */
    if (! read_state (prog, ucarp_name)) {
      fprintf (stderr, "%s: ucarp interface %s not configured\n",
        prog, ucarp_name);
      continue;
    }

    /* check if ucarp daemon is running */
    args[0] = ucarp;
    args[1] = ucarp_name;
    args[2] = virt_addr;
    args[3] = NULL;

    pid = get_proc_by_args (args);
    if (pid == (pid_t)-1) {
      if (errno) {
        fprintf (stderr, "%s: failed to check if ucarp daemon is running: %s\n",
          prog, strerror (errno));
        continue;
      }
      if (strcmp (prog, "vipdown") == 0) {
        fprintf (stderr, "%s: ucarp daemon not running\n", prog);
        continue;
      }
    }

    pid = get_proc_by_args (args);
    if (pid == (pid_t)-1) {
      if (errno) {
        fprintf (stderr, "%s: failed to check if ucarp daemon is running: %s\n",
          prog, strerror (errno));
        continue;
      }
      /* virtual interface considered down if ucarp daemon is not running */
      if (strcmp (prog, "vipdown") == 0) {
        fprintf (stderr, "%s: interface %s not configured\n",
          prog, virt_name);
      /* start ucarp daemon */
      } else {
        (void)start_ucarp_daemon (prog, ucarp_iface, virt_iface);
      }
    } else {
      /* virtual interface considered up if ucarp daemon is running */
      if (strcmp (prog, "vipup") == 0) {
        fprintf (stderr, "%s: interface %s already configured\n",
          prog, virt_name);
      /* stop ucarp daemon */
      } else {
        (void)stop_ucarp_daemon (prog, pid, ucarp_iface, virt_iface);
      }
    }
  }

  free (virt_names);

  return 0;
}

