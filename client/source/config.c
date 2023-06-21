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
	RESET_SETTINGS
};

struct stat st = {0};

static struct jsonParse settings_config_tmp;

char settingsLabels[MAX_SETTINGS][50] = {"Username", "Favorite Server (quick connect by pressing 'Y')", "Reset Settings"};

int writeSettings() {
	if (stat("3dsChat", &st)<0) {
		mkdir("3dsChat", 0700);
	}
	
	if (stat("3dsChat/settings.json", &st) == DOES_NOT_EXIST) {
		json_t* root = json_object();
		json_t* settings_obj = json_object();
	
		json_object_set_new(root, "Note", json_string("if ur reading this ur really cool 😳"));
		json_object_set_new(settings_obj, "name", json_string("NOTSET"));
		json_object_set_new(settings_obj, "fav_server", json_string("NOTSET"));
		json_object_set_new(root, "settings", settings_obj);
	
		//json_object_set_new(root, ENTRIES_STRING, pls_json_arr);
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
	json_object_set_new(root, "settings", settings_obj);
	
	//json_object_set_new(root, ENTRIES_STRING, pls_json_arr);
	json_dump_file(root, "3dsChat/settings.json", JSON_INDENT(8));
	json_decref(root);
}

//user's settings selecter pointer thing ">"
int userPointer = 0;

void drawSettings() {
	for (int i = 0; i < MAX_SETTINGS; i++) {
		char changedLabl[51];
		if (i == userPointer) {
			snprintf(changedLabl, sizeof(settingsLabels) + 1, "%s%s", ">", settingsLabels[i]);//truncate detector in compiler doens't work correctly with multidementional arrays
			text(changedLabl, 0, i*15, 0.5f, ALIGN_LEFT);
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
			swkbdInputText(&swkbd, config->name, sizeof(config->name));
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
			swkbdInputText(&swkbd, config->favServer, sizeof(config->favServer));
			if (strcmp(config->favServer, "") == 0) {
				strcpy(config->favServer, "NOTSET");
			}
		break;
		case RESET_SETTINGS:
			resetSettings(config);
		break;
	}
}