/*
 * plugin.c - Source for a simple plugin manager.
 *
 * Author: Philip R. Simonson
 * Date  : 07/27/2022
 *
 ****************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "cmd.h"
#include "plugin.h"
#include "parse.h"

#if defined(__linux)
#include <dlfcn.h>
#endif

/* Plugin manager list definition. */
static CList *pm_list;
static int plugin_count;

/* Plugin manager definition. */
struct Plugin {
#if defined(_WIN32) || defined(_WIN64)
	HANDLE sym;
#else
	void *sym;
#endif
	char *name;
	Command *cmds;
	unsigned int cmd_cnt;
	short unsigned int id;
	short unsigned int type;
	void (*func)(Plugin *self, const SOCKET fd);
};

/* Create a new plugin.
 */
static Plugin *pm_new(void *sym, const char *name,
	short unsigned int type, void (*func)(Plugin *self, const SOCKET fd))
{
	Plugin *pm;

	pm = (Plugin*)malloc(sizeof(Plugin));
	if(pm != NULL) {
		const unsigned int len = strlen(name);
		pm->name = (char *)malloc(sizeof(char)*(len+1));
		if(pm->name == NULL) {
			free(pm);
			return NULL;
		}
		memcpy(pm->name, name, len);
		pm->name[len] = 0;
		pm->func = func;
		pm->type = type;
		pm->id = plugin_count;
		pm->cmds = NULL;
		pm->cmd_cnt = 0;
		pm->sym = sym;
	}
	return pm;
}

/* Free a plugin.
 */
static void pm_free(void *data)
{
	Plugin *pm = (Plugin *)data;

	if(pm != NULL) {
		pm->id = 0;
		pm->type = PMTYPE_UNKNOWN;
		pm->func = NULL;
		free(pm->name);
		free(pm);
	}
}

/* Discover plugins in given directory.
 */
static int pm_load(const char *dirname)
{
	struct dirent *p;
	DIR *dir;

	if((dir = opendir(dirname)) == NULL) {
		printf("Gets here.\n");
		return -1;
	}

	while((p = readdir(dir)) != NULL) {
		char path[2048];
		Plugin *plugin = NULL;
#if defined(_WIN32) || defined(_WIN64)
		HANDLE sym = NULL;
#else
		void *sym = NULL;
#endif

		/* Do not get current directory or previous. */
		if((strncmp(p->d_name, ".", 2)
				&& strncmp(p->d_name, "..", 3)) == 0) {
			continue;
		}

#if defined(_WIN32) || defined(_WIN64)
		if(!strstr(p->d_name, ".dll")) {
			continue;
		}
#else
		if(!strstr(p->d_name, ".so")) {
			continue;
		}
#endif

		snprintf(path, sizeof(path)-1, "%s/%s", dirname, p->d_name);
#if defined(_WIN32) || defined(_WIN64)
		/* Load plugin into list. */
		sym = LoadLibrary(path);
		if(sym != NULL) {
			void *func = GetProcAddress(sym, "init");
			if(func != NULL) {
				plugin = pm_new(sym, p->d_name, PMTYPE_UNKNOWN, func);
				if(plugin != NULL) {
					clist_add(&pm_list, CLIST_TYPE_STRUCT, plugin, pm_free);
					++plugin_count;
					printf("Loaded plugin: %s\n", path);
				}
			}
		}
#else
		/* Load plugin into list. */
		sym = dlopen(path, RTLD_LAZY);
		if(sym != NULL) {
			void *func = dlsym(sym, "init");
			if(func != NULL) {
				plugin = pm_new(sym, p->d_name, PMTYPE_UNKNOWN, func);
				if(plugin != NULL) {
					clist_add(&pm_list, CLIST_TYPE_STRUCT, plugin, pm_free);
					++plugin_count;
				}
			}
		}
#endif
	}
	closedir(dir);
	return 0;
}

/* -------------------------- Public Functions --------------------------- */

/* Initialize the plugins and load them all.
 */
int pm_init(const char *dirname)
{
	pm_list = clist_init();
	if(pm_load(dirname) < 0) {
		printf("Cannot initialize plugins.\n");
		return -1;
	}
	printf("Total plugins loaded %d.\n", plugin_count);
	return 0;
}

/* De-initialize the plugins and free all resources.
 */
void pm_deinit(void)
{
	CList *tmp = pm_list;

	while(tmp != NULL) {
		Plugin *plugin = (Plugin *)clist_getdata(tmp);
		if(plugin != NULL) {
#if defined(_WIN32) || defined(_WIN64)
			FreeLibrary(plugin->sym);
#else
			dlclose(plugin->sym);
#endif
			--plugin_count;
		}
		tmp = clist_getnext(tmp);
	}
	clist_free(pm_list);
	pm_list = NULL;
	printf("Plugins deactivated.\n");
}

/* Register plugin help for command plugins.
 */
void pm_register_help(const SOCKET fd)
{
	CList *tmp = pm_list;
	AppendBuffer *ab;
	char buf[1024];
	unsigned int len;

	while(tmp != NULL) {
		Plugin *plugin = (Plugin *)clist_getdata(tmp);
		if(plugin != NULL && plugin->type == PMTYPE_COMMAND) {
			const char *tmp = NULL;
			unsigned int i;

			ab = ab_init();
			for(i = 0; i < plugin->cmd_cnt; i++) {
				len = snprintf(buf, sizeof(buf) - 1,
					"%-10s - [%-5s]: %s\r\n",
					plugin->cmds[i].name,
					plugin->cmds[i].args,
					plugin->cmds[i].help);
				ab_append(ab, buf, len);
			}
			tmp = (const char *)ab_getdata(ab);
			send(fd, tmp, strlen(tmp), 0);
			ab_free(ab);
		}
		tmp = clist_getnext(tmp);
	}
}

/* Register commands from command plugin.
 */
int pm_register_commands(const char *tok, const SOCKET fd)
{
	CList *tmp = pm_list;

	while(tmp != NULL) {
		Plugin *plugin = (Plugin *)clist_getdata(tmp);
		if(plugin != NULL && plugin->type == PMTYPE_COMMAND) {
			unsigned int i;

			i = plugin->cmd_cnt;
			while(i--) {
				const Command *cmd = &plugin->cmds[i];

				if(!strncmp(tok, cmd->name, strlen(tok))) {
					Argument *args = arg_parser(cmd->args);
					int rc;

					if(args == NULL && strlen(cmd->args)) {
						send(fd,
							"Bad argument(s).\r\n",
							18, 0);
						return 1;
					}

					tok = strtok(NULL, DELIM);
					if(tok != NULL) {
						send(fd,
							"Bad argument(s).\r\n",
							18, 0);
						free(args);
						return 1;
					}

					rc = cmd->func(fd, args);
					free(args);
					return rc;
				}
			}
		}
		tmp = clist_getnext(tmp);
	}

	send(fd, "Bad command.\r\n", 14, 0);
	return 1;
}

/* Register plugin hook for normal plugins.
 */
void pm_register(const SOCKET fd)
{
	CList *tmp = pm_list;

	while(tmp != NULL) {
		Plugin *plugin = (Plugin *)clist_getdata(tmp);
		if(plugin != NULL) {
			plugin->func(plugin, fd);
		}
		tmp = clist_getnext(tmp);
	}
}

/* Display all available normal modules.
 */
void pm_show(const SOCKET fd)
{
	CList *tmp = pm_list;
	char buf[2048];
	unsigned int total;
	unsigned int len;

	total = 0;
	memset(buf, 0, sizeof(buf));
	while(tmp != NULL) {
		Plugin *plugin = (Plugin *)clist_getdata(tmp);
		if(plugin != NULL && plugin->type == PMTYPE_NORMAL) {
			len = snprintf(buf+total, sizeof(buf)-1, "%s\r\n",
					plugin->name);
			if(len > 0) {
				total += len;
			}
		}
		tmp = clist_getnext(tmp);
	}
	send(fd, buf, len, 0);
}

/* Find a specific module by name.
 */
Plugin *pm_find(const char *name)
{
	CList *tmp = pm_list;
	Plugin *found = NULL;

	while(tmp != NULL) {
		Plugin *plugin = (Plugin *)clist_getdata(tmp);
		if(plugin != NULL && plugin->type == PMTYPE_NORMAL) {
			const char *name_end = strchr(plugin->name, '.');
			if(name_end && !memcmp(plugin->name, name, name_end - plugin->name)) {
				found = plugin;
				break;
			}
		}
		tmp = clist_getnext(tmp);
	}
	return found;
}

/* Exec a specific module.
 */
void pm_exec(Plugin *plugin, const SOCKET fd)
{
	if(plugin != NULL && plugin->type == PMTYPE_NORMAL) {
		plugin->func(plugin, fd);
	}
}

/* Set the commands pointer and count.
 */
void pm_set(Plugin *pm, Command *cmds, int CMD_CNT)
{
	if(pm != NULL) {
		pm->cmds = cmds;
		pm->cmd_cnt = CMD_CNT;
	}
}

/* Set type of plugin.
 */
void pm_settype(Plugin *pm, short unsigned int type)
{
	if(pm != NULL) {
		pm->type = type >= PMTYPE_COUNT ? PMTYPE_UNKNOWN : type;
	}
}
