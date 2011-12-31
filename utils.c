/* system includes */
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/* vipupdown includes */
#include "utils.h"

cmd_t *
cmdfargs (const char *path, const char *arg0, ...)
{
  char **argv, *arg;
  cmd_t *cmd;
  int argc, argno;
  int nerrno;
  va_list ap;

  assert (path);

  va_start (ap, arg0);
  for (arg = (char *)arg0, argc = 0;
       arg;
       arg = (char *)va_arg (ap, const char *))
  {
    argc++;
  }
  va_end (ap);

  if (! (argv = calloc ((argc + 2), sizeof (char *))))
    return NULL; /* errno set by calloc */

  va_start (ap, arg0);
  if (! (argv[argno] = strdup (path))) {
    nerrno = errno;
    goto error;
  }
  for (arg = (char *)arg0, argno++;
       arg;
       arg = (char *)va_arg (ap, const char *), argno++)
  {
    if (! (argv[argno] = strdup (arg))) {
      nerrno = errno;
      goto error;
    }
  }
  va_end (ap);

  if (! (cmd = calloc (1, sizeof (cmd_t)))) {
    nerrno = errno;
    goto error;
  }

  cmd->argv = argv;
  cmd->argc = argc;
  return cmd;
error:
  for (--argc; argc >= 0; argc--) {
    if (argv[argc])
      free (argv[argc]);
  }
  errno = nerrno;
  return NULL;
}

void
cmdfree (cmd_t *cmd)
{
  int argc;

  if (cmd) {
    for (argc = cmd->argc; argc >=0; argc--) {
      if ((cmd->argv)[argc])
        free ((cmd->argv)[argc]);
    }
    free (cmd);
  }
}

cmd_t *
read_cmdline (const char *path)
{
#define BUFLEN (1024)
  char **argv = NULL;
  char *buf = NULL;
  char *boa, *nbuf, *ptr;
  cmd_t *cmd;
  int argc, argno, nerrno;
  int eoa = 0;
  int fd = -1;
  size_t cnt = 0;
  size_t len = 0;
  size_t nlen;
  ssize_t nrd;

  if ((fd = open (path, O_RDONLY)) < 0)
    return NULL;

  nerrno = errno;
  errno = 0;

  for (;;) {
    if (cnt >= len) {
      nlen = len + BUFLEN;
      if (! (nbuf = realloc (buf, nlen + 1))) {
        nerrno = errno;
        goto error;
      }
      buf = nbuf;
      len = nlen;
    }

    nrd = read (fd, (buf + cnt), (len - cnt));
    if (nrd == 0)
      break;
    if (nrd >  0)
      cnt += nrd;
    if (nrd < (len - cnt)) {
      if (errno) {
        if (errno != EINTR) {
          nerrno = errno;
          goto error;
        }
        errno = 0;
      }
    }
  }

  errno = nerrno;

  (void)close (fd); fd = -1;

  /* count number or arguments */
  for (argc = 0, eoa = 1, ptr = buf; ; ptr++) {
    if (eoa) {
      if (*ptr == '\0')
        break;
      else
        eoa = 0;
    } else {
      if (*ptr == '\0') {
        eoa = 1;
        argc++;
      }
    }
  }

  if (! (argv = calloc (argc + 1, sizeof (char *)))) {
    nerrno = errno;
    goto error;
  }

  for (argno = 0, eoa = 1, ptr = buf; argno < argc ; ptr++) {
    if (eoa) {
      if (*ptr == '\0') {
        break;
      } else {
        boa = ptr; /* begin of argument */
        eoa = 0;
      }
    } else {
      if (*ptr == '\0') {
        if (! (argv[argno++] = strndup (boa, (size_t)(ptr - boa)))) {
          nerrno = errno;
          goto error;
        }
        eoa = 1;
      }
    }
  }

  free (buf); buf = NULL;

  if (! (cmd = calloc (1, sizeof (cmd_t)))) {
    nerrno = errno;
    goto error;
  }

  cmd->argv = argv;
  cmd->argc = argc;
  return cmd;
error:
  if (argv) {
    for (; argc >= 0; argc--) {
      if (argv[argc])
        free (argv[argc]);
    }
  }
  if (buf)
    free (buf);
  if (fd >= 0)
    (void)close (fd);
  errno = nerrno;
  return NULL;
#undef BUFLEN
}

pid_t
getpidbycmd (cmd_t *cmd)
{
  const char *fmt = "/proc/%d/cmdline";
  char path[PATH_MAX];
  cmd_t *procmd = NULL;
  DIR *dir = NULL;
  int nargs, i, j, ret, nerrno;
  pid_t pid = 0;
  pid_t tmpid;
  struct dirent *entry;

  if (! (dir = opendir ("/proc")))
    return (pid_t)-1;

  nerrno = errno;
  errno = 0;
next:
  while ((entry = readdir (dir)) && ! pid) {
    if (! entry) {
      if (errno) {
        nerrno = errno;
        goto error;
      } else {
        /* no more entries */
        break;
      }
    } else {
      if ((tmpid = strtol (entry->d_name, NULL, 10)) > 0) {
        /* create path argument to be passed to read_cmdline */
        if ((ret = snprintf (path, PATH_MAX, fmt, tmpid)) < 0 || ret > PATH_MAX) {
          nerrno = errno;
          goto error;
        }
        if (! (procmd = read_cmdline (path))) {
          switch (errno) {
            case EACCES:
            case ENOENT:
            case ENOTDIR:
              errno = 0;
              goto next;
            default:
              nerrno = errno;
              goto error;
          }
        }

        /* compare cmd argument to cmd or running process */
        if (procmd->argc < cmd->argc)
          goto next;
        if (strcmp (cmd->argv[0], procmd->argv[0]) != 0)
          goto next;

        for (i = 1, nargs = 1; i < cmd->argc && i == nargs; i++) {
          for (j = 1; j < procmd->argc; j++) {
            if (strcmp (cmd->argv[i], procmd->argv[j]) == 0) {
              nargs++;
              break;
            }
          }
        }

        cmdfree (procmd); procmd = NULL;

        if (nargs == cmd->argc)
          pid = tmpid;
      } else {
        /* errno set to EINVAL by strtol */
        errno = 0;
      }
    }
  }
  errno = nerrno;

  for (; (ret = closedir (dir)); ) {
    if (errno != EINTR) {
      nerrno = errno;
      goto error;
    }
  }
  dir = NULL;
  return (pid) ? pid : (pid_t)-1;
error:
  if (procmd)
    cmdfree (procmd);
  if (dir)
    (void)closedir (dir);
  errno = nerrno;
  return (pid_t)-1;
}

