/* system includes */
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* vipupdown includes */
#include "utils.h"

/* prototypes */
char *ftos (const char *);
char **parse_cmdline (const char *);

char *
ftos (const char *path)
{
#define BUFLEN (2048)
  char *buf, *new_buf;
  int fd, ret;
  int orig_errno;
  size_t cnt, len, new_len;
  ssize_t got;

  for (; (fd = open (path, O_RDONLY)) < 0 && errno == EINTR; )
    ;
  if (fd < 0)
    return NULL;

  buf = NULL;
  cnt = 0;
  len = 0;

  orig_errno = errno;
  errno = 0;

  for (;;) {
    if (cnt >= len) {
      if (! (new_buf = realloc (buf, (len + BUFLEN) + 1)))
        goto error;
      buf = new_buf;
      len = new_len;
    }

    got = read (fd, buf, (len - cnt));

    if (got > 0) {
      cnt += got;
    }
    if (got < (len - cnt)) {
      if (errno) {
        if (errno != EINTR)
          goto error;
        errno = 0;
      } else {
        break;
      }
    }
  }

  (void)close (fd);
  errno = orig_errno;
  return buf;
error:
  if (buf)
    free (buf);
  return NULL;
#undef BUFLEN
}

char **
parse_cmdline (const char *path)
{
  char **argv;
  char *boa, *ptr, *str;
  int argno, argc;
  int delim;
  int ret, new_errno;

  if (! (str = ftos (path)))
    return NULL;

  /* count number or arguments */
  for (argc = 0, delim = 1, ptr = str; ; ptr++) {
    if (delim) {
      if (*ptr == '\0') {
        break;
      } else {
        delim = 0;
      }
    } else {
      if (*ptr == '\0') {
        argc++;
        delim = 1;
      }
    }
  }

  if (! (argv = calloc (argc + 1, sizeof (char *)))) {
    new_errno = errno;
    goto error;
  }

  for (argno = 0, delim = 1, ptr = str; argno < argc ; ptr++) {
    if (delim) {
      if (*ptr == '\0') {
        break;
      } else {
        boa = ptr; /* begin of argument */
        delim = 0;
      }
    } else {
      if (*ptr == '\0') {
        if (! (argv[argno++] = strndup (boa, (size_t)(ptr - boa)))) {
          new_errno = errno;
          goto error;
        }
        delim = 1;
      }
    }
  }

  free (str); str = NULL;

  return argv;
error:
  if (argv) {
    for (argno = 0; argno < argc; argno++) {
      if (argv[argno])
        free (argv[argno]);
    }
    free (argv);
  }
  if (str) {
    free (str);
  }
  errno = new_errno;
  return NULL;
}

pid_t
get_proc_by_args (char **args)
{
  char **proc_args;
  char path[PATH_MAX];
  DIR *dir = NULL;
  int i, j, n, found, ret;
  int new_errno, orig_errno;
  pid_t pid, tmppid;
  struct dirent *entry;

  if (! (dir = opendir ("/proc")))
    return (pid_t)-1;

  pid = (pid_t)-1;
  orig_errno = errno;
  errno = 0;
next:
  while ((entry = readdir (dir)) && pid == (pid_t)-1) {
    if (! entry) {
      if (errno) {
        new_errno = errno;
        goto error;
      } else {
        /* no more entries */
        break;
      }
    } else {
      if ((tmppid = strtol (entry->d_name, NULL, 10)) > 0) {
        /* compile path that will be passed to read_cmdline */
        ret = snprintf (path, PATH_MAX, "/proc/%u/cmdline", tmppid);
        if (ret < 0 || ret > PATH_MAX) {
          new_errno = errno;
          goto error;
        }

        if (! (proc_args = parse_cmdline (path))) {
          switch (errno) {
            case EACCES:
            case ENOENT:
            case ENOTDIR:
              errno = 0;
              goto next;
            default:
              new_errno = errno;
              goto error;
          }
        }

        /* compare given arguments to arguments of running process */
        for (i = 0, found = 1; args[i] && found; i++) {
          found = 0;
          for (j = 0; proc_args[j] && ! found; j++) {
            if (strcmp (args[i], proc_args[j]) == 0) {
              found = 1;
              break;
            }
          }
        }

        for (i = 0; proc_args[i]; i++) {
          free (proc_args[i]);
        }
        free (proc_args);

        if (found)
          pid = tmppid;
      } else {
        /* errno set to EINVAL by strtol */
        errno = 0;
      }
    }
  }

  errno = orig_errno;

  for (; (ret = closedir (dir)); ) {
    if (errno == EINTR) {
      errno = orig_errno;
    } else {
      new_errno = errno;
      goto error;
    }
  }

  return pid;
error:
  if (dir)
    (void)closedir (dir);
  errno = new_errno;
  return (pid_t)-1;
}

/*  based on the code from http://stackoverflow.com/questions/5828753/start-time-of-a-process-on-linux */
time_t
get_proc_starttime (pid_t pid)
{
  char *fmt;
  char path[PATH_MAX];
  char *begin, *ptr, *str;
  int new_errno, orig_errno;
  int fld, ret, spc;
  long jiffies_per_second;
  long long starttime_in_jiffies;
  time_t boot_time, starttime;

  jiffies_per_second = sysconf (_SC_CLK_TCK);

  /* get time in jiffies the process started after boot from /proc/PID/stat */
  fmt = "/proc/%u/stat";
  ret = snprintf (path, PATH_MAX, fmt, pid);
  if (ret < 0 || ret > PATH_MAX) {
    new_errno = EINVAL;
    goto error;
  }

  if (! (str = ftos (path))) {
    switch (errno) {
      case EACCES:
      case EISDIR:
      case ELOOP:
      case ENAMETOOLONG:
      case ENOENT:
      case ENOTDIR:
        new_errno = EINVAL;
        break;
      default:
        new_errno = errno;
        break;
    }
    goto error;
  }

  /* second field is filename of executable in parentheses, other fields (at
     least up until starttime field) cannot contain white space */
  if (! (ptr = strrchr (str, ')'))) {
    new_errno = EINVAL;
    goto error;
  }

  for (++ptr, fld = 2, spc = 1; *ptr; ptr++) {
    if (spc) {
      if (! isspace (*ptr)) {
        if (++fld == 22) {
          orig_errno = errno;
          errno = 0;
          if (! (starttime_in_jiffies = strtoll (ptr, NULL, 10)) && errno) {
            new_errno = errno;
            goto error;
          }
          errno = orig_errno;
          break;
        }
        spc = 0;
      }
    } else {
      if (isspace (*ptr)) {
        spc = 1;
      }
    }
  }

  free (str); str = NULL;

  /* get boot time in seconds since the Epoch from /proc/stat */
  if (! (str = ftos ("/proc/stat"))) {
    goto error;
  }
  if (! (ptr = strstr (str, "btime "))) {
    goto error;
  }

  orig_errno = errno;
  errno = 0;
  if (! (boot_time = strtoll ((ptr + 6), NULL, 10)) && errno) {
    new_errno = errno;
    goto error;
  }
  errno = orig_errno;

  free (str); str = NULL;

  starttime = boot_time + starttime_in_jiffies / jiffies_per_second;
  return starttime;
error:
  if (str)
    free (str);
  errno = new_errno;
  return (time_t)-1;
}

interface_defn *
getifbyattr (interface_defn *ifaces, const char *option, const char *value)
{
  interface_defn *iface;
  int i;

  assert (ifaces);
  assert (option);
  assert (value);

  for (iface = ifaces; iface; iface = iface->next) {
    if (strcmp (option, "logical_iface") == 0 &&
        strcmp (iface->logical_iface, value) == 0)
      return iface;

    for (i = 0; i < iface->n_options; i++) {
      if (strcmp ((iface->option + i)->name, option) == 0 &&
          strcmp ((iface->option + i)->value, value) == 0)
        return iface;
    }
  }

  return NULL;
}

char *
getifattrbyname (interface_defn *iface, const char *option)
{
  unsigned int i;

  assert (iface);
  assert (option);

  for (i = 0; i < iface->n_options; i++) {
    if (strcmp ((iface->option + i)->name, option) == 0)
      return (iface->option + i)->value;
  }

  return NULL;
}

