#pragma once

#define SEND_SIZE 512
#define HEADER_SIZE 32
#define NAME_SIZE 20
#define BODY_SIZE SEND_SIZE - HEADER_SIZE - NAME_SIZE
#define MAX_LOG 20

#define MAX_USERS 50
#define MAX_NAME 20

#include <jansson.h>
#include <stdarg.h>

#include "config.h"

void resetMsgVars(bool resetRecv, bool resetSend);
void recvChat(void);
void sendMsgSocket(struct jsonParse *config);
int initSocket(void);
int connectSocket(char* serverIp);
void moveChat(int way);
void displayChat(struct jsonParse *config);
void serverSend(char* header, char* body);
void sendStatusMsg(char* message, ...);
void exitSocket();
void debugMsg(char* message);
void offsetAllItemsDownByOne(void);
void recvUserList(void);
void displayUserList(void);
void resetChatConsole(void);
void sendCurrentUserInfo(struct jsonParse *config);