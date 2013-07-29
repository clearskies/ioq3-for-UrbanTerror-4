/*
*
* IRC for ioq3
* ----------------------
* This is in development - it may cause your client to crash
* Use at your own risk
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "client.h"

int sock;
int irc_ready, irc_connected;
int irc_joining, irc_parting, irc_saying, irc_msging;

int nextRead = 0;

char curServer[128];

void IRC_HandleData(void) {
	if (nextRead < 10) {
		nextRead++;
		return;
	} else {
		nextRead = 0;
	}
	char buffer[256];
	char obuf[256];
	char idata[10][256];
	int s;

	if (!irc_connected)
		return;

	//while (1) {
		if (sock < 0) {
			return;
		}
		bzero(buffer, 256);
		s = read(sock, buffer, 255);
		if (s < 0) {
			 Com_Printf("Error getting data from server.\n");
			 return;
		}
		Com_Printf(buffer);

		if (strcmp(buffer, "PING") > 0) {
			sscanf(buffer, "PING %s", idata[0]);
			sprintf(obuf, "PONG %s\n", idata[0]);
			write(sock, obuf, strlen(obuf));
			bzero(obuf, 256);
			bzero(idata[0], 256);
			return;
		}

		if (strstr(buffer, "001") && !irc_ready) {
			Com_Printf("IRC is ready to go!\n");
			irc_ready = 1;
			return;
		}

		if (strstr(buffer, "PRIVMSG")) {
			sscanf(buffer, "%*s PRIVMSG %s :%[^\n]", idata[0], idata[1]);
			if (idata[0][0] == '#') {
				Com_Printf("[%s]: %s\n", idata[0], idata[1]);
			} else {
				Com_Printf("Message from %s: %s\n", idata[0], idata[1]);
			}
			bzero(idata[0], 256);
			bzero(idata[1], 256);
			return;
		}

		if (irc_parting && (strstr(buffer, "403") || strstr(buffer, "442"))) {
			Com_Printf("Channel leave failed. You might not be on that channel, or it may not exist.\n");
			irc_parting = 0;
			return;
		}

		if (irc_joining && (strstr(buffer, "403") || strstr(buffer, "473"))) {
			Com_Printf("Channel join failed. That channel might not exist.\n");
			irc_joining = 0;
			return;
		}

		if (irc_saying && strstr(buffer, "404")) {
			Com_Printf("You can't say stuff on that channel. You're probably not in it.\n");
			irc_saying = 0;
			return;
		}

		if (irc_msging && strstr(buffer, "401")) {
			Com_Printf("That person doesn't exist.\n");
			irc_msging = 0;
			return;
		}
	//}
}

void IRC_Connect(void) {
	if (Cmd_Argc() < 4) {
		Com_Printf("Usage: irc_open <host> <port> <name>\n");
		return;
	}

	long int port;
	int s;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	char obuf[256];

	if (irc_connected) {
		IRC_Disconnect();
	}

	irc_connected = 0;
	irc_ready = 0;

	if (!(sock < 0)) {
		close(sock);
	}

	port = strtol(Cmd_Argv(2), NULL, 10);
	if (!port) {
		Com_Printf("Invalid port number.\n");
		return;
	}

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		Com_Printf("Error creating socket.\n");
		return;
	}

	server = gethostbyname(Cmd_Argv(1));
	if (server == NULL) {
		Com_Printf("No such host: %s\n", Cmd_Argv(1));
		return;
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, 
		 (char *)&serv_addr.sin_addr.s_addr,
		 server->h_length);
	serv_addr.sin_port = htons(port);
	if (connect(sock,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
		Com_Printf("Could not connect to server.\n");
		return;
	}

	irc_connected = 1;
	strcpy(curServer, Cmd_Argv(1));

	sprintf(obuf, "USER %s * * :%s\r\n", Cmd_Argv(3), Cmd_Argv(3));
	Com_Printf(obuf);
	s = write(sock, obuf, strlen(obuf));
	if (s < 0) {
		Com_Printf("Error sending data to server.\n");
		return;
	}
	bzero(obuf, 256);

	sprintf(obuf, "NICK %s\r\n", Cmd_Argv(3));
	Com_Printf(obuf);
	s = write(sock, obuf, strlen(obuf));
	if (s < 0) {
		Com_Printf("Error sending data to server.\n");
		return;
	}
	bzero(obuf, 256);

	return;
}

void IRC_Disconnect(void) {
	close(sock);
	Com_Printf("IRC disconnected: %s.\n", curServer);
	bzero(curServer, 128);
}

void IRC_Join(void) {
	if (!irc_connected) {
		Com_Printf("Connect to an IRC server first.\n");
		return;
	}
	if (!irc_ready) {
		Com_Printf("We are not ready to join a channel yet, please wait.\n");
		return;
	}

	if (Cmd_Argc() < 2) {
		Com_Printf("Usage: irc_join <channelname>\n");
		return;
	}

	char joinstr[64];
	int s;

	sprintf(joinstr, "JOIN %s\r\n", Cmd_Argv(1));
	s = write(sock, joinstr, strlen(joinstr));
	if (s < 0) {
		Com_Printf("Error sending data to server.\n");
	}
	irc_joining = 1;
}

void IRC_Leave(void) {
	if (!irc_connected) {
		Com_Printf("Connect to an IRC server first.\n");
		return;
	}
	if (!irc_ready) {
		Com_Printf("We are not ready to leave a channel yet, please wait.\n");
		return;
	}

	if (Cmd_Argc() < 2) {
		Com_Printf("Usage: irc_leave <channelname>\n");
		return;
	}

	char leavestr[64];
	int s;

	sprintf(leavestr, "PART %s\r\n", Cmd_Argv(1));
	s = write(sock, leavestr, strlen(leavestr));
	if (s < 0) {
		Com_Printf("Error sending data to server.\n");
	}
	irc_parting = 1;
}

void IRC_Say(void) {
	if (!irc_connected) {
		Com_Printf("Connect to an IRC server first.\n");
		return;
	}
	if (!irc_ready) {
		Com_Printf("We are not ready to say anything, please wait.\n");
		return;
	}

	if (Cmd_Argc() < 3) {
		Com_Printf("Usage: irc_say <channelname> <message>\n");
		return;
	}

	char saystr[MAX_STRING_CHARS];
	int s;

	sprintf(saystr, "PRIVMSG %s :%s\n", Cmd_Argv(1), Cmd_ArgsFrom(2));
	s = write(sock, saystr, strlen(saystr));
	if (s < 0) {
		Com_Printf("Error sending data to server.\n");
	}
	irc_saying = 1;
}

void IRC_Msg(void) {
	if (!irc_connected) {
		Com_Printf("Connect to an IRC server first.\n");
		return;
	}
	if (!irc_ready) {
		Com_Printf("We are not ready to send a message yet, please wait.\n");
		return;
	}

	if (Cmd_Argc() < 3) {
		Com_Printf("Usage: irc_msg <nickname> <message>\n");
		return;
	}
	char msgstr[MAX_STRING_CHARS];
	int s;

	sprintf(msgstr, "PRIVMSG %s :%s\n", Cmd_Argv(1), Cmd_ArgsFrom(2));
	s = write(sock, msgstr, strlen(msgstr));
	if (s < 0) {
		Com_Printf("Error sending data to server.\n");
	}
	irc_msging = 1;
}

void IRC_Nick(void) {
	if (!irc_connected) {
		Com_Printf("Connect to an IRC server first.\n");
		return;
	}
	if (!irc_ready) {
		Com_Printf("We are not ready to leave a channel yet, please wait.\n");
		return;
	}

	if (Cmd_Argc() < 2) {
		Com_Printf("Usage: irc_nick <nick>\n");
		return;
	}
	int nickstr[MAX_STRING_CHARS];
	int s;

	sprintf(nickstr, "NICK %s\n", Cmd_Argv(1));
	s = write(sock, nickstr, strlen(nickstr));
	if (s < 0) {
		Com_Printf("Error sending data to server.\n");
	}
}

void IRC_Init(void) {
	Cmd_AddCommand("irc_open", IRC_Connect);
	Cmd_AddCommand("irc_close", IRC_Disconnect);
	Cmd_AddCommand("irc_join", IRC_Join);
	Cmd_AddCommand("irc_leave", IRC_Leave);
	Cmd_AddCommand("irc_say", IRC_Say);
	Cmd_AddCommand("irc_msg", IRC_Msg);
	Cmd_AddCommand("irc_nick", IRC_Nick);
}
