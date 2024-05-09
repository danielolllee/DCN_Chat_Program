#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT 5019

void accept_conn(void* client_sock);

int main(int argc, char** argv) {
    char szBuff[100];
    int msg_len;
    struct sockaddr_in local, client_addr;

    SOCKET sock, msg_sock;
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed with error %d\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }

    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons(DEFAULT_PORT);

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "socket() failed with error %d\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }

    if (bind(sock, (struct sockaddr*)&local, sizeof(local)) == SOCKET_ERROR) {
        fprintf(stderr, "bind() failed with error %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    if (listen(sock, SOMAXCONN) == SOCKET_ERROR) {
        fprintf(stderr, "listen() failed with error %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    printf("Waiting for connections ...\n");

    int client_count = 0;
    while (client_count < 666) {
        int addr_len = sizeof(client_addr);
        msg_sock = accept(sock, (struct sockaddr*)&client_addr, &addr_len);

        if (msg_sock == INVALID_SOCKET) {
            fprintf(stderr, "accept() failed with error %d\n", WSAGetLastError());
            closesocket(sock);
            WSACleanup();
            return -1;
        }

        _beginthread(accept_conn, 0, (void*)&msg_sock);
        client_count++;
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}

void accept_conn(void* client_sock) {
    SOCKET msg_sock = *((SOCKET*)client_sock);
    char szBuff[100];
    int msg_len;

    msg_len = recv(msg_sock, szBuff, sizeof(szBuff), 0);
    if (msg_len == SOCKET_ERROR) {
        fprintf(stderr, "recv() failed with error %d\n", WSAGetLastError());
        closesocket(msg_sock);
        return;
    }

    if (msg_len == 0) {
        printf("Client closed connection\n");
        closesocket(msg_sock);
        return;
    }

    szBuff[msg_len] = '\0';
    printf("Received message: %s\n", szBuff);

    msg_len = send(msg_sock, szBuff, msg_len, 0);
    if (msg_len == SOCKET_ERROR) {
        fprintf(stderr, "send() failed with error %d\n", WSAGetLastError());
        closesocket(msg_sock);
        return;
    }

    closesocket(msg_sock);
}
