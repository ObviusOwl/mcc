#ifndef ARGS_H
#define ARGS_H

#include <argp.h>

extern struct argp argp;

struct arguments{
	char *socket_file;
	char *basedir;
	char *command;
};

error_t parse_opt (int key, char *arg, struct argp_state *state);

#endif // ARGS_H
