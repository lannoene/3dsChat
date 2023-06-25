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
#include <stdarg.h>

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
static u32 *SOC_buffer = NULL;
struct pollfd sock_descriptor;

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
			if (recvdMsgs < MAX_LOG) {
				strcpy(logMsgs[recvdMsgs].msgBody, recvMsg.msgBody);
				strcpy(logMsgs[recvdMsgs].msgName, recvMsg.msgName);
				strcpy(logMsgs[recvdMsgs].msgHead, recvMsg.msgHead);
				++recvdMsgs;
			} else {
				offsetAllItemsDownByOne();
				strcpy(logMsgs[MAX_LOG - 1].msgBody, recvMsg.msgBody);
				strcpy(logMsgs[MAX_LOG - 1].msgName, recvMsg.msgName);
				strcpy(logMsgs[MAX_LOG - 1].msgHead, recvMsg.msgHead);
			}
			
		} else if (strstr(recvMsg.msgHead, "IMAGE.") != 0) {
			/*puts("feature not implimented yet! XD");
			++recvdMsgs;*/
		} else if (strstr(recvMsg.msgHead, "STATUS.") != 0) {
			sendStatusMsg(recvMsg.msgBody);
		}
	} else if (iResult == 0) {
		
	}
}

void displayChat(struct jsonParse *config) {
	if (recvdMsgs == 0) {
		text("Connect to a server by pressing 'x' and typing in the server ip", 200, 100, 0.5f, ALIGN_CENTER);
		text("Edit settings by pressing 'select'", 200, 115, 0.5f, ALIGN_CENTER);
		text("Quick connect by pressing 'Y'", 200, 130, 0.5f, ALIGN_CENTER);
	}
	int n = recvdMsgs - 1;
	for (int i = 0; i < recvdMsgs; i++) {
		if (strcmp(logMsgs[n].msgHead, "STATUS.") == 0) {
			char* statusText = strdup(logMsgs[n].msgBody);
			text(statusText, 2, chatOffset - i*15, 0.5f, ALIGN_LEFT);
		} else {
			char fullPrnt[500];
			snprintf(fullPrnt, 500, "%s said: %s", logMsgs[n].msgName, logMsgs[n].msgBody);
			char atname[21];
			snprintf(atname, 21, "@%s", config->name);
			if (strstr(logMsgs[n].msgBody, atname) != 0 && strcmp(config->name, "NOTSET") != 0 && config->showPings == true) {
				C2D_DrawRectSolid(0, chatOffset - i*15 + 1, 0, 400, 15, C2D_Color32f(0.980f, 0.964f, 0.00f, 0.5f));
			}
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
	swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, SWKBD_FILTER_DIGITS | SWKBD_FILTER_PERCENT | SWKBD_FILTER_BACKSLASH, -1);
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
}

int initSocket() {
	// allocate buffer for SOC service
	SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);

	if(SOC_buffer == NULL) {
		debugMsg("memalign: failed to allocate");\
		return 1;
	}


	// Now intialise soc:u service
	if ((socInit(SOC_buffer, SOC_BUFFERSIZE)) != 0) {
		debugMsg("Failed to allocate buffer");
    	return 1;
	}
	return 0;
}

int connectSocket(char* serverIp) {
	struct sockaddr_in server;

	if (errno == EWOULDBLOCK) {
		sendStatusMsg("Socket connecting. You can't place 2 requests at the same time. Try again soon.");
		return 4;
	}
	
	memset(&server, 0, sizeof (server));

	server.sin_family = AF_INET;
	server.sin_port = htons(80);
	server.sin_addr.s_addr = inet_addr(serverIp);
	
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	
	fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
	
	if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
		fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) & ~O_NONBLOCK);
	} else {
		return 0;
	}
	
	if (errno == EAFNOSUPPORT) {
		debugMsg("Error 47: The specified protocol family is not supported.");
		return 6;
	}
	
	for (size_t i = 0; i < 9999; i++) {
		sock_descriptor.fd = sock;
		sock_descriptor.events = POLLIN;

		int return_value = poll(&sock_descriptor, 1, 0);
		
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START) {
			close(sock);
			debugMsg("Connection aborted by user.");
			return 1;
		}
		if (return_value == -1) {
			debugMsg("Could not poll socket.");
			return 2;
		}
		if (sock_descriptor.revents & POLLIN) {
			unsigned long sockret = 0;
			recv(sock, &sockret, sizeof(sockret), 0);
			unsigned long cEsockRet = ntohl(sockret);
			if (cEsockRet == 100) {
				fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) & ~O_NONBLOCK);
				return 0;
			} else {
				char errorNum[20];
				sprintf(errorNum, "%ld", cEsockRet);
				debugMsg(errorNum);
			}
		}
	}


	close(sock);
	debugMsg("Server did not respond in time.");
	return 3;
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

void debugMsg(char* message) {
	extern struct jsonParse settings_cfg;
	if (settings_cfg.showDebugMsgs == true) {
		if (recvdMsgs < MAX_LOG) {
			strcpy(logMsgs[recvdMsgs].msgHead, "STATUS.");
			strcpy(logMsgs[recvdMsgs].msgBody, message);
			++recvdMsgs;
		} else {
			offsetAllItemsDownByOne();
			strcpy(logMsgs[MAX_LOG - 1].msgHead, "STATUS.");
			strcpy(logMsgs[MAX_LOG - 1].msgBody, message);
		}
	}
}
void sendStatusMsg(char* messageTmp, ...) {
	char message[200];
	strcpy(message, messageTmp);
	if (strstr(messageTmp, "%") != 0) {
		va_list ap;
		va_start(ap, messageTmp);
		char out[200] = {0};
		char* token;
		token = strtok(message, "%");
		while (token != NULL) {
			char* insert = va_arg(ap, char*);
			if (token == NULL) {
				break;
			}
			char prevMsg[200];
		
			strcpy(prevMsg, token);
			strcat(prevMsg, insert);
			strcat(out, prevMsg);
			token = strtok(NULL , "%");
			if (strstr(out, "%") == 0) {
				break;
			}
		}
		strcpy(message, out);
		va_end(ap);
	}
	
	if (recvdMsgs < MAX_LOG) {
		strcpy(logMsgs[recvdMsgs].msgHead, "STATUS.");
		strcpy(logMsgs[recvdMsgs].msgBody, message);
		++recvdMsgs;
	} else {
		offsetAllItemsDownByOne();
		strcpy(logMsgs[MAX_LOG - 1].msgHead, "STATUS.");
		strcpy(logMsgs[MAX_LOG - 1].msgBody, message);
	}
}


void exitSocket() {
	close(sock);
}

void offsetAllItemsDownByOne() {
	for (int i = 0; i < MAX_LOG - 1; i++) {
		strcpy(logMsgs[i].msgHead, logMsgs[i + 1].msgHead);
		strcpy(logMsgs[i].msgBody, logMsgs[i + 1].msgBody);
		strcpy(logMsgs[i].msgName, logMsgs[i + 1].msgName);
	}
}