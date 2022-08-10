/*
 * cmd.c - Source for a command interpreter.
 *
 * Author: Philip R. Simonson
 * Date  : 07/23/2022
 *
 ****************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <time.h>

#if defined(__linux)
#include <unistd.h>
#endif

#include "abuffer.h"
#include "plugin.h"
#include "parse.h"

/* Tell program that it's finished. */
extern int global_done;
extern int plugins_loaded;

/* Lookup table for date (month). */
static char *month[] = {
	"January",
	"February",
	"March",
	"April",
	"May",
	"June",
	"July",
	"August",
	"September",
	"October",
	"November",
	"December"
};

/* ---------------------------- Commands Here --------------------------- */

CMD_DEF(help);
CMD_DEF(when);
CMD_DEF(list);
CMD_DEF(sdir);
CMD_DEF(cdir);
CMD_DEF(run);
CMD_DEF(mods);
CMD_DEF(exit);

static Command cmds[] = {
	CMD_ADD1(help, "", "Display command information."),
	CMD_ADD1(when, "s", "Display time/date, just type 'time' or 'date'."),
	CMD_ADD1(list, "", "List current working directory."),
	CMD_ADD1(sdir, "s", "Switch to a different directory."),
	CMD_ADD2(pdir, "", "Previous working directory.", sdir),
	CMD_ADD1(cdir, "", "Current working directory."),
	CMD_ADD1(run, "s", "Launch a module from plugins directory."),
	CMD_ADD1(mods, "s", "Show/Reload modules, "
			"just type 'show' or 'reload'."),
	CMD_ADD1(exit, "", "Exit this application.")
};
static int CMD_CNT = sizeof(cmds) / sizeof(cmds[0]);

CMD_DEF(help)
{
	AppendBuffer *ab;
	char buf[256];
	char *tmp;
	int len;
	int i;

	ab = ab_init();
	for(i = 0; i < CMD_CNT; i++) {
		len = snprintf(buf, sizeof(buf) - 1, "%-10s - [%-5s]: %s\r\n",
			cmds[i].name, cmds[i].args, cmds[i].help);
		ab_append(ab, buf, len);
	}
	tmp = (char *)ab_getdata(ab);
	send(fd, tmp, strlen(tmp), 0);
	ab_free(ab);

	pm_register_help(fd);
	return 0;
}

CMD_DEF(when)
{
	char buf[512];
	struct tm *tm;
	time_t now;

	now = time(NULL);
	tm = localtime(&now);
	memset(buf, 0, sizeof(buf));
	if(!strncmp(args[0].s, "time", 5)) {
		snprintf(buf, sizeof(buf)-1, "%02d:%02d:%02d\r\n",
			tm->tm_hour, tm->tm_min, tm->tm_sec);
	}
	else if(!strncmp(args[0].s, "date", 5)) {
		snprintf(buf, sizeof(buf)-1, "%s %d, %4d\r\n",
			month[tm->tm_mon], tm->tm_mday, tm->tm_year + 1900);
	}
	else {
		snprintf(buf, sizeof(buf)-1,
			"Sorry I'm having trouble, what do you mean?\r\n");
	}
	send(fd, buf, strlen(buf), 0);
	return 0;
}

CMD_DEF(list)
{
	char buf[512];
	struct dirent *p;
	DIR *dir;

	if((dir = opendir(".")) == NULL) {
		snprintf(buf, sizeof(buf)-1, "Cannot open directory: %s\r\n",
			args != NULL ? args[0].s : ".");
		send(fd, buf, strlen(buf), 0);
		return 1;
	}

	while((p = readdir(dir)) != NULL) {
		if((strcmp(p->d_name, ".") && strcmp(p->d_name, "..")) != 0) {
			snprintf(buf, sizeof(buf)-1, "%s\r\n", p->d_name);
			send(fd, buf, strlen(buf), 0);
		}
	}
	closedir(dir);
	return 0;
}

CMD_DEF(sdir)
{
	char buf[512];
	if(args == NULL) {
		snprintf(buf, sizeof(buf)-1, "Previous directory.\r\n");
	}
	else {
		snprintf(buf, sizeof(buf)-1, "Directory: %s\r\n", args[0].s);
	}
	send(fd, buf, strlen(buf), 0);
	return chdir(args != NULL ? args[0].s : "..");
}

CMD_DEF(cdir)
{
	char output[2048];
	char buf[1024];

	getcwd(buf, sizeof(buf)-1);
	snprintf(output, sizeof(output)-1, "Current directory: %s\r\n", buf);
	send(fd, output, strlen(output), 0);
	return 0;
}

CMD_DEF(run)
{
	Plugin *plugin = NULL;

	plugin = pm_find(args[0].s);
	if(plugin != NULL) {
		pm_exec(plugin, fd);
		return 0;
	}
	send(fd, "Cannot find module.\r\n", 21, 0);
	return 1;
}

CMD_DEF(mods)
{
	if(!strncmp(args[0].s, "show", 5)) {
		pm_show(fd);
	}
	else if(!strncmp(args[0].s, "start", 6)) {
		if(!plugins_loaded) {
			pm_init("plugin-sdk");
			pm_register(fd);
			send(fd, "Plugins started!\r\n", 18, 0);
			plugins_loaded = 1;
			return 0;
		}
		send(fd, "Plugins loaded already!\r\n", 25, 0);
	}
	else if(!strncmp(args[0].s, "stop", 5)) {
		if(plugins_loaded) {
			pm_deinit();
			send(fd, "Plugins stopped!\r\n", 18, 0);
			plugins_loaded = 0;
			return 0;
		}
		send(fd, "Plugins unloaded already!\r\n", 27, 0);
	}
	else if(!strncmp(args[0].s, "reload", 7)) {
		if(plugins_loaded) {
			pm_deinit();
			pm_init("plugin-sdk");
			pm_register(fd);
			send(fd, "Plugins reloaded!\r\n", 19, 0);
			return 0;
		}
		send(fd, "Plugins not loaded use 'start'.\r\n", 33, 0);
	}
	else {
		send(fd, "Invalid option.\r\n", 17, 0);
	}
	return 1;
}

CMD_DEF(exit)
{
	global_done = 1;
	return 0;
}

PARSE_INIT(cmds, CMD_CNT);

