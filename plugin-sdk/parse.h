/*
 * parse.h - Header for a simple command line parser.
 *
 * Author: Philip R. Simonson
 * Date  : 07/27/2022
 *
 ****************************************************************************
 */

#ifndef _PARSE_H_
#define _PARSE_H_

#include "cmd.h"

#define DELIM " \r\n"

#define PARSE_INIT(cmds, size) void command_init(void) { \
	parse_init(cmds, size); \
}

/* Parse command line arguments from given argument string. */
Argument *arg_parser(const char *s);

/* Initialize parser for commands. */
extern void parse_init(Command *commands, int total);

/* Parse a given command string. */
extern int parse_input(const SOCKET fd, char *string);

/* Initialize the commands given. */
extern void command_init(void);

#endif
