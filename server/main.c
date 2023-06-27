#include <stdio.h>
#include <winsock2.h>
#include <stdbool.h>
#include <ws2tcpip.h>
#include <string.h>
#include <conio.h>
#include <time.h>

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
	char name[NAME_SIZE];
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
		//casting the struct to a char pointer because that's what sockets wants even though it doesn't do anything
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

void sendUserList() {
	connected_sockets = 0;
	for (int i = 0; i < MAX_SOCKETS; i++) {
		if (ClientSockets[i].isConnected == true) {
			++connected_sockets;
		}
	}
	char userListNum[50];
	snprintf(userListNum, 50, "%d", connected_sockets);
	sendMessageToAll("USERUPDATE.", userListNum, "Server");
}


int main() {
	int iResult;
	
	printf("3dsChat SERVER alpha v1.4\n");
	
	//connect a new socket command
	connectSock();
	
	//get server ip
	struct hostent *thisHost;
	thisHost = gethostbyname("");
	printf("Server address: %s\n", inet_ntoa(*(struct in_addr *) *thisHost->h_addr_list));
	
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	
	printf("Press 't' to chat.\n");
	
	
	char header[HEADER_SIZE];
	char bodyer[BODY_SIZE];
	
	WSAEVENT NewEvent[1];
	WSANETWORKEVENTS NetworkEvents;
	
	//create wsapoll data
	WSAPOLLFD fdarray = {0};
	
	for (int i = 0; i < MAX_SOCKETS; i++) {
		ClientSockets[i].isConnected = false;
	}
	//winsock event var
	NewEvent[0] = WSACreateEvent();
	
	// with the listening socket and NewEvent listening for FD_ACCEPT
	WSAEventSelect(ListenSocket, NewEvent[0], FD_ACCEPT);
	
	// Receive until the peer shuts down the connection
	while (true) {
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);

		Sleep(100);//don't want to use up too much resources
		//deal with incoming clients
		//no documentation... thank god for this single post pointing me in the right direction: https://stackoverflow.com/a/72793077
		iResult = WSAWaitForMultipleEvents(1, NewEvent, FALSE, 0, FALSE);

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
				
				int rResult = WSAEnumNetworkEvents(ListenSocket, NewEvent[0], &NetworkEvents);
				
				if (rResult < 0) {
					printf("error");
				} else {
					printf("%d", iResult);
				}
			}
			puts("closed listen sock");
			closesocket(ListenSocket);
			connectSock();
			// with the listening socket and NewEvent listening for FD_ACCEPT
			WSAEventSelect(ListenSocket, NewEvent[0], FD_ACCEPT);
			sendUserList();
		}
		iResult = 0;
		
		//deal with messages
		
		if (kbhit()) {
			char kPres = getch();
			char youTyped[BODY_SIZE];
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
				sendUserList();
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
						sendUserList();
						continue;
					} else if (strstr(recvMsg.msgHead, "NAMEUPDATE.") != 0) {
						if (strcmp(recvMsg.msgBody, "NOTSET") == 0) {
							char tmpName[20];
							snprintf(ClientSockets[f].name, NAME_SIZE, "Client #%d", f);
						} else {
							strcpy(ClientSockets[f].name, recvMsg.msgBody);
						}
					} else if (strstr(recvMsg.msgHead, "TEXT.") != 0) {
						snprintf(bodyer, SEND_SIZE, "%s", recvMsg.msgBody);
						strcpy(header, recvMsg.msgHead);

						//send the message to everyone
						sendMessageToAll(header, bodyer, ClientSockets[f].name);
					}
				}
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