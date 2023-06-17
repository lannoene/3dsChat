#include <stdio.h>
#include <winsock2.h>
#include <stdbool.h>
#include <ws2tcpip.h>
#include <string.h>

#define DEFAULT_PORT "80"
#define DEFAULT_BUFLEN 512

const static char http_200[] = "HTTP/1.1 200 OK\r\n";

const static char indexdata[] = "Test sockets page";

const static char http_html_hdr[] = "Content-type: text/html\r\n\r\n";
const static char http_get_index[] = "GET / HTTP/1.1\r\n";

struct addrinfo *result = NULL, *ptr = NULL, hints;

int main() {
	printf("3dsChat SERVER alpha 0");
	
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
		printf("Error at socket(): %ld\n", WSAGetLastError());
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
	
	char str[10];
	ZeroMemory(str, sizeof(str));
	
	printf("Point your browser to: %s\n", "http://192.168.1.253/");
	
	freeaddrinfo(result);
	
	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		printf("Listen failed with error: %ld\n", WSAGetLastError());
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
		memset(recvbuf, '\0', sizeof(char)*DEFAULT_BUFLEN);
		
		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		char sendMsg[100];
		printf("Your message to send to them: ");
		fgets(sendMsg, 100, stdin);
		if (iResult > 0) {
			printf("Bytes received: %d: %s\n", iResult, recvbuf);

			//printf("%s\n", recvbuf);

			//Send data to user
			iSendResult = send(ClientSocket, sendMsg, strlen(sendMsg), 0);
			if (iSendResult == SOCKET_ERROR) {
				printf("send failed: %d\n", WSAGetLastError());
				closesocket(ClientSocket);
				WSACleanup();
				return 1;
			}
			printf("Bytes sent: %d\n", iSendResult);
		} else if (iResult == 0)
			printf("Listen request timed out\n");
		else {
			printf("recv failed: %d\n", WSAGetLastError());
		}
	}
	printf("exiting...");
	closesocket(ClientSocket);
	
	return 0;
}