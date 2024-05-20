#include <winsock2.h>
#include <cstdio>
#include <string>
#include <thread>
#include <iostream>

#define DEFAULT_PORT 5019
#define DEFAULT_IP "127.0.0.1"
#define BUFFER_SIZE 256
#define MAX_CONNECT_ATTEMPT 10

void send_msg(SOCKET sock);
void recv_msg(SOCKET sock);

std::string name, msg;

int main(){
    int attempts = 0;
    struct hostent* hp;
    struct sockaddr_in server_addr = {0};

    SOCKET client_sock;
    WSADATA wsaData;

    const char* server_name = DEFAULT_IP;
    unsigned short port = DEFAULT_PORT;

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

    // Announce username to server
    char szBuff[BUFFER_SIZE] = {0};
    while(true){
        printf("Pick a username: ");
        std::getline(std::cin, name);
        if (name.length() > 32 || name.length() < 3){
            printf("Username shall have 3-32 characters. Please choose another one.\n");
            continue;
        }
        std::string my_name = "#New Client:" + name;
        send(client_sock, my_name.c_str(), my_name.length() + 1, 0);
        int msg_len = recv(client_sock, szBuff, sizeof(szBuff)-1, 0);
        printf("%s\n", szBuff);
        if(msg_len == my_name.length() + 1){
            break;
        }
        else{
            name.clear();
            continue;
        }
    }

    printf("### Chat commands:\n");
    printf("Chatroom Message: msg\n");
    printf("Direct Message: @username msg\n");
    printf("Group Message: @[Group name] msg\n");
    printf("### Group commands:\n");
    printf("Create Group: Group @[user1 user2...] Group name, password\n");
    printf("Add into a Group: Group_add @Group name, password\n");
    printf("Delete a Group: Group_del @Group name, password\n");
    printf("Delete a Group Member: Group_delp @Group name, username, password\n");
    printf("### Hints:\n");
    printf("#clientList - Show the current online client list\n");
    printf("#help - Show this help message\n");
    printf("#quit - Quit the chat\n");

    // Send & Recv messages
    std::thread snd(send_msg, client_sock);
    std::thread rcv(recv_msg, client_sock);

    snd.join();
    rcv.join();

    shutdown(client_sock, SD_SEND);
    closesocket(client_sock);
    WSACleanup();
    return 0;
}

void send_msg(SOCKET sock){
    int msg_len;
    char szBuff[BUFFER_SIZE] = {0};
    while(true){
        // Get user input
        fgets(szBuff, sizeof(szBuff), stdin);
        szBuff[strcspn(szBuff, "\n")] = '\0';

        if((int)strlen(szBuff) == 0){
            printf("You cannot send empty message!\n");
            continue;
        }

        // Handle commands
        if (std::string(szBuff) == "#help"){
            printf("### Chat commands:\n");
            printf("Chatroom Message: msg\n");
            printf("Direct Message: @username msg\n");
            printf("Group Message: @[Group name] msg\n");
            printf("### Group commands:\n");
            printf("Create Group: Group @[user1 user2...] Group name, password\n");
            printf("Add into a Group: Group_add @Group name, password\n");
            printf("Delete a Group: Group_del @Group name, password\n");
            printf("Delete a Group Member: Group_delp @Group name, username, password\n");
            printf("### Hints:\n");
            printf("#clientList - Show the current online client list\n");
            printf("#help - Show this help message\n");
            printf("#quit - Quit the chat\n");
            continue;
        }

        // Client List
        if (std::string(szBuff) == "#clientList"){
            msg = "[" + name + "] " + "#applyforclientList";
        }

        // Quit
        else if (std::string(szBuff) == "#quit"){
            closesocket(sock);
            WSACleanup();
            exit(0);
        }

        else{
            msg = "[" + name + "] " + szBuff;
        }

        // Send input to server
        msg_len = send(sock, msg.c_str(), msg.length() + 1, 0);
        if (msg_len == SOCKET_ERROR){
            fprintf(stderr, "send() failed with error %d\n", WSAGetLastError());
            break;
        }

        // Check if server closed connection
        if (msg_len == 0){
            closesocket(sock);
            printf("server closed connection\n");
            break;
        }
    }
}

void recv_msg(SOCKET sock){
    int msg_len;
    char szBuff[BUFFER_SIZE + name.length() + 1];

    while(true){
        // Get respond from server
        msg_len = recv(sock, szBuff, sizeof(szBuff)-1, 0);
        if (msg_len == SOCKET_ERROR){
            fprintf(stderr, "recv() failed with error %d\nProgram will be closed in 3s.\n", WSAGetLastError());
            closesocket(sock);
            Sleep(3000);
            exit(-1);
        }

        // Check if server closed connection
        if (msg_len == 0){
            printf("server closed connection\nProgram will be closed in 3s.\n");
            closesocket(sock);
            Sleep(3000);
            exit(-1);
        }

        // Display other user's messages
        szBuff[msg_len] = '\0';
        if(strcmp(szBuff, msg.c_str()) != 0){
            printf("%s\n", szBuff);
        }
    }
}
