#pragma once

#define DOES_NOT_EXIST -1
#define MAX_SETTINGS 6
#include <jansson.h>
#include <stdbool.h>
#include <stdbool.h>

struct jsonParse {
	char name[20];
	char favServer[16];
	bool showPings;
	bool showDebugMsgs;
	bool clearChatWhenConn;
};

int writeDefaultSettings();
struct jsonParse checkSettings();
void parseEntries(json_t* entries_elem);
void saveJson(struct jsonParse *config);
void drawSettings(struct jsonParse *config);
void moveCfgPointer(int direction);
void performCfgAction(struct jsonParse *config);
void removeNameSpaces(struct jsonParse *config);