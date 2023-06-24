#pragma once

#define SEND_SIZE 512
#define HEADER_SIZE 32
#define NAME_SIZE 20
#define BODY_SIZE SEND_SIZE - HEADER_SIZE - NAME_SIZE
#define MAX_LOG 250

#include <jansson.h>
#include <stdarg.h>

#include "config.h"



void resetMsgVars(bool resetRecv, bool resetSend);
void recvChat();
void sendMsgSocket();
int initSocket();
int connectSocket();
void moveChat(int way);
void displayChat(struct jsonParse *config);
void serverSend(char* header, char* body);
void sendStatusMsg(char* message, ...);
void exitSocket();
void debugMsg(char* message);