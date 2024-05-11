#include <winsock2.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT 5019
#define BUFFER_SIZE 100
#define MAX_CONNECT_ATTEMPT 10

int main(int argc, char** argv){
    int msg_len;
    int attempts = 0;
    char szBuff[BUFFER_SIZE] = {0};

    struct hostent* hp;
    struct sockaddr_in server_addr = {0};

    SOCKET client_sock;
    WSADATA wsaData;

    const char* server_name = "localhost";
    unsigned short port = DEFAULT_PORT;

    // Handle add client command
    if (argc != 3){
        printf("Usage: ./client.exe [server name] [port number]\n");
        return -1;
    }
    else{
        server_name = argv[1];
        port = atoi(argv[2]);
    }

    // Handle WSAStartup
    if (WSAStartup(0x202, &wsaData) == SOCKET_ERROR){
        fprintf(stderr, "WSAStartup failed with error %d\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }

    // Initialize an IPV4 address & port for the client
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Resolves host's IP address
    if (isalpha(server_name[0])){
        hp = gethostbyname(server_name);
        if (hp == nullptr){
            fprintf(stderr, "Cannot resolve address: %d\n", WSAGetLastError());
            WSACleanup();
            return -1;
        }
        memcpy(&(server_addr.sin_addr), hp->h_addr, hp->h_length);
    }
    else{
        server_addr.sin_addr.s_addr = inet_addr(server_name);
    }

    // Initialize a TCP socket for client
    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock == INVALID_SOCKET){
        fprintf(stderr, "socket() failed with error %d\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }

    printf("Client connecting to: %s\n", hp->h_name);

    // Connection retry strategy
    while (attempts < MAX_CONNECT_ATTEMPT){
        if (connect(client_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            fprintf(stderr, "connect() failed with error %d, attempt %d\n", WSAGetLastError(), attempts + 1);
            Sleep(1000);
            attempts++;
        }
        else{
            break;
        }
    }
    if (attempts >= MAX_CONNECT_ATTEMPT) {
        fprintf(stderr, "Failed to connect after %d attempts\n", attempts);
        closesocket(client_sock);
        WSACleanup();
        return -1;
    }

    while (true){
        // Get user input
        printf("Input character string: ");
        fgets(szBuff, sizeof(szBuff), stdin);
        szBuff[strcspn(szBuff, "\n")] = '\0';

        if((int)strlen(szBuff) == 0){
            printf("You cannot send empty message!\n");
            continue;
        }

        // Send input to server
        msg_len = send(client_sock, szBuff, (int)strlen(szBuff), 0);
        if (msg_len == SOCKET_ERROR){
            fprintf(stderr, "send() failed with error %d\n", WSAGetLastError());
            break;
        }

        // Check if server closed connection
        if (msg_len == 0){
            closesocket(client_sock);
            printf("server closed connection\n");
            break;
        }

        // Get respond from server
        msg_len = recv(client_sock, szBuff, sizeof(szBuff)-1, 0);
        if (msg_len == SOCKET_ERROR){
            fprintf(stderr, "send() failed with error %d\n", WSAGetLastError());
            break;
        }

        // Check if server closed connection
        if (msg_len == 0){
            closesocket(client_sock);
            printf("server closed connection\n");
            break;
        }

        szBuff[msg_len] = '\0';
        printf("Echo from the server: %s\n", szBuff);
    }

    shutdown(client_sock, SD_SEND);
    closesocket(client_sock);
    WSACleanup();
    return 0;
}
