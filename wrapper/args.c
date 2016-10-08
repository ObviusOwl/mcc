#include "args.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>

const char *argp_program_version = "minecraft-wrapper 1.0";
const char *argp_program_bug_address = "<jojo@terhaak.de>";

static char prog_doc[] = "Wrapper for minecraft with stdio redirect to a socket";
static char args_doc[] = "[bs] startscript";

static struct argp_option options[] = {
	{ "basedir",	'b',	"path",	0,	"change to directory dir before running", 0},
	{ "socket",		's',	"path",	0,	"path to socket for minecraft stdio", 0},
	{ 0 }
};

error_t parse_opt (int key, char *arg, struct argp_state *state) {
	struct arguments *arguments = state->input;
	int ret;
	double val;
	switch (key){
		case 'b':
			arguments->basedir = arg;
			break;
		case 's':
			arguments->socket_file = arg;
			break;
		case ARGP_KEY_ARG:
			if( arg == NULL ){
				break;
			}
			arguments->command = arg;
			break;
		case ARGP_KEY_END:
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}


struct argp argp = { options, parse_opt, args_doc, prog_doc, NULL, NULL, NULL };
