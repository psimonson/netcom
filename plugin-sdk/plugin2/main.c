/*
 * main.c - Source for a simple hello world plugin.
 *
 * Author: Philip R. Simonson
 * Date  : 08/06/2022
 *
 ****************************************************************************
 */

#include <stdio.h>
#include "network.h"
#include "plugin.h"

char _flag;

PLUGIN_INIT(PMTYPE_NORMAL, NULL, 0);

void init(Plugin *pm, const SOCKET fd)
{
	if(!_flag) {
		plugin_init(pm);
	}
	else {
		send(fd, "Hello world!\r\n", 14, 0);
	}
}
