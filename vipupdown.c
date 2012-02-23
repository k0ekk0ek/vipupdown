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
void help (const char *);
void usage (const char *);
void version (const char *, const char *);
char **get_all_virt_names (const char *, interface_defn *);
char **get_deduped_virt_names (const char *, int, char **);

#define VERSION "0.1"
#define KILL_RETRY (5)

char ucarp[] = "/usr/sbin/ucarp";
char ucarp_up[] = "/etc/network/if-up.d/ucarp";

void
help (const char *prog)
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
main (int argc, char *argv[])
{
  char *prog;
  char *args[4];
  char *opts = "i:hVanv";
  char *interfaces = NULL;
  char *virt_addr;
  char *virt_name, **virt_names;
  char ucarp_name[80];
  char *ptr, *tail, *delim;
  int all, export;
  int c, len, ret, res;
  int envno, ifno, optno;
  int orig_errno;
  interfaces_file *iface_file;
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

  if ((prog = strrchr (argv[0], '/')))
    prog++;
  else
    prog = argv[0];

  if (strcmp (prog, "vipup") != 0 && strcmp (prog, "vipdown") != 0) {
    fprintf (stderr, "%s: invoke as vipup or vipdown\n", prog);
    exit (1);
  }

  no_act = 0;
  verbose = 0;

  /* parse command line options */
  all = 0;
  for (; (c = getopt_long (argc, argv, opts, long_opts, NULL)) != EOF ;) {
    switch (c) {
      case 'h':
        help (prog);
        break; /* never reached */
      case 'V':
        version (prog, VERSION);
        break; /* never reached */
      case 'a':
        all = 1;
        break;
      case 'i':
        interfaces = strdup (optarg);
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
  if (! all && optind == argc)
    usage (prog);

  if (! interfaces)
    interfaces = strdup (INTERFACES);

  /* parse interfaces file */
  if (! (iface_file = read_interfaces (interfaces))) {
    fprintf (stderr, "%s: cannot read interfaces file \"%s\"\n",
      prog, interfaces);
    exit (1);
  }

  ifaces = iface_file->ifaces;

  /* decide which ucarp interfaces should be brought up */
  if (all)
    virt_names = get_all_virt_names (prog, ifaces);
  else
    virt_names = get_deduped_virt_names (prog, argc - optind, argv + optind);

  if (! virt_names) {
    if (all)
      return 0;
    usage (prog);
  }

  for (ifno = 0; virt_names[ifno]; ifno++) {
    virt_addr = NULL;
    virt_name = virt_names[ifno];
    virt_iface = NULL;
    ucarp_iface = NULL;

    if (! (delim = strstr (virt_name, ":ucarp"))) {
      fprintf (stderr, "%s: ignoring interface %s\n", prog, virt_name);
      continue;
    }

    if ((tail = strrchr (virt_name, ':'))) {
      if (! isdigit (*(++tail)))
        tail = NULL;
    }

    /* retrieve interface definition */
    for (iface = ifaces; ! virt_iface && iface; iface = iface->next) {
      if (strcmp (iface->logical_iface, virt_name) == 0)
        virt_iface = iface;
    }

    if (! virt_iface) {
      fprintf (stderr, "%s: unkown interface %s\n", prog, virt_name);
      continue;
    }

    for (optno = 0; optno < virt_iface->n_options; optno++) {
      if (strcmp (virt_iface->option[optno].name, "address") == 0)
        virt_addr = virt_iface->option[optno].value;
    }

    if (! virt_addr) {
      fprintf (stderr, "%s: unkown interface %s\n", prog, virt_name);
      continue;
    }

    /* retrieve ucarp interface definition */
    memset (ucarp_name, '\0', 80);
    (void)strncpy (ucarp_name, virt_name, (size_t)(delim - virt_name));

    for (iface = ifaces; ! ucarp_iface && iface; iface = iface->next) {
      if (strcmp (iface->logical_iface, ucarp_name) == 0)
        ucarp_iface = iface;
    }

    if (! ucarp_iface) {
      fprintf (stderr, "%s: unknown ucarp interface %s\n", prog, ucarp_name);
      continue;
    }

    /* verify the interface is known by the same name under the ucarp
       interface */
    for (optno = 0; optno < ucarp_iface->n_options; optno++) {
      ptr = ucarp_iface->option[optno].name;
      if (tail) {
        if (strncmp (ptr, "ucarp-vip:", 10) == 0 && strcmp (ptr+10, tail) == 0)
          break;
      } else {
        if (strcmp (ptr, "ucarp-vip") == 0)
          break;
      }
    }

    if (optno >= ucarp_iface->n_options) {
      fprintf (stderr, "%s: interface %s not configured in ucarp interface\n", prog, virt_name);
      continue;
    }
    if (strcmp (ucarp_iface->option[optno].value, virt_addr) != 0) {
      fprintf (stderr, "%s: interface %s configured with different ip address in ucarp interface\n", prog, virt_name);
      continue;
    }

    /* ucarp daemon is considered not to be running if ucarp interface is
       down. virtual interface is skipped in both cases, since there's nothing
       we can do. */
    if (! read_state (prog, ucarp_name)) {
      fprintf (stderr, "%s: ucarp interface %s not configured\n", prog, ucarp_name);
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
        fprintf (stderr, "%s: get_proc_by_args: %s\n", prog, strerror (errno));
        goto error;
      }
      /* virtual interface considered down if ucarp daemon is not running */
      if (strcmp (prog, "vipdown") == 0) {
        fprintf (stderr, "%s: ucarp not started for interface %s\n",
          prog, virt_name);
      } else if (no_act) {
        fprintf (stderr, "%s: would start ucarp for interface %s\n",
          prog, virt_name);
      } else {
        /* export environment variables */
        if (! (environ = calloc (ucarp_iface->n_options + 9, sizeof (char *)))) {
          fprintf (stderr, "%s: calloc: %s", prog, strerror (errno));
          goto error;
        }

        for (optno = 0, envno = 0; optno < ucarp_iface->n_options; optno++) {
          if (strcmp (ucarp_iface->option[optno].name, "pre-up")    == 0 ||
              strcmp (ucarp_iface->option[optno].name, "up")        == 0 ||
              strcmp (ucarp_iface->option[optno].name, "down")      == 0 ||
              strcmp (ucarp_iface->option[optno].name, "post-down") == 0)
            continue;

          export = 0;
          if (strncmp (ucarp_iface->option[optno].name, "ucarp-", 6) == 0) {
            ptr = strchr (ucarp_iface->option[optno].name, ':');
            if ((!ptr && !tail) ||
                ( ptr &&  tail && strcmp (++ptr, tail) == 0))
              export = 1;
          } else {
            export = 1;
          }

          if (export) {
            environ[envno++] = setlocalenv ("IF_%s=%s",
                                            ucarp_iface->option[optno].name, 
                                            ucarp_iface->option[optno].value);
          }
        }

        /* export generic environment variables */
        environ[envno++] = setlocalenv("%s=%s", "IFACE",     ucarp_iface->logical_iface);
        environ[envno++] = setlocalenv("%s=%s", "LOGICAL",   ucarp_iface->logical_iface);
        environ[envno++] = setlocalenv("%s=%s", "ADDRFAM",   ucarp_iface->address_family->name);
        environ[envno++] = setlocalenv("%s=%s", "METHOD",    ucarp_iface->method->name);
        environ[envno++] = setlocalenv("%s=%s", "MODE",      "start");
        environ[envno++] = setlocalenv("%s=%s", "PHASE",     "post-up");
        environ[envno++] = setlocalenv("%s=%s", "VERBOSITY", verbose ? "1" : "0");
        environ[envno++] = setlocalenv("%s=%s", "PATH",      "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin");
        environ[envno]   = NULL;

        /* execute /etc/network/if-up.d/ucarp */
        if (! doit (ucarp_up)) {
          fprintf (stderr, "%s: ucarp not started for interface %s\n",
            prog, virt_iface->logical_iface);
        }

        for (envno = 0; environ[envno]; envno++) {
          free (environ[envno]);
          environ[envno] = NULL;
        }
        free (environ);
        environ = NULL;
      }
    } else {
      /* virtual interface considered up if ucarp daemon is running */
      if (strcmp (prog, "vipup") == 0) {
        fprintf (stderr, "%s: ucarp already started for interface %s\n", prog, virt_name);
      } else if (no_act) {
        fprintf (stderr, "%s: would start ucarp for interface %s\n", prog, virt_name);
      } else {
        ret = kill_proc (pid, SIGTERM, KILL_RETRY);
        if (ret < 0)
          fprintf (stderr, "%s: kill_proc: %s\n", prog, strerror (errno));
        else if (ret > KILL_RETRY)
          fprintf (stderr, "%s: ucarp not stopped for interface %s\n", prog, virt_name);
      }
    }
  }

  res = 0;
cleanup:

  if (virt_names) {
    for (ifno = 0; virt_names[ifno]; ifno++)
      free (virt_names[ifno]);
    free (virt_names);
  }

  if (environ) {
    for (envno = 0; environ[envno]; envno++)
      free (environ[envno]);
    free (environ);
  }

  free_interfaces (iface_file);
  free (interfaces);

  return res;
error:
  res = 1;
  goto cleanup;
}

