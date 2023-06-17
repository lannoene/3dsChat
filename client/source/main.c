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

#include <3ds.h>

#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000

#define DEFAULT_BUFLEN 512
#define STACKSIZE (4 * 1024)

static u32 *SOC_buffer = NULL;
s32 sock = -1, csock = -1;

__attribute__((format(printf,1,2)))
void failExit(const char *fmt, ...);

const static char http_200[] = "HTTP/1.1 200 OK\r\n";

const static char indexdata[] = "<html> \
                               <head><title>A test page</title></head> \
                               <body> \
                               This small test page has had %d hits. \
                               </body> \
                               </html>";

const static char http_html_hdr[] = "Content-type: text/html\r\n\r\n";
const static char http_get_index[] = "GET / HTTP/1.1\r\n";


void socShutdown() {
	printf("waiting for socExit...\n");
	send(sock, "EXIT.", strlen("EXIT."), 0);
	
	close(sock);
	socExit();
}

bool exitThread = false;

void socketsListen() {
	
}

void socketsWrite() {
	while (!exitThread) {
		hidScanInput();
		
		u32 kDown = hidKeysDown();
		if (kDown & KEY_A) {
			if (send(sock, "Hey server!", strlen("Hey server!"), 0) < 0) {
				printf("Send failed (ON SOCKET SEND FUNC).");
				return;
			}
		}
	}
}

int main(int argc, char **argv) {
	
	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);
	
	printf ("\n3dsChat client alpha 0\n");

	int ret;

	u32	clientlen;
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

	// libctru provides BSD sockets so most code from here is standard
	clientlen = sizeof(client);

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

	if (sock < 0) {
		failExit("socket: %d %s\n", errno, strerror(errno));
	}

	memset (&server, 0, sizeof (server));
	memset (&client, 0, sizeof (client));

	server.sin_family = AF_INET;
	server.sin_port = htons(80);
	server.sin_addr.s_addr = inet_addr("192.168.1.253");

	// Set socket non blocking so we can still read input to exit
	//fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
	printf("connecting...\n");
	//Connect to remote server
	if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
		puts("connect error");
		return;
	}


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
		
		int iResult;
		char* successMsg = "WAITING.";
	
		char recvbuf[DEFAULT_BUFLEN];
		int recvbuflen = DEFAULT_BUFLEN;
		memset(recvbuf, '\0', sizeof(char)*DEFAULT_BUFLEN);
	
		if (send(sock, successMsg, strlen(successMsg), 0) < 0) {
			printf("Send failed. You may exit the program. (ON SOCKET RECV FUNC)");
			return;
		}
		printf("wating for message...\n");
		
		gspWaitForVBlank();
		
        iResult = recv(sock, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            printf("Bytes received: %d\n", iResult);
			printf("%s\n", recvbuf);
			memset(recvbuf, '\0', sizeof(char)*100);
		} else if (iResult == 0) {
			
		}
	}
	
	exitThread = true;
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