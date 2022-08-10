/*
 * main.c - Source for a simple command plugin.
 *
 * Author: Philip R. Simonson
 * Date  : 07/29/2022
 *
 ****************************************************************************
 */

#include <stdio.h>
#include "network.h"
#include "plugin.h"

void init(Plugin *pm, const SOCKET fd)
{
	extern char _flag;

	if(!_flag) {
		/* Initialize command module. */
		plugin_init(pm);
	}
}
