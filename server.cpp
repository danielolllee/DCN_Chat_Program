#include <cstdio>
#include <process.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT 5019
#define BUFFER_SIZE 100
#define MAX_CLIENT 100

void accept_conn(void* client_msg_sock);

int main(int argc, char** argv) {
    int client_count = 0;
	struct sockaddr_in server_addr{}, client_addr{};

	WSADATA wsaData;
    CRITICAL_SECTION cs;
    SOCKET server_sock, msg_sock;

    // Handle WSAStartup
	if (WSAStartup(0x202, &wsaData) == SOCKET_ERROR){
		fprintf(stderr, "WSAStartup failed with error %d\n", WSAGetLastError());
		WSACleanup();
		return -1;
	}

	// Initialize the address structure for IPV4 listening
	server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(DEFAULT_PORT);

    // Create a TCP socket
	server_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (server_sock == INVALID_SOCKET){
		fprintf(stderr, "socket() failed with error %d\n", WSAGetLastError());
        closesocket(server_sock);
		WSACleanup();
		return -1;
	}

    // Bind server socket to server_addr
	if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR){
		fprintf(stderr, "bind() failed with error %d\n", WSAGetLastError());
        closesocket(server_sock);
		WSACleanup();
		return -1;
	}

	// Set up server socket for listening incoming connections
	if (listen(server_sock, MAX_CLIENT) == SOCKET_ERROR){
		fprintf(stderr, "listen() failed with error %d\n", WSAGetLastError());
        closesocket(server_sock);
		WSACleanup();
		return -1;
	}

    // Waiting for connections
	printf("Waiting for connections ........\n");
    InitializeCriticalSection(&cs);

    while (true) {
        if(client_count < MAX_CLIENT){
            // Get client address
            int client_addr_len = sizeof(client_addr);
            msg_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_len);
            if (msg_sock == INVALID_SOCKET) {
                fprintf(stderr, "accept() failed with error %d\n", WSAGetLastError());
                closesocket(msg_sock);
                return -1;
            }
            // Successfully connected to client notice
            char addr_buffer[INET_ADDRSTRLEN];
            printf("accepted connection from %s, port %d\n",
                   inet_ntop(AF_INET, &(client_addr.sin_addr), addr_buffer, INET_ADDRSTRLEN),
                   htons(client_addr.sin_port));
            // Start accept_Conn()
            EnterCriticalSection(&cs);
            if (_beginthread(accept_conn, 0, (void*)&msg_sock) == -1){
                fprintf(stderr, "Thread creation failed\n");
                closesocket(msg_sock);
                LeaveCriticalSection(&cs);
                continue;
            }
            client_count++;
            LeaveCriticalSection(&cs);
        }
        else{
            printf("Maximum client count reached. Closing connection.\n");
        }
    }
    DeleteCriticalSection(&cs);
	closesocket(server_sock);
	WSACleanup();
    return 0;
}

void accept_conn(void* client_msg_sock) {
    int msg_len;
    char szBuff[BUFFER_SIZE] = {0};
    char addr_buffer[INET_ADDRSTRLEN] = {0};

    // Cast to the main() msg_sock
    SOCKET msg_sock = *((SOCKET*)client_msg_sock);

    // Get the address information inside the msg_sock socket descriptor
    struct sockaddr_in client_addr{};
    int addr_len = sizeof(client_addr);
    getpeername(msg_sock, (struct sockaddr*)&client_addr, &addr_len);

    while (true){
        // Receive client message
        msg_len = recv(msg_sock, szBuff, sizeof(szBuff)-1, 0);
        // Handle recv() exceptions
        if (msg_len == SOCKET_ERROR){
            fprintf(stderr, "recv() failed with error %d\n", WSAGetLastError());
            return;
        }

        // Check if client closed connection
        if (msg_len == 0){
            printf("Client closed connection\n");
            closesocket(msg_sock);
            return;
        }

        // Ensure szBuff is null-terminated after receiving data
        szBuff[msg_len] = '\0';
        // Successfully receive notification
        printf("Bytes Received: %d, message: %s from %s\n", msg_len, szBuff, inet_ntop(AF_INET, &(client_addr.sin_addr), addr_buffer, INET_ADDRSTRLEN));

        // Respond to the client
        msg_len = send(msg_sock, szBuff, msg_len, 0);

        // Check if client closed connection
        if (msg_len == 0){
            printf("Client closed connection\n");
            closesocket(msg_sock);
            return;
        }
    }
}
