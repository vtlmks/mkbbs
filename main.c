//
//       _\____ ___ __  _                                      _ __ ___ ____/_
//         \     _ _    .    ___   ___|   |_,   .   |   |    __     __     /
//     _ ___\ - ( ` ) - | - (  / - \__) - ( \ - | - | - | - (__/ - (  ` - /___
//  _\________        .      ________    _____     _________`-__  ____ ________/_
//   _\  ____/___:::: ::::  _\  ____/____\  _/__  _\_  _____/\_ \/  _/_\  ____/___
//   \________  /::::.:.... \________  /  _____/_/  _____/__  _  _    \________  /
//   /  __  /  /....  ::::_ /  __  /_      /  _ /   /_  _      \/  _ /   __  /  /
//  \____/    /_ :::: :::: \____/   _\_____    \_____   _\____ /   _\_____/    /_
//  jA!_/____/// `:::.::::. .  /____/   _/____/    /____/     /____/e^d _/____///
//     /              :                 /                               /
//                    .
//

#define _CRT_SECURE_NO_DEPRECATE

#include <winsock2.h>
#include <Ws2def.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef float r32;
typedef double r64;

typedef u64 bool;

#define true 1
#define false 0


#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "sqlite3.lib")

#define TELOPTS
#include "telnet.h"
#include "sqlite3.h"

enum {
	TOP_LEVEL,
	SEEN_IAC,
	SEEN_WILL,
	SEEN_WONT,
	SEEN_DO,
	SEEN_DONT,
	SEEN_SB,
	SEEN_CR,
	SUBNEG_OT,
	SUBNEG_IAC,
} state;

#define CR 13
#define LF 10
#define NUL 0

uint64_t telnet_state;

char main_line[] = "[0 p[44;36m ~SERVERNAME  #~NODE  [0m [34m-[35m«[36m([0m~CONF [36m)[35m»[34m- [36m([33m~USERTIMELH :~USERTIMELM  [0mmins. left[36m)[34m: [0m[1 p¤";

char buffer[4096];

static void send_option(SOCKET sock, int command, int option) {
	unsigned char b[3];

	b[0] = IAC;
	b[1] = command;
	b[2] = option;
	send(sock, b, 3, 0);
}

static void display_file(char *filename, SOCKET sock) {
	FILE *f = fopen("ansi-texts\\AwaitScreen.txt", "rb");
	fseek(f, 0, SEEK_END);
	int filesize = ftell(f);
	fseek(f, 0, SEEK_SET);
	fread(buffer, filesize, 1, f);
	fclose(f);
	send(sock, buffer, filesize, 0);
}

static DWORD receive_cmds(LPVOID lpParam) {
	SOCKET sock = (SOCKET)lpParam;

	char buf[100];				// buffer to hold our recived data
	//char sendData[100];		// buffer to hold our sent data
	int res;						// for error checking

	char ask_ttype[] = {
		IAC,SB,TELOPT_NAWS,0,80,0,29,IAC,SE,
	};

	send_option(sock, WILL, TELOPT_ECHO);
	send_option(sock, WILL, TELOPT_SGA);
	send_option(sock, WONT, TELOPT_LINEMODE);
	send(sock, ask_ttype, sizeof(ask_ttype), 0);

	display_file("ansi-texts\\AwaitScreen.txt", sock);

	send(sock, main_line, sizeof(main_line), 0);

	while(1) {
		res = recv(sock, buf, 1, 0); // recv cmds

		if(res == 0) break;	// disconnect

		int c = (unsigned char)*buf;

		switch(telnet_state) {
			case TOP_LEVEL:			// fall through, for now
			case SEEN_CR: {
				if((c == 0) || (c == '\n') && telnet_state == SEEN_CR) {
					telnet_state = TOP_LEVEL;
				} else if(c == IAC) {
					telnet_state = SEEN_IAC;
				} else {
					char cc = c;
					telnet_state = (c == CR) ? SEEN_CR : TOP_LEVEL;
				}
				printf("%c", c);
				send(sock, buf, 1, 0);

			} break;
			case SEEN_IAC: {
				if(c == DO) {
					telnet_state = SEEN_DO;
				} else if(c == DONT) {
					telnet_state = SEEN_DONT;
				} else if(c == WILL) {
					telnet_state = SEEN_WILL;
				} else if(c == WONT) {
					telnet_state = SEEN_WONT;
				} else if(c == SB) {
					telnet_state = SEEN_SB;
				} else if(c == DM) {
					telnet_state = TOP_LEVEL;
				} else {
					printf("%c", c);
					send(sock, buf, 1, 0);
					telnet_state = TOP_LEVEL;
				}
			} break;
			case SEEN_WILL: {
				printf("WILL %s\n", telopts[c]);
				telnet_state = TOP_LEVEL;
			} break;
			case SEEN_WONT: {
				printf("WONT %s\n", telopts[c]);
				telnet_state = TOP_LEVEL;
			} break;
			case SEEN_DO: {
				printf("DO %s\n", telopts[c]);
				telnet_state = TOP_LEVEL;
			} break;
			case SEEN_DONT: {
				printf("DONT %s\n", telopts[c]);
				telnet_state = TOP_LEVEL;
			} break;
			case SEEN_SB: {
				printf("SB %s\n", telopts[c]);
				telnet_state = TOP_LEVEL;
			} break;
			case SUBNEG_OT: {
				printf("OT\n");
				telnet_state = TOP_LEVEL;
			} break;
			case SUBNEG_IAC: {
				printf("IAC\n");
				telnet_state = TOP_LEVEL;
			} break;
		}
	}
	return 0;
}

int main() {
	SOCKET sock;

	DWORD thread;

	WSADATA wsaData;
	SOCKADDR_IN server;

	int ret = WSAStartup(0x101, &wsaData);

	if(ret != 0) {
		return 0;
	}

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(123); // listen on telnet port 123

	sock = socket(AF_INET, SOCK_STREAM, 0);

	if(sock == INVALID_SOCKET) {
		return 0;
	}

	if(bind(sock, (SOCKADDR *)&server, sizeof(server)) != 0) {
		return 0;
	}

	if(listen(sock, 5) != 0) {
		return 0;
	}

	SOCKET client;

	SOCKADDR from;
	int fromlen = sizeof(from);

	while(1) {
		client = accept(sock, (SOCKADDR *)&from, &fromlen);
		printf("Client connected\n");

		CreateThread(0, 0, receive_cmds, (LPVOID)client, 0, &thread);
	}

	closesocket(sock);
	WSACleanup();

	return 0;
}