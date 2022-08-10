/*
 * plugin.h - Header for a simple plugin manager.
 *
 * Author: Philip R. Simonson
 * Date  : 07/27/2022
 *
 ****************************************************************************
 */

#ifndef _PLUGIN_H_
#define _PLUGIN_H_

#include "network.h"
#include "abuffer.h"
#include "clist.h"
#include "cmd.h"

#define PLUGIN_INIT(A, B, C) void plugin_init(Plugin *pm) { \
	pm_set(pm, B, C); \
	pm_settype(pm, A); \
	_flag = 1; \
}

/* Blank enumeration for plugin types. */
enum { PMTYPE_UNKNOWN, PMTYPE_NORMAL, PMTYPE_COMMAND, PMTYPE_COUNT };

/* Plugin manager forward declaration. */
struct Plugin;
typedef struct Plugin Plugin;

/* Initialize plugin manager. */
extern int pm_init(const char *dirname);

/* Clean up plugin manager. */
extern void pm_deinit(void);

/* Register help for all external commands. */
extern void pm_register_help(const SOCKET fd);

/* Register commands for all external command plugins. */
extern int pm_register_commands(const char *tok, const SOCKET fd);

/* Register all plugins. */
extern void pm_register(const SOCKET fd);

/* Display all available modules. */
extern void pm_show(const SOCKET fd);

/* Find a specific module by name. */
extern Plugin *pm_find(const char *name);

/* Execute a specific plugin. */
extern void pm_exec(Plugin *plugin, const SOCKET fd);

/* Set the commands pointer and count. */
extern void pm_set(Plugin *pm, Command *cmds, int CMD_CNT);

/* Set the type of the plugin. */
extern void pm_settype(Plugin *pm, short unsigned int type);

/* Plugin initialization for commands. */
extern void plugin_init(Plugin *pm);

/* Plugin variable for loaded. */
extern int pm_loaded(void);

#endif
