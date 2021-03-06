
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "header.h"

#include <errno.h>

#include <stdarg.h>

#include <unistd.h>
#include <sys/wait.h>

static char **environ = NULL;

static int check(char *str);

static void set_environ(interface_defn *iface, char *mode, char *phase);

 char *setlocalenv(char *format, char *name, char *value);

 int doit(char *str);

static char *parse(char *command, interface_defn *ifd);

void addstr(char **buf, size_t *len, size_t *pos, char *str, size_t strlen);

int strncmpz(char *l, char *r, size_t llen);

char *get_var(char *id, size_t idlen, interface_defn *ifd);

static int popen2(FILE **in, FILE **out, char *command, ...);

static int check(char *str) {
	return str != NULL;
}

static void set_environ(interface_defn *iface, char *mode, char *phase) {
	

char **environend;

	int i;
	const int n_env_entries = iface->n_options + 8;

	

if (environ != NULL) {
	char **ppch;
	for (ppch = environ; *ppch; ppch++) {
		free(*ppch);
		*ppch = NULL;
	}
	free(environ);
	environ = NULL;
}

environ = malloc(sizeof(char*) * (n_env_entries + 1 /* for final NULL */));
environend = environ; 
*environend = NULL;


	for (i = 0; i < iface->n_options; i++) {
		

if (strcmp(iface->option[i].name, "pre-up") == 0
    || strcmp(iface->option[i].name, "up") == 0
    || strcmp(iface->option[i].name, "down") == 0
    || strcmp(iface->option[i].name, "post-down") == 0)
{
	continue;
}


		

*(environend++) = setlocalenv("IF_%s=%s", iface->option[i].name,
                              iface->option[i].value);
*environend = NULL;

	}

	

*(environend++) = setlocalenv("%s=%s", "IFACE", iface->real_iface);
*environend = NULL;

	

*(environend++) = setlocalenv("%s=%s", "LOGICAL", iface->logical_iface);
*environend = NULL;

	

*(environend++) = setlocalenv("%s=%s", "ADDRFAM", iface->address_family->name);
*environend = NULL;

	

*(environend++) = setlocalenv("%s=%s", "METHOD", iface->method->name);
*environend = NULL;


	

*(environend++) = setlocalenv("%s=%s", "MODE", mode);
*environend = NULL;

	

*(environend++) = setlocalenv("%s=%s", "PHASE", phase); 
*environend = NULL;

	

*(environend++) = setlocalenv("%s=%s", "VERBOSITY", verbose ? "1" : "0");
*environend = NULL;

	

*(environend++) = setlocalenv("%s=%s", "PATH", "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin");
*environend = NULL;

}

 char *setlocalenv(char *format, char *name, char *value) {
	char *result;

	

result = malloc(strlen(format)   /* -4 for the two %s's */
                + strlen(name) 
                + strlen(value) 
                + 1);
if (!result) {
	perror("malloc");
	exit(1);
}


	sprintf(result, format, name, value);

	

{
	char *here, *there;

	for(here = there = result; *there != '=' && *there; there++) {
		if (*there == '-') *there = '_';
		if (isalpha(*there)) *there = toupper(*there);

		if (isalnum(*there) || *there == '_') {
			*here = *there;
			here++;
		}
	}
	memmove(here, there, strlen(there) + 1);
}


	return result;
}

 int doit(char *str) {
	assert(str);

	if (verbose || no_act) {
		fprintf(stderr, "%s\n", str);
	}
	if (!no_act) {
		pid_t child;
		int status;

		fflush(NULL);
		switch(child = fork()) {
		    case -1: /* failure */
			return 0;
		    case 0: /* child */
			execle("/bin/sh", "/bin/sh", "-c", str, NULL, environ);
			exit(127);
		    default: /* parent */
		    	break;
		}
		waitpid(child, &status, 0);
		if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
			return 0;
	}
	return 1;
}





static char *parse(char *command, interface_defn *ifd) {
	

char *result = NULL;
size_t pos = 0, len = 0;

size_t old_pos[MAX_OPT_DEPTH] = {0};
int okay[MAX_OPT_DEPTH] = {1};
int opt_depth = 1;


	while(*command) {
		switch(*command) {
			

default:
	addstr(&result, &len, &pos, command, 1);
	command++;
	break;

case '\\':
	if (command[1]) {
		addstr(&result, &len, &pos, command+1, 1);
		command += 2;
	} else {
		addstr(&result, &len, &pos, command, 1);
		command++;
	}
	break;

case '[':
	if (command[1] == '[' && opt_depth < MAX_OPT_DEPTH) {
		old_pos[opt_depth] = pos;
		okay[opt_depth] = 1;
		opt_depth++;
		command += 2;
	} else {
		addstr(&result, &len, &pos, "[", 1);
		command++;
	}
	break;

case ']':
	if (command[1] == ']' && opt_depth > 1) {
		opt_depth--;
		if (!okay[opt_depth]) {
			pos = old_pos[opt_depth];
			result[pos] = '\0';
		}
		command += 2;
	} else {
		addstr(&result, &len, &pos, "]", 1);
		command++;
	}
	break;

case '%':
{
	

char *nextpercent;

	char *varvalue;

	

command++;
nextpercent = strchr(command, '%');
if (!nextpercent) {
	errno = EUNBALPER;
	free(result);
	return NULL;
}


	

varvalue = get_var(command, nextpercent - command, ifd);


	if (varvalue) {
		addstr(&result, &len, &pos, varvalue, strlen(varvalue));
	} else {
		okay[opt_depth - 1] = 0;
	}

	

command = nextpercent + 1;


	break;
}

		}
	}

	

if (opt_depth > 1) {
	errno = EUNBALBRACK;
	free(result);
	return NULL;
}

if (!okay[0]) {
	errno = EUNDEFVAR;
	free(result);
	return NULL;
}


	

return result;

}

void addstr(char **buf, size_t *len, size_t *pos, char *str, size_t strlen) {
	assert(*len >= *pos);
	assert(*len == 0 || (*buf)[*pos] == '\0');

	if (*pos + strlen >= *len) {
		char *newbuf;
		newbuf = realloc(*buf, *len * 2 + strlen + 1);
		if (!newbuf) {
			perror("realloc");
			exit(1); /* a little ugly */
		}
		*buf = newbuf;
		*len = *len * 2 + strlen + 1;
	}

	while (strlen-- >= 1) {
		(*buf)[(*pos)++] = *str;
		str++;
	}
	(*buf)[*pos] = '\0';
}

int strncmpz(char *l, char *r, size_t llen) {
	int i = strncmp(l, r, llen);
	if (i == 0)
		return -r[llen];
	else
		return i;
}

char *get_var(char *id, size_t idlen, interface_defn *ifd) {
	int i;

	if (strncmpz(id, "iface", idlen) == 0) {
		return ifd->real_iface;
	} else {
		for (i = 0; i < ifd->n_options; i++) {
			if (strncmpz(id, ifd->option[i].name, idlen) == 0) {
				return ifd->option[i].value;
			}
		}
	}

	return NULL;
}

int run_mapping(char *physical, char *logical, int len, mapping_defn *map) {
	FILE *in, *out;
	int i, status;
	pid_t pid;

	

pid = popen2(&in, &out, map->script, physical, NULL);
if (pid == 0) {
	return 0;
}

	

for (i = 0; i < map->n_mappings; i++) {
	fprintf(in, "%s\n", map->mapping[i]);
}
fclose(in);

	

waitpid(pid, &status, 0);

	

if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
	if (fgets(logical, len, out)) {
		char *pch = logical + strlen(logical) - 1;
		while (pch >= logical && isspace(*pch)) 
			*(pch--) = '\0';
	}
}
fclose(out);	


	return 1;
}

static int popen2(FILE **in, FILE **out, char *command, ...) {
	va_list ap;
	char *argv[11] = {command};
	int argc;
	int infd[2], outfd[2];
	pid_t pid;

	argc = 1;
	va_start(ap, command);
	while((argc < 10) && (argv[argc] = va_arg(ap, char*))) {
		argc++;
	}
	argv[argc] = NULL; /* make sure */
	va_end(ap);

	if (pipe(infd) != 0) return 0;
	if (pipe(outfd) != 0) {
		close(infd[0]); close(infd[1]);
		return 0;
	}

	fflush(NULL);
	switch(pid = fork()) {
		case -1: /* failure */
			close(infd[0]); close(infd[1]);
			close(outfd[0]); close(outfd[1]);
			return 0;
		case 0: /* child */
			dup2(infd[0], 0);
			dup2(outfd[1], 1);
			close(infd[0]); close(infd[1]);
			close(outfd[0]); close(outfd[1]);
			execvp(command, argv);
			exit(127);
		default: /* parent */
			*in = fdopen(infd[1], "w");
			*out = fdopen(outfd[0], "r");
			close(infd[0]);	close(outfd[1]);
			return pid;
	}
	/* unreached */
}
