/*
 * parse.c - Source for a simple command line parser.
 *
 * Author: Philip R. Simonson
 * Date  : 07/27/2022
 *
 ****************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include "parse.h"
#include "plugin.h"

static Command *cmds;
static int CMD_CNT;

/* Initialize the parser for commands.
 */
void parse_init(Command *commands, int total)
{
	cmds = commands;
	CMD_CNT = total;
}

#define ESCAPE \
	free(args); \
	return NULL
/* Parse command line arguments from given argument string.
 */
Argument *arg_parser(const char *s)
{
	Argument *args;
	int i, cnt;

	cnt = strlen(s);
	if(!cnt) return NULL;

	args = (Argument *)malloc(cnt * sizeof(Argument));
	if(!args) return NULL;

	for(i = 0; i < cnt; i++) {
		char *tok = NULL;

		switch(s[i]) {
			case 's':
				tok = strtok(NULL, DELIM);
				if(!tok) {
					ESCAPE;
				}
				args[i].s = tok;
			break;
			case 'd':
				tok = strtok(NULL, DELIM);
				if(!tok) {
					ESCAPE;
				}
				args[i].d = atoi(tok);
			break;
			case 'f':
				tok = strtok(NULL, DELIM);
				if(!tok) {
					ESCAPE;
				}
				args[i].f = atof(tok);
			break;
			default:
				ESCAPE;
			break;
		}
	}
	args[i].s = NULL;
	return args;
}
#undef ESCAPE

/* String parser for this command interpreter.
 */
int parse_input(const SOCKET fd, char *string)
{
	char *tok;
	int i;

	tok = strtok(string, DELIM);
	if(!tok) {
		send(fd, "No command entered!\r\n", 21, 0);
		return 1;
	}

	i = CMD_CNT;
	while(i--) {
		const Command *cmd = &cmds[i];
		
		if(!strcmp(tok, cmd->name)) {
			Argument *args = arg_parser(cmd->args);
			int rc;

			if(args == NULL && strlen(cmd->args)) {
				send(fd, "Bad argument(s).\r\n", 18, 0);
				return 1;
			}

			tok = strtok(NULL, DELIM);
			if(tok != NULL) {
				send(fd, "Bad argument(s).\r\n", 18, 0);
				free(args);
				return 1;
			}

			rc = cmd->func(fd, args);
			free(args);
			return rc;
		}
	}

	/* Process other commands from external plugins. */
	{
		int rc = pm_register_commands(tok, fd);
		if(rc >= 0) return rc;
	}

	send(fd, "Bad command.\r\n", 14, 0);
	return 1;
}

