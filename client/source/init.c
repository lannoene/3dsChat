#include <citro2d.h>
#include <citro3d.h>

void citroInit(void) {
	// Init libs
	romfsInit();
	
	//citro2d init vars
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();
	
}