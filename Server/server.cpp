#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT 5019

int main(int argc, char** argv) {
	char szBuff[100];
	int msg_len;
	int addr_len;
	struct sockaddr_in local, client_addr;

	SOCKET sock, msg_sock;
	WSADATA wsaData;

	if (WSAStartup(0x202, &wsaData) == SOCKET_ERROR){
		// stderr: standard error are printed to the screen.
		fprintf(stderr, "WSAStartup failed with error %d\n", WSAGetLastError());
		//WSACleanup function terminates use of the Windows Sockets DLL. 
		WSACleanup();
		return -1;
	}

	// Fill in the address structure
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY;
	local.sin_port = htons(DEFAULT_PORT);

	sock = socket(AF_INET, SOCK_STREAM, 0);	//TCp socket

	if (sock == INVALID_SOCKET){
		fprintf(stderr, "socket() failed with error %d\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}

	if (bind(sock, (struct sockaddr*)&local, sizeof(local)) == SOCKET_ERROR){
		fprintf(stderr, "bind() failed with error %d\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}

	// waiting for connections
	if (listen(sock, 5) == SOCKET_ERROR){
		fprintf(stderr, "listen() failed with error %d\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}

	printf("Waiting for connections ........\n");

	addr_len = sizeof(client_addr);
	msg_sock = accept(sock, (struct sockaddr*)&client_addr, &addr_len);

	if (msg_sock == INVALID_SOCKET){
		fprintf(stderr, "accept() failed with error %d\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}

	char buffer[INET_ADDRSTRLEN];
	printf("accepted connection from %s, port %d\n",
           inet_ntop(AF_INET, &(client_addr.sin_addr), buffer, INET_ADDRSTRLEN),
           htons(client_addr.sin_port));

	while (1){
		msg_len = recv(msg_sock, szBuff, sizeof(szBuff), 0);

		if (msg_len == SOCKET_ERROR){
			fprintf(stderr, "recv() failed with error %d\n", WSAGetLastError());
			WSACleanup();
			return -1;
		}

		if (msg_len == 0){
			printf("Client closed connection\n");
			closesocket(msg_sock);
			return -1;
		}

        szBuff[msg_len] = '\0';
		printf("Bytes Received: %d, message: %s from %s\n", msg_len, szBuff, inet_ntop(AF_INET, &(client_addr.sin_addr), buffer, INET_ADDRSTRLEN));

		msg_len = send(msg_sock, szBuff, msg_len, 0);
		if (msg_len == 0){
			printf("Client closed connection\n");
			closesocket(msg_sock);
			return -1;
		}
	}

	closesocket(msg_sock);
	WSACleanup();
}
