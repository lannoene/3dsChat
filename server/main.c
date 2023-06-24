/*
* lannoene here! I'm probably not going to continue working on this project
* it doesn't seem to be getting very much attention, which is fair I guess
* though, it does mean I can just quit and nobody will care XD
* so that's what I'm doing.
* sorry!
* if you need any bug fixes done, submit a request and I may fix it.
*/

#include <stdio.h>
#include <winsock2.h>
#include <stdbool.h>
#include <ws2tcpip.h>
#include <string.h>
#include <conio.h>

#define DEFAULT_PORT "80"
#define DEFAULT_BUFLEN 512

#define MAX_SOCKETS 50

#define SEND_SIZE 512
#define HEADER_SIZE 32
#define NAME_SIZE 20
#define BODY_SIZE SEND_SIZE - HEADER_SIZE - NAME_SIZE

struct addrinfo *result = NULL, *ptr = NULL, hints;
int num_sockets;
int connected_sockets = 0;
struct clients {
	SOCKET sock;
	bool isConnected;
} ClientSockets[MAX_SOCKETS];
SOCKET ListenSocket = INVALID_SOCKET;

struct Smessage {
	char msgHead[HEADER_SIZE];
	char msgBody[BODY_SIZE];
	char msgName[NAME_SIZE];
} sendMsg;

struct Rmessage {
	char msgHead[HEADER_SIZE];
	char msgBody[BODY_SIZE];
	char msgName[NAME_SIZE];
} recvMsg;

static inline void resetMsgVars(bool resetRecv, bool resetSend) {
	if (resetRecv == true) {
		memset(sendMsg.msgHead, 0, sizeof(sendMsg.msgHead));
		memset(sendMsg.msgBody, 0, sizeof(sendMsg.msgBody));
	}
	
	if (resetSend == true) {
		memset(recvMsg.msgHead, 0, sizeof(recvMsg.msgHead));
		memset(recvMsg.msgBody, 0, sizeof(recvMsg.msgBody));
	}
}

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
	
	printf("---listening for new client---\n");
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
	
	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		printf("Listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	
	return 0;
}

int sendMessageToAll(char* header, char* message, char* name) {
	printf("sending head: %s, Body: %s\n", header, message);
	int iSendResult = 0;
	
	resetMsgVars(true, true);
	
	strcpy(sendMsg.msgHead, header);
	strcpy(sendMsg.msgBody, message);
	strcpy(sendMsg.msgName, name);
	
	for (int f = 0; f < MAX_SOCKETS; f++) {
		if (ClientSockets[f].isConnected == false) {
			continue;
		}
		//casting the struct to a char pointer because that's what sockets wants even though it doesn't even do anything 🙄
		iSendResult = send(ClientSockets[f].sock, (char*)&sendMsg, sizeof(sendMsg), 0);
		if (iSendResult == SOCKET_ERROR) {
			printf("send failed: %d\n", WSAGetLastError());
			closesocket(ClientSockets[f].sock);
			WSACleanup();
			return 1;
		}
		printf("Bytes sent: %d\n", iSendResult);
	}
	return 0;
}

int main() {

	int iResult;
	
	printf("3dsChat SERVER alpha v1.4\n");
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
	
	printf("Press 't' to chat.\n");
	
	
	char header[HEADER_SIZE];
	char bodyer[BODY_SIZE];
	WSAEVENT NewEvent[1];
	
	//create wsapoll data
	WSAPOLLFD fdarray = {0};
	
	for (int i = 0; i < MAX_SOCKETS; i++) {
		ClientSockets[i].isConnected = false;
	}
	
	// Receive until the peer shuts down the connection
	while (true) {
		//deal with incoming clients
		//winsock event var
		NewEvent[0] = WSACreateEvent();
		// with the listening socket and NewEvent listening for FD_ACCEPT
		WSAEventSelect(ListenSocket, NewEvent[0], FD_ACCEPT);
		//no documentation... thank god for this single post pointing me in the right direction: https://stackoverflow.com/a/72793077
		iResult = WSAWaitForMultipleEvents(1, NewEvent, FALSE, 0, FALSE);
		//puts("waiting..");
		if (iResult == WSA_WAIT_EVENT_0) {
			// Accept a client socket
			int currentConnectingSocket;
			for (int i = 0; i < MAX_SOCKETS; i++) {
				if (ClientSockets[i].isConnected == false) {
					ClientSockets[i].sock = accept(ListenSocket, NULL, NULL);
					currentConnectingSocket = i;
					break;
				}
			}
			if (ClientSockets[currentConnectingSocket].sock == INVALID_SOCKET) {
				printf("accept failed: %d\n", WSAGetLastError());
				closesocket(ListenSocket);
				WSACleanup();
				//if you ever get error 10035 here, your 3ds will immidietly crash. i'm too lazy to fix this (adding error handling)
				return 1;
			} else {
				printf("New client #%d accepted\n", currentConnectingSocket);
				char joinMsg[20];
				snprintf(joinMsg, 20, "Client #%d joined", currentConnectingSocket);
				sendMessageToAll("STATUS.", joinMsg, "Server");
				unsigned long OK_HANDSHAKE = htonl(100);
				send(ClientSockets[currentConnectingSocket].sock, (char*)&OK_HANDSHAKE, sizeof(OK_HANDSHAKE), 0);//perform handshake
				ClientSockets[currentConnectingSocket].isConnected = true;
				
			}
			puts("closed listen sock");
			closesocket(ListenSocket);
			connectSock();
		}
		iResult = 0;
		
		//deal with messages
		
		if (kbhit()) {
			char kPres = getch();
			char youTyped[BODY_SIZE];
			//printf("%c\n", c);
			if (kPres == 't') {//send custom server message
				printf("Your message to send to them: ");
				fgets(youTyped, BODY_SIZE, stdin);
				//Send data to user(s)
				snprintf(bodyer, SEND_SIZE, "%s", youTyped);
				
				sendMessageToAll("TEXT.", bodyer, "Server");
			
			} else if (kPres == 'i') {//test image header message
				sendMessageToAll("IMAGE.", "", "Server");
			}
		}
		
		for (int f = 0; f < MAX_SOCKETS; f++) {
			if (ClientSockets[f].isConnected == false) {
				continue;
			}
			fdarray.fd = ClientSockets[f].sock;
			fdarray.events = POLLRDNORM;
			
			memset(recvMsg.msgBody, '\0', sizeof(char)*BODY_SIZE);
			
			int ret = WSAPoll(&fdarray, 1, 0);

			if (ret == SOCKET_ERROR) {
				ClientSockets[f].isConnected = false;
				printf("POLL ERR.Disconnecting client socket.\n");
				closesocket(ClientSockets[f].sock);
				continue;
			} else {
				if (fdarray.revents & POLLRDNORM) {
					//clear the buffer
					//memset(recvbuf, 0, sizeof(char)*SEND_SIZE);
					recv(ClientSockets[f].sock, (char*)&recvMsg, recvbuflen, 0);
					if (strstr(recvMsg.msgHead, "EXIT.") != 0) {
						printf("Client #%d sent an exit call.\n", f);
						//sendMessageToAll("EXIT.", "", "Server");
						closesocket(ClientSockets[f].sock);
						ClientSockets[f].isConnected = false;
						char leftMsg[20];
						snprintf(leftMsg, 20, "Client #%d left", f);
						sendMessageToAll("STATUS.", leftMsg, "Server");
						continue;
						//return 0;
					}
					printf("Body: %s\n", recvMsg.msgBody);
					printf("Head: %s\n", recvMsg.msgHead);
					snprintf(bodyer, SEND_SIZE, "%s", recvMsg.msgBody);
					printf("%s\n", sendMsg.msgBody);
					strcpy(header, recvMsg.msgHead);
					
					//send the message to everyone
					if (strcmp(recvMsg.msgName, "NOTSET") == 0) {
						char tmpName[20];
						snprintf(tmpName, 20, "Client #%d", f);
						sendMessageToAll(header, bodyer, tmpName);
					} else {
						sendMessageToAll(header, bodyer, recvMsg.msgName);
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
		closesocket(ClientSockets[f].sock);
	}
	
	return 0;
}