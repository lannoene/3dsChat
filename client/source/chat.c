#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>

#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>

#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000

#include <3ds.h>

#include "chat.h"
#include "draw.h"
#include "config.h"

int chatOffset = 220;

struct log {
	char msgHead[HEADER_SIZE];
	char msgBody[BODY_SIZE];
	char msgName[NAME_SIZE];
} logMsgs[MAX_LOG];

struct Rmessage {
	char msgHead[HEADER_SIZE];
	char msgBody[BODY_SIZE];
	char msgName[NAME_SIZE];
} recvMsg;

struct Smessage {
	char msgHead[HEADER_SIZE];
	char msgBody[BODY_SIZE];
	char msgName[NAME_SIZE];
} sendMsg;

int recvdMsgs = 0;
s32 sock = -1;

void resetMsgVars(bool resetRecv, bool resetSend) {
	if (resetRecv == true) {
		memset(sendMsg.msgHead, 0, sizeof(sendMsg.msgHead));
		memset(sendMsg.msgBody, 0, sizeof(sendMsg.msgBody));
	}
	
	if (resetSend == true) {
		memset(recvMsg.msgHead, 0, sizeof(recvMsg.msgHead));
		memset(recvMsg.msgBody, 0, sizeof(recvMsg.msgBody));
	}
}

void recvChat() {
	resetMsgVars(true, true);
	int iResult = 0;
	
	struct pollfd sock_descriptor;
	sock_descriptor.fd = sock;
	sock_descriptor.events = POLLIN;

	int return_value = poll(&sock_descriptor, 1, 0);

		if (return_value == -1) {
		printf("%s", strerror(errno));
	} else if (return_value == 0) {
		//printf("No data available to be read");
	} else {
		if (sock_descriptor.revents & POLLIN) {
			iResult = recv(sock, &recvMsg, sizeof(recvMsg), 0);
		}
	}
	if (iResult > 0) {
		printf("Bytes received: %d\n", iResult);
		
		if (strstr(recvMsg.msgHead, "EXIT.") != 0) {
			return;
		} else if (strstr(recvMsg.msgHead, "TEXT.") != 0) {
			printf("%s\n", recvMsg.msgBody);
			strcpy(logMsgs[recvdMsgs].msgBody, recvMsg.msgBody);
			strcpy(logMsgs[recvdMsgs].msgName, recvMsg.msgName);
			++recvdMsgs;
		} else if (strstr(recvMsg.msgHead, "IMAGE.") != 0) {
			puts("feature not implimented yet! XD");
			++recvdMsgs;
		} else if (strstr(recvMsg.msgHead, "STATUS.") != 0) {
			sendStatusMsg(recvMsg.msgBody);
		}
	} else if (iResult == 0) {
		
	}
}

void displayChat() {
	int n = recvdMsgs - 1;
	for (int i = 0; i < recvdMsgs; i++) {
		if (strcmp(logMsgs[n].msgHead, "STATUS.") == 0) {
			char* statusText = strdup(logMsgs[n].msgBody);
			text(statusText, 2, chatOffset - i*15, 0.5f, ALIGN_LEFT);
		} else {
			char fullPrnt[50];
			snprintf(fullPrnt, 50, "%s said: %s", logMsgs[n].msgName, logMsgs[n].msgBody);
			text(fullPrnt, 2, chatOffset - i*15, 0.5f, ALIGN_LEFT);
		}
		--n;
	}
}
SwkbdState swkbd;

SwkbdStatusData swkbdStatus;
SwkbdLearningData swkbdLearning;

void sendMsgSocket(struct jsonParse *config) {
	swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 2, -1);
	swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, SWKBD_FILTER_DIGITS | SWKBD_FILTER_AT | SWKBD_FILTER_PERCENT | SWKBD_FILTER_BACKSLASH | SWKBD_FILTER_PROFANITY, -1);
	swkbdSetFeatures(&swkbd, SWKBD_MULTILINE | SWKBD_PREDICTIVE_INPUT);
	static bool reload = false;
	swkbdSetStatusData(&swkbd, &swkbdStatus, reload, true);
	swkbdSetLearningData(&swkbd, &swkbdLearning, reload, true);
	swkbdSetHintText(&swkbd, "Send message!");
	int button = swkbdInputText(&swkbd, sendMsg.msgBody, sizeof(sendMsg.msgBody));
	strcpy(sendMsg.msgHead, "TEXT.");
	strcpy(sendMsg.msgName, config->name);
	if (button == 2 && send(sock, &sendMsg, sizeof(sendMsg), 0) < 0) {
		sendStatusMsg("Could not send message (server may be offline)");
		return;
	}
	//printf("Sent: %s\n", sendMsg);
}


int initSockets(char* serverIp) {
	struct sockaddr_in server;
	static u32 *SOC_buffer = NULL;
	int ret;
	
	// allocate buffer for SOC service
	SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);

	if(SOC_buffer == NULL) {
		//failExit("memalign: failed to allocate\n");
	}

	// Now intialise soc:u service
	if ((ret = socInit(SOC_buffer, SOC_BUFFERSIZE)) != 0) {
    	//failExit("socInit: 0x%08X\n", (unsigned int)ret);
	}

	// register socShutdown to run at exit
	// atexit functions execute in reverse order so this runs before gfxExit
	//extern atexit(socShutdown);

	if (sock < 0) {
		//failExit("socket: %d %s\n", errno, strerror(errno));
	}
	
	memset(&server, 0, sizeof (server));

	server.sin_family = AF_INET;
	server.sin_port = htons(80);
	server.sin_addr.s_addr = inet_addr(serverIp);
	
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	
	if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
		sendStatusMsg(strerror(errno));
		return 1;
	}
	
	return 0;
}

void moveChat(int way) {
	if (way == 0 && chatOffset < recvdMsgs*15) {
		chatOffset += 15;
	} else if (way == 1 && chatOffset > 220) {
		chatOffset -= 15;
	}
}

void serverSend(char* header, char* body) {
	resetMsgVars(true, true);
	
	strcpy(sendMsg.msgHead, header);
	strcpy(sendMsg.msgBody, body);
	send(sock, &sendMsg, sizeof(sendMsg), 0);
}

void sendStatusMsg(char* message) {
	strcpy(logMsgs[recvdMsgs].msgHead, "STATUS.");
	strcpy(logMsgs[recvdMsgs].msgBody, message);
	++recvdMsgs;
}

void exitSocket() {
	close(sock);
}