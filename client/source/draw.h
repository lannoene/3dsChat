#pragma once

#include <citro2d.h>

#define APP_ICON 0
#define SETTINGS_ICON 1

#define WIFI_FULL	2
#define WIFI_GOOD	3
#define WIFI_OKAY	4
#define NO_WIFI		5

#define ALIGN_LEFT 0
#define ALIGN_CENTER 1

void text(char* yourText, int x, int y, float scale, int displType);
void drawImage(int image, int x, int y, float scale);
void initSheet(void);
void drawHud(char* heading, C2D_SpriteSheet spriteSheet);
int drawOptionsMenu(void);