
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

/* telnet options */
#define TELOPT_BINARY			0			/* 8-bit data path */
#define TELOPT_ECHO				1			/* echo */
#define TELOPT_RCP				2			/* prepare to reconnect */
#define TELOPT_SGA				3			/* suppress go ahead */
#define TELOPT_NAMS				4			/* approximate message size */
#define TELOPT_STATUS			5			/* give status */
#define TELOPT_TM					6			/* timing mark */
#define TELOPT_RCTE				7			/* remote controlled transmission and echo */
#define TELOPT_NAOL			 	8			/* negotiate about output line width */
#define TELOPT_NAOP			 	9			/* negotiate about output page size */
#define TELOPT_NAOCRD			10			/* negotiate about CR disposition */
#define TELOPT_NAOHTS			11			/* negotiate about horizontal tabstops */
#define TELOPT_NAOHTD			12			/* negotiate about horizontal tab disposition */
#define TELOPT_NAOFFD			13			/* negotiate about formfeed disposition */
#define TELOPT_NAOVTS			14			/* negotiate about vertical tab stops */
#define TELOPT_NAOVTD			15			/* negotiate about vertical tab disposition */
#define TELOPT_NAOLFD			16			/* negotiate about output LF disposition */
#define TELOPT_XASCII			17			/* extended ascic character set */
#define TELOPT_LOGOUT			18			/* force logout */
#define TELOPT_BM					19			/* byte macro */
#define TELOPT_DET				20			/* data entry terminal */
#define TELOPT_SUPDUP			21			/* supdup protocol */
#define TELOPT_SUPDUPOUTPUT	22			/* supdup output */
#define TELOPT_SNDLOC			23			/* send location */
#define TELOPT_TTYPE				24			/* terminal type */
#define TELOPT_EOR				25			/* end or record */
#define TELOPT_TUID				26			/* TACACS user identification */
#define TELOPT_OUTMRK			27			/* output marking */
#define TELOPT_TTYLOC			28			/* terminal location number */
#define TELOPT_3270REGIME		29			/* 3270 regime */
#define TELOPT_X3PAD				30			/* X.3 PAD */
#define TELOPT_NAWS				31			/* window size */
#define TELOPT_TSPEED			32			/* terminal speed */
#define TELOPT_LFLOW				33			/* remote flow control */
#define TELOPT_LINEMODE			34			/* Linemode option */
#define TELOPT_XDISPLOC			35			/* X Display Location */
#define TELOPT_OLD_ENVIRON		36			/* Old - Environment variables */
#define TELOPT_AUTHENTICATION 37			/* Authenticate */
#define TELOPT_ENCRYPT			38			/* Encryption option */
#define TELOPT_NEW_ENVIRON		39			/* New - Environment variables */
#define TELOPT_EXOPL				255		/* extended-options-list */

#define	NTELOPTS	(1+TELOPT_NEW_ENVIRON)
char *telopts[NTELOPTS+1] = {
	"BINARY", "ECHO", "RCP", "SUPPRESS GO AHEAD", "NAME",
	"STATUS", "TIMING MARK", "RCTE", "NAOL", "NAOP",
	"NAOCRD", "NAOHTS", "NAOHTD", "NAOFFD", "NAOVTS",
	"NAOVTD", "NAOLFD", "EXTEND ASCII", "LOGOUT", "BYTE MACRO",
	"DATA ENTRY TERMINAL", "SUPDUP", "SUPDUP OUTPUT",
	"SEND LOCATION", "TERMINAL TYPE", "END OF RECORD",
	"TACACS UID", "OUTPUT MARKING", "TTYLOC",
	"3270 REGIME", "X.3 PAD", "NAWS", "TSPEED", "LFLOW",
	"LINEMODE", "XDISPLOC", "OLD-ENVIRON", "AUTHENTICATION",
	"ENCRYPT", "NEW-ENVIRON",
	0
};

#define TTYPE			24
#define SEND			1

uint64_t telnet_state;


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
		IAC,SB,TTYPE,SEND,IAC,SE,
	};

	send_option(sock, WILL, TELOPT_ECHO);
	send_option(sock, WILL, TELOPT_SGA);
	send_option(sock, WONT, TELOPT_LINEMODE);
	send(sock, ask_ttype, sizeof(ask_ttype), 0);

	display_file("ansi-texts\\AwaitScreen.txt", sock);

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