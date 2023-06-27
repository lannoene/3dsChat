#include "config.h"
#include "draw.h"

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <3ds.h>

enum settingsActions {
	CHANGE_NAME = 0,
	CHANGE_FAV_SERVER,
	RESET_SETTINGS,
	SHOW_PINGS,
	DEBUG_MESSAGES,
	CLEAR_CHAT_WITH_NEW_CONN
};

struct stat st = {0};

static struct jsonParse settings_config_tmp;

char settingsLabels[MAX_SETTINGS][50] = {"Username", "Favorite Server (quick connect by pressing 'Y')", "Reset Settings", "Show pings (@username)", "Show debug messages (for testers)", "Clear chat when you connect to a new server"};

int writeDefaultSettings() {
	if (stat("3dsChat", &st)<0) {
		mkdir("3dsChat", 0700);
	}
	
	if (stat("3dsChat/settings.json", &st) == DOES_NOT_EXIST) {
		json_t* root = json_object();
		json_t* settings_obj = json_object();
		
		json_object_set_new(root, "Note", json_string("if ur reading this ur really cool 😳"));
		json_object_set_new(settings_obj, "name", json_string("NOTSET"));
		json_object_set_new(settings_obj, "fav_server", json_string("NOTSET"));
		json_object_set_new(settings_obj, "show_pings", json_boolean(true));
		json_object_set_new(settings_obj, "show_debug_messages", json_boolean(false));
		json_object_set_new(settings_obj, "clear_chat_when_conn", json_boolean(false));
		json_object_set_new(root, "settings", settings_obj);
	
		json_dump_file(root, "3dsChat/settings.json", JSON_INDENT(8));
		json_decref(root);
	}
	
	return 0;
}

struct jsonParse checkSettings() {
	json_error_t* pjsonError = NULL;
	json_t* pJson = json_load_file("3dsChat/settings.json", 0, pjsonError);
	
	if (!pJson) {
		
	}
	
	const char* key;
	json_t* value;
	
	json_object_foreach(pJson, key, value) {//this works
		if (strcmp(key, "settings") == 0) {
			parseEntries(value);
		}
	}
	json_decref(pJson);
	
	return settings_config_tmp;
}

void parseEntries(json_t* entries_elem) {
	const char* key;
	json_t* value;
	
	void *iter = json_object_iter(entries_elem);
	while (iter) {
		key = json_object_iter_key(iter);
		value = json_object_iter_value(iter);
		
		if (strcmp(key, "name") == 0) {
			if (json_is_string(value)) {
				strcpy(settings_config_tmp.name, json_string_value(value));
			}
		} else if (strcmp(key, "fav_server") == 0) {
			if (json_is_string(value)) {
				strcpy(settings_config_tmp.favServer, json_string_value(value));
			}
		} else if (strcmp(key, "show_pings") == 0) {
			if (json_is_boolean(value)) {
				settings_config_tmp.showPings = json_boolean_value(value);
			}
		} else if (strcmp(key, "show_debug_messages") == 0) {
			if (json_is_boolean(value)) {
				settings_config_tmp.showDebugMsgs = json_boolean_value(value);
			}
		} else if (strcmp(key, "clear_chat_when_conn") == 0) {
			if (json_is_boolean(value)) {
				settings_config_tmp.clearChatWhenConn = json_boolean_value(value);
			}
		}
		
		iter = json_object_iter_next(entries_elem, iter);
	}
}

void saveJson(struct jsonParse *config) {
	json_t* root = json_object();
	json_t* settings_obj = json_object();
	
	json_object_set_new(root, "Note", json_string("if ur reading this ur really cool 😳"));
	json_object_set_new(settings_obj, "name", json_string(config->name));
	json_object_set_new(settings_obj, "fav_server", json_string(config->favServer));
	json_object_set_new(settings_obj, "show_pings", json_boolean(config->showPings));
	json_object_set_new(settings_obj, "show_debug_messages", json_boolean(config->showDebugMsgs));
	json_object_set_new(settings_obj, "clear_chat_when_conn", json_boolean(config->clearChatWhenConn));
	json_object_set_new(root, "settings", settings_obj);
	
	//json_object_set_new(root, ENTRIES_STRING, pls_json_arr);
	json_dump_file(root, "3dsChat/settings.json", JSON_INDENT(8));
	json_decref(root);
}

//user's settings selecter pointer thing ">"
int userPointer = 0;

void drawSettings(struct jsonParse *config) {
	C2D_DrawRectSolid(0, userPointer*15 + 1, 0, 400, 15, C2D_Color32f(0.656f, 0.707f, 0.810f, 1.0f));
	for (int i = 0; i < MAX_SETTINGS; i++) {
		char boolLabel[55];
		if (i == SHOW_PINGS) {
			if (config->showPings == true) {
				snprintf(boolLabel, sizeof(settingsLabels) + 1, "%s [on]", settingsLabels[i]);
			} else {
				snprintf(boolLabel, sizeof(settingsLabels) + 1, "%s [off]", settingsLabels[i]);
			}
			
			text(boolLabel, 9, i*15, 0.5f, ALIGN_LEFT);
		} else if (i == DEBUG_MESSAGES) {
			if (config->showDebugMsgs == true) {
				snprintf(boolLabel, sizeof(settingsLabels) + 1, "%s [on]", settingsLabels[i]);
			} else {
				snprintf(boolLabel, sizeof(settingsLabels) + 1, "%s [off]", settingsLabels[i]);
			}
			text(boolLabel, 9, i*15, 0.5f, ALIGN_LEFT);
		} else if (i == CLEAR_CHAT_WITH_NEW_CONN) {
			if (config->clearChatWhenConn == true) {
				snprintf(boolLabel, sizeof(settingsLabels) + 1, "%s [on]", settingsLabels[i]);
			} else {
				snprintf(boolLabel, sizeof(settingsLabels) + 1, "%s [off]", settingsLabels[i]);
			}
			text(boolLabel, 9, i*15, 0.5f, ALIGN_LEFT);
		} else {
			text(settingsLabels[i], 9, i*15, 0.5f, ALIGN_LEFT);
		}
	}
}

void moveCfgPointer(int direction) {
	if (direction == 0 && userPointer > 0) {
		--userPointer;
	} else if (direction == 1 && userPointer < MAX_SETTINGS - 1) {
		++userPointer;
	}
}

void resetSettings(struct jsonParse *config) {
	strcpy(config->favServer, "NOTSET");
	strcpy(config->name, "NOTSET");
	config->showPings = true;
	config->showDebugMsgs = false;
	config->clearChatWhenConn = false;
}

void truncateTrailingSpaces(struct jsonParse *config) {
	while ((strstr(config->name, " \0") != 0)) {
		char* retPointer = strstr(config->name, " \0");
		strcpy(retPointer, "\0");
	}
}

void performCfgAction(struct jsonParse *config) {
	switch (userPointer) {
		case CHANGE_NAME:
			extern SwkbdState swkbd;
			swkbdInit(&swkbd, SWKBD_TYPE_WESTERN, 1, -1);
			swkbdSetFeatures(&swkbd, SWKBD_MULTILINE);
			swkbdSetHintText(&swkbd, "Your username");
			if (strcmp(config->name, "NOTSET") != 0) {
				swkbdSetInitialText(&swkbd, config->name);
			}
			swkbdInputText(&swkbd, config->name, 20);
			truncateTrailingSpaces(config);
			if (strcmp(config->name, "") == 0) {
				strcpy(config->name, "NOTSET");
			}
		break;
		case CHANGE_FAV_SERVER:
			extern SwkbdState swkbd;
			swkbdInit(&swkbd, SWKBD_TYPE_WESTERN, 1, -1);
			swkbdSetFeatures(&swkbd, SWKBD_MULTILINE);
			swkbdSetHintText(&swkbd, "Favorite Server");
			if (strcmp(config->favServer, "NOTSET") != 0) {
				swkbdSetInitialText(&swkbd, config->favServer);
			}
			swkbdInputText(&swkbd, config->favServer, 16);
			if (strcmp(config->favServer, "") == 0) {
				strcpy(config->favServer, "NOTSET");
			}
		break;
		case RESET_SETTINGS:
			resetSettings(config);
		break;
		case SHOW_PINGS:
			config->showPings = !config->showPings;
		break;
		case DEBUG_MESSAGES:
			config->showDebugMsgs = !config->showDebugMsgs;
		break;
		case CLEAR_CHAT_WITH_NEW_CONN:
			config->clearChatWhenConn = !config->clearChatWhenConn;
		break;
	}
}
