
#define _CRT_SECURE_NO_DEPRECATE

#include <winsock2.h>
#include <Ws2def.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#pragma comment(lib, "ws2_32.lib")

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

static DWORD receive_cmds(LPVOID lpParam) {
	SOCKET current_client = (SOCKET)lpParam;

	char buf[100];				// buffer to hold our recived data
	char sendData[100];		// buffer to hold our sent data
	int res;						// for error checking


#define IAC     255                    /* interpret as command: */
#define DONT    254                    /* you are not to use option */
#define DO      253                    /* please, you use option */
#define WONT    252                    /* I won't use option */
#define WILL    251                    /* I will use option */
#define SB      250                    /* interpret as subnegotiation */
#define SE      240                    /* end sub negotiation */

#define GA      249                    /* you may reverse the line */
#define EL      248                    /* erase the current line */
#define EC      247                    /* erase the current character */
#define AYT     246                    /* are you there */
#define AO      245                    /* abort output--but let prog finish */
#define IP      244                    /* interrupt process--permanently */
#define BREAK   243                    /* break */
#define DM      242                    /* data mark--for connect. cleaning */
#define NOP     241                    /* nop */
#define EOR     239                    /* end of record (transparent mode) */
#define ABORT   238                    /* Abort process */
#define SUSP    237                    /* Suspend process */
#define xEOF    236                    /* End of file: EOF is already used... */


#define TELNET_WILL	251
#define TELNET_WONT	252
#define TELNET_DO		253
#define TELNET_DONT	254
#define IAC				255

#define SB				250
#define TTYPE			24
#define SEND			1
#define SE				240

	char wont_linemode[] = {
		IAC, TELNET_DONT, 34,

		IAC,SB,TTYPE,SEND,IAC,SE,

		//IAC, WILL_USE, 1,
		//255, 252, 34,
		//255, 254, 34,
		//255, 251, 1,

	};


	send(current_client, wont_linemode, sizeof(wont_linemode), 0);

	while(1) {
		res = recv(current_client, buf, 1, 0); // recv cmds

		int c = (unsigned char)*buf;

		switch(telnet_state) {
			case TOP_LEVEL: //{		// fall through, for now
//			} break;
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
					telnet_state = TOP_LEVEL;
				}
			} break;
			case SEEN_WILL: {
				printf("WILL\n");
				telnet_state = TOP_LEVEL;
			} break;
			case SEEN_WONT: {
				printf("WONT\n");
				telnet_state = TOP_LEVEL;
			} break;
			case SEEN_DO: {
				printf("DO\n");
				telnet_state = TOP_LEVEL;
			} break;
			case SEEN_DONT: {
				printf("DONT\n");
				telnet_state = TOP_LEVEL;
			} break;
			case SEEN_SB: {
				printf("SB\n");
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