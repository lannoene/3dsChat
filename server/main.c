#include <stdio.h>
#include <winsock2.h>
#include <stdbool.h>
#include <ws2tcpip.h>
#include <string.h>
#include <conio.h>

#define DEFAULT_PORT "80"
#define DEFAULT_BUFLEN 512

struct addrinfo *result = NULL, *ptr = NULL, hints;

int main() {
	printf("3dsChat SERVER alpha 1");
	
	//create wsadata
	WSADATA wsaData;
	
	int iResult;
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	} else {
		printf("WSAStartup succeeded.\n");
	}
	//create socket info
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);

	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	} else {
		printf("getaddrinfo succeeded\n");
	}
	
	SOCKET ListenSocket = INVALID_SOCKET;
	
	// Create a SOCKET for the server to listen for client connections
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	
	if (ListenSocket == INVALID_SOCKET) {
		printf("Error at socket(): %d\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	} else {
		printf("Socket succeeded\n");
	}
	
	// Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
    } else {
		printf("listening\n");
	}
	
	char str[100];
	ZeroMemory(str, sizeof(str));
	
	struct in_addr addr = { 0, };
	struct hostent * res;
	int i = 0;
	
	if (gethostname(str, sizeof(str)) == SOCKET_ERROR) {
		printf("error getting hostname %d",  WSAGetLastError());
	}

	res = gethostbyname(str);
	while (res->h_addr_list[i] != 0) {
		addr.s_addr = *(u_long *)res->h_addr_list[i++];
		printf("Server address: %s\n", inet_ntoa(addr));
	}
	
	freeaddrinfo(result);
	
	printf("Press any key to chat.\n");
	
	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		printf("Listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	
	SOCKET ClientSocket = INVALID_SOCKET;
	
	char recvbuf[DEFAULT_BUFLEN];
	int iSendResult;
	int recvbuflen = DEFAULT_BUFLEN;
	
	// Accept a client socket
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	} else {
		printf("New client accepted\n");
	}
	


	
	// Receive until the peer shuts down the connection
	while (true) {
		//create wsapoll data
		WSAPOLLFD fdarray = {0};
		fdarray.fd = ClientSocket;
		fdarray.events = POLLRDNORM;
		
		//printf("Starting loop over again.");
		memset(recvbuf, '\0', sizeof(char)*DEFAULT_BUFLEN);
		
		int ret = WSAPoll(&fdarray, 1, 0);

		if (ret == SOCKET_ERROR) {
			printf("POLL ERR\n");
		} else {
			if (fdarray.revents & POLLRDNORM) {
				recv(ClientSocket, recvbuf, recvbuflen, 0);
				if (strstr(recvbuf, "EXIT.") != 0) {
					return 0;
				}
				printf("They said: %s", recvbuf);
			}
		}
		
		//printf("Got past recv");
		
		char sendMsg[100] = {0};
		
		if (kbhit()) {
			char c = getch();
            printf("%c\n", c);
			printf("Your message to send to them: ");
			fgets(sendMsg, 100, stdin);
			//Send data to user
			iSendResult = send(ClientSocket, sendMsg, strlen(sendMsg), 0);
			if (iSendResult == SOCKET_ERROR) {
				printf("send failed: %d\n", WSAGetLastError());
				closesocket(ClientSocket);
				WSACleanup();
				return 1;
			}
			printf("Bytes sent: %d\n", iSendResult);
		}
		
		if (iResult > 0) {
			printf("Bytes received: %d: %s\n", iResult, recvbuf);

			//printf("%s\n", recvbuf);

			
		} else if (iResult == 0) {
			//printf("Listen request timed out\n");
		} else {
			printf("recv failed: %d\n", WSAGetLastError());
		}
		iResult = 0;
	}
	printf("exiting...");
	closesocket(ClientSocket);
	
	return 0;
}