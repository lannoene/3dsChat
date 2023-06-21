#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/stat.h>

#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

#include <3ds.h>
#include <citro2d.h>

#include "draw.h"
#include "init.h"
#include "chat.h"
#include "config.h"

//s32 sock = -1;

__attribute__((format(printf,1,2)))
void failExit(const char *fmt, ...);

C2D_TextBuf g_dynamicBuf;
C3D_RenderTarget* top;
C3D_RenderTarget* bot;

enum isOnnum {
	TITLESCREEN = 0,
	CHAT,
	SETTINGS
};

struct jsonParse settings_cfg;

int main() {
	gfxInitDefault();
	citroInit();
	
	if (chdir("sdmc:/3ds") != 0) {
		if (!mkdir("sdmc:/3ds", 0700)) {
			consoleInit(GFX_TOP, NULL);
			printf("Could not create or move into /3ds dir.\nPlease set sd card to non-write protected or do smth else idk\nApp will exit in 7 seconds.");
			sleep(7);
			return 1;
		} else {
			chdir("sdmc:/3ds");
		}
	}
	
	g_dynamicBuf = C2D_TextBufNew(4096); // support up to 4096 glyphs in the buffer
	
	top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
	
	settings_cfg = checkSettings();
	//force set settings set to false
	writeSettings();
	//memset(logMsgs, 0, sizeof(logMsgs));
	
	C2D_SpriteSheet spriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/sprites.t3x");
	if (!spriteSheet) svcBreak(USERBREAK_PANIC);
	bool hasConn = false;
	int isOn = TITLESCREEN;
	while (aptMainLoop()) {
		
		gspWaitForVBlank();
		hidScanInput();
		
		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
		
		u32 kDown = hidKeysDown();
		
		if (kDown & KEY_START) {
			printf("Both the client and server must exit. Sorry about that!\n");
			break;
		}
		if (isOn == TITLESCREEN) {
			C2D_TargetClear(top, C2D_Color32f(1.0f, 1.0f, 1.0f, 1.0f));
			C2D_SceneBegin(top);
			
			text("3dsChat Client v1.3", 200, 90, 0.5f, ALIGN_CENTER);
			text("Press 'a' to begin", 200, 110, 0.5f, ALIGN_CENTER);
			
			C2D_TargetClear(bot, C2D_Color32f(1.0f, 1.0f, 1.0f, 1.0f));
			C2D_SceneBegin(bot);
			
			
			if (kDown & KEY_A) {
				isOn = CHAT;
			}
		} else if (isOn == CHAT) {
			//render top screen
			C2D_TargetClear(top, C2D_Color32f(1.0f, 1.0f, 1.0f, 1.0f));
			C2D_SceneBegin(top);
			if (hasConn == true) {
				if (kDown & KEY_A) {
					sendMsgSocket(&settings_cfg);
				}
			
				if (kDown & KEY_UP) {
					moveChat(0);
				} else if (kDown & KEY_DOWN) {
					moveChat(1);
				}
		
				recvChat();
				displayChat();
			} else {
				text("Connect to a server by pressing 'x' and typing in the server ip", 200, 100, 0.5f, ALIGN_CENTER);
				text("Edit settings by pressing 'select'", 200, 115, 0.5f, ALIGN_CENTER);
				text("Quick connect by pressing 'Y'", 200, 130, 0.5f, ALIGN_CENTER);
				displayChat();
			}
			
			if (kDown & KEY_X && hasConn == false) {
				char serverIp[50];
				extern SwkbdState swkbd;//todo: find out why i'm doing this
				swkbdInit(&swkbd, SWKBD_TYPE_WESTERN, 1, -1);
				swkbdSetFeatures(&swkbd, SWKBD_MULTILINE);
				swkbdSetHintText(&swkbd, "Server ip");
				swkbdInputText(&swkbd, serverIp, sizeof(serverIp));
				//sendStatusMsg("Connecting...");
		
				if (initSockets(serverIp) != 0) {
					sendStatusMsg("Could not connect.");
				} else {
					sendStatusMsg("Successfully connected. Press 'a' to chat!");
					hasConn = true;
				}
			} else if (kDown & KEY_SELECT) {
				isOn = SETTINGS;
				extern int userPointer;
				userPointer = 0;
			} else if (kDown & KEY_Y && hasConn == false) {
				//sendStatusMsg("Connecting...");
				if (strcmp(settings_cfg.favServer, "NOTSET") == 0) {
					sendStatusMsg("You need to set a favorite server!");
				} else {
					if (initSockets(settings_cfg.favServer) != 0) {
						sendStatusMsg("Could not connect.");
					} else {
						sendStatusMsg("Successfully connected. Press 'a' to chat!");
						hasConn = true;
					}
				}
			}
			
			drawHud("Chat", spriteSheet);
			
			//render bottom screen
			
			C2D_TargetClear(bot, C2D_Color32f(1.0f, 1.0f, 1.0f, 1.0f));
			C2D_SceneBegin(bot);
		
			text("3dsChat Client Alpha v1.3", 0, 0, 0.5f, ALIGN_LEFT);
		
			C2D_DrawImageAt(C2D_SpriteSheetGetImage(spriteSheet, APP_ICON), 90, 40, 0.0f, NULL, 2, 2);
		} else if (isOn == SETTINGS) {
			//render top screen
			C2D_TargetClear(top, C2D_Color32f(1.0f, 1.0f, 1.0f, 1.0f));
			C2D_SceneBegin(top);
			
			drawHud("Settings", spriteSheet);
			
			C2D_DrawImageAt(C2D_SpriteSheetGetImage(spriteSheet, SETTINGS_ICON), 130, 50, 0.0f, NULL, 2, 2);
			
			C2D_TargetClear(bot, C2D_Color32f(1.0f, 1.0f, 1.0f, 1.0f));
			C2D_SceneBegin(bot);
			
			drawSettings();
			
			if (kDown & KEY_UP) {
				moveCfgPointer(0);
			} else if (kDown & KEY_DOWN) {
				moveCfgPointer(1);
			}
			/*
			char your_age[10];
			char settingName[50];
			itoa(settings_cfg.age, your_age, 10);
			snprintf(settingName, sizeof(settingName), "Your name: %s", settings_cfg.name);
			if (strcmp(settings_cfg.name, "NOTSET") != 0) {
				text(settingName, 0, 0, 0.5f, ALIGN_LEFT);
			} else {
				text("You have not set a name.", 0, 0, 0.5f, ALIGN_LEFT);
			}*/
			if (kDown & KEY_A) {
				performCfgAction(&settings_cfg);
			} else if (kDown & KEY_SELECT) {
				saveJson(&settings_cfg);
				isOn = CHAT;
			}
		}
		C3D_FrameEnd(0);
	}
	
	saveJson(&settings_cfg);
	// Delete graphics
	C2D_SpriteSheetFree(spriteSheet);
	
	// Deinit libs
	C2D_Fini();
	C3D_Fini();
	gfxExit();
	
	serverSend("EXIT.", "");
	exitSocket();
	socExit();
	return 0;
}

void failExit(const char *fmt, ...) {

	//if(sock>0) close(sock);

	va_list ap;

	printf(CONSOLE_RED);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf(CONSOLE_RESET);
	printf("\nPress B to exit\n");

	while (aptMainLoop()) {
		gspWaitForVBlank();
		hidScanInput();

		u32 kDown = hidKeysDown();
		if (kDown & KEY_B) exit(0);
	}
}