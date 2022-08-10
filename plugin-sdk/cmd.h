/*
 * cmd.h - Header for command interpreter.
 *
 * Author: Philip R. Simonson
 * Date  : 07/23/2022
 *
 ****************************************************************************
 */

#ifndef _CMD_H_
#define _CMD_H_

#include "network.h"

#define CMD_DEF(X) static int cmd_ ##X (const SOCKET fd, const Argument *args)
#define CMD_ADD1(X,A,M) { #X, A, M, cmd_ ##X }
#define CMD_ADD2(X,A,M,F) { #X, A, M, cmd_ ##F }

/* Argument definition and typedef. */
union Argument {
	char *s;
	float f;
	int d;
};
typedef union Argument Argument;

/* Command structure definition and typedef. */
struct Command {
	const char *name;
	const char *args;
	const char *help;
	int (*func)(const SOCKET fd, const Argument *args);
};
typedef struct Command Command;

#endif
