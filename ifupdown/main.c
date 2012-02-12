
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>

#include "header.h"

#include <unistd.h>
#include <fcntl.h>

#include <getopt.h>

#include <fnmatch.h>

int no_act = 0;
int verbose = 0;
char *statefile = "/etc/network/run/ifstate";
char *tmpstatefile = "/etc/network/run/.ifstate.tmp";


const char *read_state(const char *argv0, const char *iface);
static void read_all_state(const char *argv0, char ***ifaces, int *n_ifaces);
static void update_state(const char *argv0, const char *iface, const char *liface);

static int lock_fd (int fd);




const char *
read_state (const char *argv0, const char *iface)
{
	char *ret = NULL;

	

FILE *state_fp;
char buf[80];
char *p;

state_fp = fopen(statefile, no_act ? "r" : "a+");
if (state_fp == NULL) {
	if (!no_act) {
		fprintf(stderr, 
			"%s: failed to open statefile %s: %s\n",
			argv0, statefile, strerror(errno));
		exit (1);
	} else {
		goto noact;
	}
}

if (!no_act) {
	int flags;

	if ((flags = fcntl(fileno(state_fp), F_GETFD)) < 0
	    || fcntl(fileno(state_fp), F_SETFD, flags | FD_CLOEXEC) < 0) {
		fprintf(stderr, 
			"%s: failed to set FD_CLOEXEC on statefile %s: %s\n",
			argv0, statefile, strerror(errno));
		exit(1);
	}

	if (lock_fd (fileno(state_fp)) < 0) {
		fprintf(stderr, 
			"%s: failed to lock statefile %s: %s\n",
			argv0, statefile, strerror(errno));
		exit(1);
	}
}


	while((p = fgets(buf, sizeof buf, state_fp)) != NULL) {
		

char *pch;

pch = buf + strlen(buf) - 1;
while(pch > buf && isspace(*pch)) pch--;
*(pch+1) = '\0';

pch = buf;
while(isspace(*pch)) pch++;


		if (strncmp(iface, pch, strlen(iface)) == 0) {
			if (pch[strlen(iface)] == '=') {
				ret = pch + strlen(iface) + 1;
				break;
			}
		}
	}

	

noact:
if (state_fp != NULL) {
	fclose(state_fp);
	state_fp = NULL;
}


	return ret;
}

static void
read_all_state (const char *argv0, char ***ifaces, int *n_ifaces)
{
	

FILE *state_fp;
char buf[80];
char *p;

state_fp = fopen(statefile, no_act ? "r" : "a+");
if (state_fp == NULL) {
	if (!no_act) {
		fprintf(stderr, 
			"%s: failed to open statefile %s: %s\n",
			argv0, statefile, strerror(errno));
		exit (1);
	} else {
		goto noact;
	}
}

if (!no_act) {
	int flags;

	if ((flags = fcntl(fileno(state_fp), F_GETFD)) < 0
	    || fcntl(fileno(state_fp), F_SETFD, flags | FD_CLOEXEC) < 0) {
		fprintf(stderr, 
			"%s: failed to set FD_CLOEXEC on statefile %s: %s\n",
			argv0, statefile, strerror(errno));
		exit(1);
	}

	if (lock_fd (fileno(state_fp)) < 0) {
		fprintf(stderr, 
			"%s: failed to lock statefile %s: %s\n",
			argv0, statefile, strerror(errno));
		exit(1);
	}
}


	*n_ifaces = 0;
	*ifaces = NULL;

	while((p = fgets(buf, sizeof buf, state_fp)) != NULL) {
		

char *pch;

pch = buf + strlen(buf) - 1;
while(pch > buf && isspace(*pch)) pch--;
*(pch+1) = '\0';

pch = buf;
while(isspace(*pch)) pch++;


		(*n_ifaces)++;
		*ifaces = realloc (*ifaces, sizeof (**ifaces) * *n_ifaces);
		(*ifaces)[(*n_ifaces)-1] = strdup (pch);
	}

	

noact:
if (state_fp != NULL) {
	fclose(state_fp);
	state_fp = NULL;
}

}

static void update_state(const char *argv0, const char *iface, const char *state)
{
	FILE *tmp_fp;

	

FILE *state_fp;
char buf[80];
char *p;

state_fp = fopen(statefile, no_act ? "r" : "a+");
if (state_fp == NULL) {
	if (!no_act) {
		fprintf(stderr, 
			"%s: failed to open statefile %s: %s\n",
			argv0, statefile, strerror(errno));
		exit (1);
	} else {
		goto noact;
	}
}

if (!no_act) {
	int flags;

	if ((flags = fcntl(fileno(state_fp), F_GETFD)) < 0
	    || fcntl(fileno(state_fp), F_SETFD, flags | FD_CLOEXEC) < 0) {
		fprintf(stderr, 
			"%s: failed to set FD_CLOEXEC on statefile %s: %s\n",
			argv0, statefile, strerror(errno));
		exit(1);
	}

	if (lock_fd (fileno(state_fp)) < 0) {
		fprintf(stderr, 
			"%s: failed to lock statefile %s: %s\n",
			argv0, statefile, strerror(errno));
		exit(1);
	}
}


	if (no_act)
		goto noact;

	tmp_fp = fopen(tmpstatefile, "w");
	if (tmp_fp == NULL) {
		fprintf(stderr, 
			"%s: failed to open temporary statefile %s: %s\n",
			argv0, tmpstatefile, strerror(errno));
		exit (1);
	}

	while((p = fgets(buf, sizeof buf, state_fp)) != NULL) {
		

char *pch;

pch = buf + strlen(buf) - 1;
while(pch > buf && isspace(*pch)) pch--;
*(pch+1) = '\0';

pch = buf;
while(isspace(*pch)) pch++;


		if (strncmp(iface, pch, strlen(iface)) == 0) {
			if (pch[strlen(iface)] == '=') {
				if (state != NULL) {
					fprintf (tmp_fp, "%s=%s\n",
						 iface, state);
					state = NULL;
				}

				continue;
			}
		}

		fprintf (tmp_fp, "%s\n", pch);
	}

	if (state != NULL)
		fprintf (tmp_fp, "%s=%s\n", iface, state);

	fclose (tmp_fp);
	if (rename (tmpstatefile, statefile)) {
		fprintf(stderr, 
			"%s: failed to overwrite statefile %s: %s\n",
			argv0, statefile, strerror(errno));
		exit (1);
	}

	

noact:
if (state_fp != NULL) {
	fclose(state_fp);
	state_fp = NULL;
}

}

static int lock_fd (int fd) {
	struct flock lock;

	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;

	if  (fcntl(fd, F_SETLKW, &lock) < 0) {
		return -1;
	}

	return 0;
}

