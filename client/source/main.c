#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>

#include <fcntl.h>

#include <sys/types.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>

#include <3ds.h>

#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000

#define DEFAULT_BUFLEN 512
#define STACKSIZE (4 * 1024)

static u32 *SOC_buffer = NULL;
s32 sock = -1, csock = -1;

__attribute__((format(printf,1,2)))
void failExit(const char *fmt, ...);

void socShutdown() {
	printf("waiting for socExit...\n");
	send(sock, "EXIT.", strlen("EXIT."), 0);
	
	close(sock);
	socExit();
}

int main() {
	//software keyboard vars
	static SwkbdState swkbd;
	
	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);
	
	printf ("\n3dsChat client alpha 1\n");

	int ret;

	struct sockaddr_in client;
	struct sockaddr_in server;
	
	
	// allocate buffer for SOC service
	SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);

	if(SOC_buffer == NULL) {
		failExit("memalign: failed to allocate\n");
	}

	// Now intialise soc:u service
	if ((ret = socInit(SOC_buffer, SOC_BUFFERSIZE)) != 0) {
    	failExit("socInit: 0x%08X\n", (unsigned int)ret);
	}

	// register socShutdown to run at exit
	// atexit functions execute in reverse order so this runs before gfxExit
	atexit(socShutdown);

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

	if (sock < 0) {
		failExit("socket: %d %s\n", errno, strerror(errno));
	}

	memset(&server, 0, sizeof (server));
	memset(&client, 0, sizeof (client));

	char serverIp[50];

	swkbdInit(&swkbd, SWKBD_TYPE_WESTERN, 1, -1);
	swkbdSetFeatures(&swkbd, SWKBD_MULTILINE);
	swkbdSetHintText(&swkbd, "Server ip");
	swkbdInputText(&swkbd, serverIp, sizeof(serverIp));

	server.sin_family = AF_INET;
	server.sin_port = htons(80);
	server.sin_addr.s_addr = inet_addr(serverIp);

	printf("%s connecting...\n", serverIp);
	//Connect to remote server
	if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
		puts("connect error");
		return 1;
	}
	
	printf("connected! press A to chat\n");

	// register gfxExit to be run when app quits
	// this can help simplify error handling
	atexit(gfxExit);

	while (aptMainLoop()) {
		
		gspWaitForVBlank();
		hidScanInput();
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START) {
			printf("Both the client and server must exit. Sorry about that!\n");
			break;
		}
		
		if (kDown & KEY_A) {
			char sendMsg[100];
			swkbdInit(&swkbd, SWKBD_TYPE_WESTERN, 1, -1);
			swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, SWKBD_FILTER_DIGITS | SWKBD_FILTER_AT | SWKBD_FILTER_PERCENT | SWKBD_FILTER_BACKSLASH | SWKBD_FILTER_PROFANITY, 2);
			swkbdSetFeatures(&swkbd, SWKBD_MULTILINE);
			swkbdSetHintText(&swkbd, "Send message!");
			swkbdInputText(&swkbd, sendMsg, sizeof(sendMsg));
			
			if (send(sock, sendMsg, strlen(sendMsg), 0) < 0) {
				printf("Send failed (ON SOCKET SEND FUNC).");
				return 1;
			}
			printf("Sent: %s\n", sendMsg);
		}
		
		int iResult;
	
		char recvbuf[DEFAULT_BUFLEN];
		int recvbuflen = DEFAULT_BUFLEN;
		memset(recvbuf, '\0', sizeof(char)*DEFAULT_BUFLEN);
	
		
		//printf("wating for message...\n");
		
		gspWaitForVBlank();
		struct pollfd sock_descriptor;
		sock_descriptor.fd = sock;
		sock_descriptor.events = POLLIN;

		int return_value = poll(&sock_descriptor, 1, 0);

		if (return_value == -1) {
			printf("%s", strerror(errno));
		} else if (return_value == 0) {
			//printf("No data available to be read");
		} else {
			if (sock_descriptor.revents & POLLIN) {
				iResult = recv(sock, recvbuf, recvbuflen, 0);
			}
		}
        if (iResult > 0) {
            printf("Bytes received: %d\n", iResult);
			printf("They said: %s\n", recvbuf);
			memset(recvbuf, '\0', sizeof(char)*100);
		} else if (iResult == 0) {
			
		}
		iResult = 0;
	}
	return 0;
}

void failExit(const char *fmt, ...) {

	if(sock>0) close(sock);
	if(csock>0) close(csock);

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