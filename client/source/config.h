#pragma once

#define DOES_NOT_EXIST -1
#define MAX_SETTINGS 3
#include <jansson.h>
#include <stdbool.h>

struct jsonParse {
	char name[20];
	char favServer[15];
};

int writeSettings();
struct jsonParse checkSettings();
void parseEntries(json_t* entries_elem);
void saveJson(struct jsonParse *config);
void drawSettings();
void moveCfgPointer(int direction);
void performCfgAction(struct jsonParse *config);