#include <cstdio>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mutex>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <thread>
#include <sstream>
#include <regex>

#define SERVER_PORT 5019
#define BUFFER_SIZE 256
#define MAX_CLIENT 128

void handle_conn(SOCKET sock);
void handle_msg(const std::string &msg, SOCKET sender);
void command_listener();

int client_count = 0;
bool server_status = true;
std::mutex mtx;
std::unordered_map<std::string, int> msg_socks;
struct Group {
    std::unordered_set<std::string> members;
    std::string password;
};
std::unordered_map<std::string, Group> groups;

int main(){
    WSADATA wsaData;
    SOCKET server_sock, msg_sock;
    struct sockaddr_in server_addr{}, client_addr{};

    // WSAStartup
    if (WSAStartup(0x202, &wsaData) == SOCKET_ERROR){
        fprintf(stderr, "WSAStartup failed with error %d\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }

    // Initialize the address structure for IPV4 listening
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    // Create a TCP socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == INVALID_SOCKET){
        fprintf(stderr, "socket() failed with error %d\n", WSAGetLastError());
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

    // Listen incoming connections
    if (listen(server_sock, MAX_CLIENT) == SOCKET_ERROR){
        fprintf(stderr, "listen() failed with error %d\n", WSAGetLastError());
        closesocket(server_sock);
        WSACleanup();
        return -1;
    }

    // Start the command listener thread
    std::thread command_thread(command_listener);
    command_thread.detach();

    // Waiting for connections
    printf("Waiting for connections ........\n");

    // Accept connections
    while (server_status){
        if(client_count < MAX_CLIENT){
            // Get client address
            int client_addr_len = sizeof(client_addr);
            msg_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_len);
            if (msg_sock == INVALID_SOCKET) {
                fprintf(stderr, "accept() failed with error %d\n", WSAGetLastError());
                continue;
            }

            // Successfully connected to client notice
            char addr_buffer[INET_ADDRSTRLEN];
            printf("accepted connection from %s, port %d\n",
                   inet_ntop(AF_INET, &(client_addr.sin_addr), addr_buffer, INET_ADDRSTRLEN),
                   htons(client_addr.sin_port));

            // Accepting the connection in a new thread
            mtx.lock();
            std::thread th(handle_conn, msg_sock);
            th.detach();
            client_count++;
            mtx.unlock();
        }
        else{
            printf("Maximum client count reached. Closing connection.\n");
            Sleep(1000);
        }
    }
    closesocket(msg_sock);
    closesocket(server_sock);
    WSACleanup();
    return 0;
}

void handle_conn(SOCKET sock){
    int msg_len;
    char name[32] = {0};
    char szBuff[BUFFER_SIZE] = {0};
    char addr_buffer[INET_ADDRSTRLEN] = {0};
    char msg_prefix[13] = {0};
    std::string feedback_msg;

    // Command Prefixes
    char new_client_prefix[13] = "#New Client:";

    // Get the address information inside the msg_sock socket descriptor
    struct sockaddr_in client_addr{};
    int addr_len = sizeof(client_addr);
    getpeername(sock, (struct sockaddr*)&client_addr, &addr_len);

    // Handling username
    while(server_status){
        // Receive client message
        msg_len = recv(sock, szBuff, sizeof(szBuff)-1, 0);
        if (msg_len == SOCKET_ERROR){
            fprintf(stderr, "recv() failed with error %d\n", WSAGetLastError());
            break;
        }
        if (msg_len == 0){
            printf("Client %s closed connection\n", name);
            mtx.lock();
            client_count--;
            mtx.unlock();
            closesocket(sock);
            break;
        }

        strncpy(msg_prefix, szBuff, 12);
        msg_prefix[12] = '\0';

        if (strcmp(msg_prefix, new_client_prefix) == 0){
            strcpy(name, szBuff + 12);
            if (msg_socks.find(name) == msg_socks.end()){
                printf("The name of client %s %llu: %s\n",
                       inet_ntop(AF_INET, &(client_addr.sin_addr), addr_buffer, INET_ADDRSTRLEN),
                       sock, name);
                msg_socks[name] = sock;
                handle_msg(szBuff, sock);
                break;
            }
            else{
                feedback_msg = "User " + std::string(name) + " already exists. Please choose another one!";
                send(sock, feedback_msg.c_str(), feedback_msg.length() + 1, 0);
                continue;
            }
        }
    }

    // Handling messages
    while (server_status){
        // Receive client message
        msg_len = recv(sock, szBuff, sizeof(szBuff)-1, 0);
        if (msg_len == SOCKET_ERROR){
            fprintf(stderr, "recv() failed with error %d\n", WSAGetLastError());
            feedback_msg = "Error occurred when receiving message.\n";
            send(sock, feedback_msg.c_str(), feedback_msg.length() + 1, 0);
            break;
        }
        if (msg_len == 0){
            printf("Client closed connection\n");
            break;
        }

        // Ensure szBuff is null-terminated after receiving data
        szBuff[msg_len] = '\0';
        // Successfully receive notification
        printf("Bytes Received: %d, message: %s from %s\n", msg_len, szBuff, name);
        // Handle message
        handle_msg(std::string(szBuff), sock);
    }

    feedback_msg = "[System] " + std::string(name) + " exit the chatroom.\n";
    handle_msg(feedback_msg, sock);
    mtx.lock();
    msg_socks.erase(name);
    client_count--;
    mtx.unlock();
    closesocket(sock);
    return;
}

void handle_msg(const std::string &msg, SOCKET sender) {
    mtx.lock();

    if(sender != -1) {
        std::smatch match;
        std::regex group_create_regex(R"(\[(\S+)\] Group @\[([^\]]+)\] ([^,]+), (\S+))");

        // Create group
        if (std::regex_match(msg, match, group_create_regex)) {
            std::string sender_name = match[1];
            std::string group_members_str = match[2];
            std::string group_name = match[3];
            std::string password = match[4];

            group_members_str += " " + sender_name;

            std::istringstream iss(group_members_str);
            std::unordered_set<std::string> group_members;
            std::string member;
            bool all_members_exist = true;

            while (std::getline(iss, member, ' ')) {
                if (msg_socks.find(member) == msg_socks.end()) {
                    all_members_exist = false;
                    break;
                }
                group_members.insert(member);
            }

            if (all_members_exist) {
                groups[group_name] = {group_members, password};
                std::string feedback_msg = "Group " + group_name + " created successfully.\n";
                for (const auto &m : group_members) {
                    send(msg_socks[m], feedback_msg.c_str(), feedback_msg.length() + 1, 0);
                }
            } else {
                std::string feedback_msg = "Error: One or more users do not exist. Group not created.\n";
                send(sender, feedback_msg.c_str(), feedback_msg.length() + 1, 0);
            }

            mtx.unlock();
            return;
        }

        std::string dm_prefix = "@";
        std::string group_prefix = "@[";
        int first_space = msg.find_first_of(" ");

        // Group chat message
        if (msg.compare(first_space + 1, 2, group_prefix) == 0) {
            int group_start = msg.find(group_prefix) + group_prefix.length();
            int group_end = msg.find(']', group_start);
            std::string group_name = msg.substr(group_start, group_end - group_start);
            std::string message = msg.substr(group_end + 1);

            auto group_it = groups.find(group_name);
            if (group_it != groups.end()) {
                for (const auto &member : group_it->second.members) {
                    send(msg_socks[member], message.c_str(), message.length() + 1, 0);
                }
            } else {
                std::string feedback_msg = "Error: Group " + group_name + " does not exist.\n";
                send(sender, feedback_msg.c_str(), feedback_msg.length() + 1, 0);
            }
            mtx.unlock();
            return;
        }

        // Direct message
        if (msg.compare(first_space + 1, 1, dm_prefix) == 0) {
            int space = msg.find_first_of(" ", first_space + 1);
            std::string receive_name = msg.substr(first_space + 2, space - first_space - 2);
            std::string send_name = msg.substr(1, first_space - 2);
            // If user does not exist
            if (msg_socks.find(receive_name) == msg_socks.end()) {
                std::string error_msg = "[error] there is no client named " + receive_name;
                send(msg_socks[send_name], error_msg.c_str(), error_msg.length() + 1, 0);
            } else {
                send(msg_socks[receive_name], msg.c_str(), msg.length() + 1, 0);
                send(msg_socks[send_name], msg.c_str(), msg.length() + 1, 0);
            }
            mtx.unlock();
            return;
        }
    }

    // Group Message
    for (const auto &it : msg_socks){
        send(it.second, msg.c_str(), msg.length()+1, 0);
    }

    mtx.unlock();
}

void command_listener() {
    std::string command;
    std::string feedback_msg;

    while(true){
        std::getline(std::cin, command);
        std::istringstream iss(command);
        std::string cmd, arg;
        iss >> cmd >> arg;

        if (cmd == "#quit" || cmd == "#Quit") {
            std::cout << "Shutting down server...\n";
            server_status = false;

            // Close all client sockets
            mtx.lock();
            for (auto &entry : msg_socks) {
                closesocket(entry.second);
            }
            msg_socks.clear();
            mtx.unlock();

            exit(0);
        }
        else if (cmd == "#del" && !arg.empty()) {
            mtx.lock();
            auto it = msg_socks.find(arg);
            if (it != msg_socks.end()) {
                closesocket(it->second);
                msg_socks.erase(it);
                std::cout << "User " << arg << " has been deleted.\n";
                feedback_msg = "#Client " + arg + " has been deleted from this chatroom.\n";
                handle_msg(feedback_msg, -1);
                client_count--;
            }
            else {
                std::cout << "No such user: " << arg << "\n";
            }
            mtx.unlock();
        }
        else{
            printf("Invalid command.\n");
        }
    }
}
