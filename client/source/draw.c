#include <citro2d.h>
#include <time.h>
#include "draw.h"

extern C2D_TextBuf g_dynamicBuf;
C2D_Text dynText;

void text(char* yourText, int x, int y, float scale, int displType) {
	C2D_TextParse(&dynText, g_dynamicBuf, yourText);
	C2D_TextOptimize(&dynText);
	if (displType == 0) {
		C2D_DrawText(&dynText, C2D_WithColor, x, y, 0.0f, scale, scale, C2D_Color32f(0.0f, 0.0f, 0.0f, 1.0f));
	} else if (displType == 1) {
		C2D_DrawText(&dynText, C2D_WithColor | C2D_AlignCenter, x, y, 0.0f, scale, scale, C2D_Color32f(0.0f, 0.0f, 0.0f, 1.0f));
	}
	C2D_TextBufClear(g_dynamicBuf);
}

void drawImage(int image, int x, int y, float scale) {
	
}

void initSheet() {
	
}

void drawHud(char* heading, C2D_SpriteSheet spriteSheet) {
	time_t curtime;
	
	C2D_DrawRectSolid(0, 0, 0, 400, 20, C2D_Color32f(0.5f, 0.5f, 0.5f, 1.0f));
	
	text(heading, 200, 2, 0.5f, ALIGN_CENTER);
	
	//time display
	time(&curtime);
	char timeString[10];
	struct tm *timeInfo = gmtime(&curtime);
	
	int normalTmHr = timeInfo->tm_hour%12;
	if (normalTmHr == 0) {
		normalTmHr = 12;
	}

	snprintf(timeString, sizeof(timeString), "%d %02d", normalTmHr, timeInfo->tm_min);
	
	text(timeString, 380, 2, 0.5f, ALIGN_CENTER);
	
	if (timeInfo->tm_sec % 2 == 0) {
		text(":", 374, 2, 0.5f, 0);
	}
	
	//draw wifi
	switch (osGetWifiStrength()) {
		case 3:
			C2D_DrawImageAt(C2D_SpriteSheetGetImage(spriteSheet, WIFI_FULL), 2, 2, 0.0f, NULL, 1, 1);
		break;
		case 2:
			C2D_DrawImageAt(C2D_SpriteSheetGetImage(spriteSheet, WIFI_GOOD), 2, 2, 0.0f, NULL, 1, 1);
		break;
		case 1:
			C2D_DrawImageAt(C2D_SpriteSheetGetImage(spriteSheet, WIFI_OKAY), 2, 2, 0.0f, NULL, 1, 1);
		break;
		case 0:
			C2D_DrawImageAt(C2D_SpriteSheetGetImage(spriteSheet, NO_WIFI), 2, 2, 0.0f, NULL, 1, 1);
		break;
	}
}