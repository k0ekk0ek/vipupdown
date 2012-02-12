
#ifndef HEADER_H
#define HEADER_H


typedef struct address_family address_family;

typedef struct method method;

typedef struct interfaces_file interfaces_file;

typedef struct allowup_defn allowup_defn;

typedef struct interface_defn interface_defn;

typedef struct variable variable;

typedef struct mapping_defn mapping_defn;

typedef int (execfn)(char *command);
typedef int (command_set)(interface_defn *ifd, execfn *e);

struct address_family {
	char *name;
	int n_methods;
	method *method;
};

struct method {
	char *name;
	command_set *up, *down;
};

struct interfaces_file {
	allowup_defn *allowups;
	interface_defn *ifaces;
	mapping_defn *mappings;
};

struct allowup_defn {
	allowup_defn *next;

	char *when;
	int max_interfaces;
	int n_interfaces;
	char **interfaces;
};

struct interface_defn {
	interface_defn *next;

	char *logical_iface;
	char *real_iface;

	address_family *address_family;
	method *method;

	int automatic;

	int max_options;
	int n_options;
	variable *option;
};

struct variable {
	char *name;
	char *value;
};

struct mapping_defn {
	mapping_defn *next;

	int max_matches;
	int n_matches;
	char **match;

	char *script;

	int max_mappings;
	int n_mappings;
	char **mapping;
};

#define MAX_OPT_DEPTH 10

#define EUNBALBRACK 10001
#define EUNDEFVAR   10002

#define MAX_VARNAME    32
#define EUNBALPER   10000

extern address_family *addr_fams[];

interfaces_file *read_interfaces(char *filename);

allowup_defn *find_allowup(interfaces_file *defn, char *name);




int run_mapping(char *physical, char *logical, int len, mapping_defn *map);

extern int no_act;
extern int verbose;


#endif /* HEADER_H */
