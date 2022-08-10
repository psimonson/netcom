/*
 * cmd.c - Source for simple command extension.
 *
 * Author: Philip R. Simonson
 * Date  : 07/29/2022
 *
 ****************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parse.h"
#include "plugin.h"

/* Bool flag set if plugin active. */
char _flag;

/* Forward declarations for command functions. */
CMD_DEF(dummy);

/* Define commands structure. */
static Command cmds[] = {
	CMD_ADD1(dummy, "", "Simple example command.")
};
static int CMD_CNT = sizeof(cmds) / sizeof(cmds[0]);

/* Simple dummy command.
 */
CMD_DEF(dummy)
{
	printf("I'm a dummy.\n");
	return 0;
}

PLUGIN_INIT(PMTYPE_COMMAND, cmds, CMD_CNT);

