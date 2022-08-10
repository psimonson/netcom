/*
 * main.c - Source for testing my command interpreter.
 *
 * Author: Philip R. Simonson
 * Date  : 07/24/2022
 *
 ****************************************************************************
 */

#include <stdio.h>
#include "parse.h"
#include "plugin.h"

int plugins_loaded;
int global_done;

/* Initialize winsock for windows.
 */
int ws_init(void)
{
#if defined(_WIN32) || defined(_WIN64)
	WSADATA wsaData;

	return WSAStartup(MAKEWORD(2, 2), &wsaData);
#else
	return 0;
#endif
}

int main()
{
	unsigned short port = 0xBEEF; /* 48640 */
	char addr[INET6_ADDRSTRLEN+1];
	char buf[256];
	SOCKET s, c;
	int nbytes;

	if(pm_init("plugin-sdk") != 0) {
		return 1;
	}
	command_init();
	plugins_loaded = 1;

	if(ws_init() != 0) {
		fprintf(stderr, "Error: Failed to initialize winsock.\n");
		pm_deinit();
		return 1;
	}

	s = server_socket_open(&port);
	if(s == INVALID_SOCKET) {
		fprintf(stderr,
			"Error: Cannot open server socket on port %d.\n",
			port);
		pm_deinit();
		return 1;
	}

	c = server_socket_accept(s);
	if(c == INVALID_SOCKET) {
		printf("Warning: Client connection not accepted.\n");
		socket_close(s);
		pm_deinit();
		return 1;
	}

	get_addr(c, addr, sizeof(addr)-1);
	printf("Client %s connected.\n", addr);
	send(c, ">> ", 4, 0);
	pm_register(c);

	while(!global_done) {
		nbytes = recv(c, buf, sizeof(buf)-1, 0);
		if(nbytes <= 0) {
			if(!nbytes) {
				printf("Client disconnected.\n");
			}
			socket_close(c);
			break;
		}
		else {
			buf[nbytes] = 0;
			(void)parse_input(c, buf);
			if(!global_done) send(c, ">> ", 4, 0);
		}
	}

	printf("Client %s disconnected.\n", addr);
	socket_close(s);
	pm_deinit();
#if defined(_WIN32) || defined(_WIN64)
	WSACleanup();
#endif
	return 0;
}
