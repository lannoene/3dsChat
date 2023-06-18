#include <stdio.h>
#include <winsock2.h>
#include <stdbool.h>
#include <ws2tcpip.h>
#include <string.h>
#include <conio.h>

#define DEFAULT_PORT "80"
#define DEFAULT_BUFLEN 512

struct addrinfo *result = NULL, *ptr = NULL, hints;
int num_sockets;
int connected_sockets = 0;
SOCKET ClientSockets[10];
SOCKET ListenSocket = INVALID_SOCKET;

int connectSock() {
	int iResult;
	
	//create wsadata
	WSADATA wsaData;
	
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
	
	printf("---listening for socket #%d---\n", connected_sockets);
	ListenSocket = INVALID_SOCKET;
	
	bool optval = true;
	
	// Create a SOCKET for the server to listen for client connections
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	
	//we want to reuse the same address because we only have one address
	iResult = setsockopt(ListenSocket, SOL_SOCKET, SO_REUSEADDR, (char *) &optval, sizeof(optval));
		
	if (ListenSocket == INVALID_SOCKET) {
		printf("Error at socket(): %d\n", WSAGetLastError());
		WSACleanup();
		return 1;
	} else {
		printf("Socket succeeded\n");
	}
	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	} else {
		printf("listening\n");
	}
		
	char str[100];
	ZeroMemory(str, sizeof(str));
		
	if (gethostname(str, sizeof(str)) == SOCKET_ERROR) {
		printf("error getting hostname %d",  WSAGetLastError());
	}
	
	
	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		printf("Listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	
	return 0;
}

int main() {

	int iResult;
	
	printf("3dsChat SERVER alpha v1.2\n");
	//printf("How many clients? (For now, you need to fill the server up before chatting)\n");
	//scanf("%d", &num_sockets);
	
	//connect a new socket command
	connectSock();
	
	//get server ip
	struct hostent *thisHost;
	thisHost = gethostbyname("");
	printf("Server address: %s\n", inet_ntoa(*(struct in_addr *) *thisHost->h_addr_list));
	
	
	
	char recvbuf[DEFAULT_BUFLEN];
	int iSendResult;
	int recvbuflen = DEFAULT_BUFLEN;

	printf("Press any key to chat.\n");
	
	// Receive until the peer shuts down the connection
	while (true) {

		// Declare and initialize variables
		WSAEVENT NewEvent[1];


		// Create new event
		NewEvent[0] = WSACreateEvent();
	
		// with the listening socket and NewEvent
		WSAEventSelect(ListenSocket, NewEvent[0], FD_ACCEPT);
		//deal with incoming clients
		//no documentation... thank god for this single post pointing me in the right direction: https://stackoverflow.com/a/72793077
		iResult = WSAWaitForMultipleEvents(1, NewEvent, FALSE, 0, FALSE);
		//puts("waiting..");
		if (iResult == WSA_WAIT_EVENT_0) {
			// Accept a client socket
			ClientSockets[connected_sockets] = accept(ListenSocket, NULL, NULL);
			if (ClientSockets[connected_sockets] == INVALID_SOCKET) {
				printf("accept failed: %d\n", WSAGetLastError());
				closesocket(ListenSocket);
				WSACleanup();
				return 1;
			} else {
				printf("New client #%d accepted\n", connected_sockets);
			}
			puts("closed listen sock");
			closesocket(ListenSocket);
			++connected_sockets;
			connectSock();
		}
		
		iResult = 0;
		
		//deal with messages
		char sendMsg[100] = {0};
		char newbuf[100] = {0};
			
		if (kbhit()) {
			char c = getch();
			printf("%c\n", c);
			printf("Your message to send to them: ");
			fgets(sendMsg, 100, stdin);
			//Send data to user(s)
			for (int f = 0; f < connected_sockets; f++) {
				snprintf(newbuf, 100, "Server: %s", sendMsg);
				iSendResult = send(ClientSockets[f], newbuf, strlen(newbuf), 0);
				if (iSendResult == SOCKET_ERROR) {
					printf("send failed: %d\n", WSAGetLastError());
					closesocket(ClientSockets[f]);
					WSACleanup();
					return 1;
				}
				printf("Bytes sent: %d\n", iSendResult);
			}
		}
		
		for (int f = 0; f < connected_sockets; f++) {
			//create wsapoll data
			WSAPOLLFD fdarray = {0};
			fdarray.fd = ClientSockets[f];
			fdarray.events = POLLRDNORM;
			
			memset(recvbuf, '\0', sizeof(char)*DEFAULT_BUFLEN);
			
			int ret = WSAPoll(&fdarray, 1, 0);

			if (ret == SOCKET_ERROR) {
				printf("POLL ERR\n");
			} else {
				if (fdarray.revents & POLLRDNORM) {
					recv(ClientSockets[f], recvbuf, recvbuflen, 0);
					if (strstr(recvbuf, "EXIT.") != 0) {
						printf("EXIT CALL RECIEVED.\n");
						for (int j = 0; j < connected_sockets; j++) {
							iSendResult = send(ClientSockets[j], recvbuf, strlen("EXIT."), 0);
							printf("Bytes sent: %d\n", iSendResult);
						}
						
						return 0;
					}
					snprintf(newbuf, 100, "Client #%d said: %s", f, recvbuf);
					printf("%s\n", newbuf);
					//send the message to all users
					for (int j = 0; j < connected_sockets; j++) {
						iSendResult = send(ClientSockets[j], newbuf, strlen(newbuf), 0);
						printf("Bytes sent: %d\n", iSendResult);
					}
				}
			}
			
			
			if (iResult > 0) {
				printf("Bytes received: %d: %s\n", iResult, recvbuf);
				
			} else if (iResult == 0) {
				//printf("Listen request timed out\n");
			} else {
				printf("recv failed: %d\n", WSAGetLastError());
			}
			iResult = 0;
		}
	}
	printf("exiting...");
	for (int f = 0; f < connected_sockets; f++) {
		closesocket(ClientSockets[f]);
	}
	
	return 0;
}