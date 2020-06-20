#pragma once

#include <iostream>
#include <string>
#include <unistd.h>
#include <queue>
#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include "RoomManager.h"
#include "Controller.h"

#define PORT 8080
#define MAX_CONNECTIONS 100

using namespace std;

void close_mainproc(int sig);
void newConnection(int client_sock);

int server_sock;
bool closeServer_flag = false;

class RoomManager;

int main() {

    RoomManager::getInstance().init();

    int client_sock;
    sockaddr_in server_addr, client_addr;

    struct sigaction act_SIGINT;
    act_SIGINT.sa_handler = close_mainproc;
    sigemptyset(&act_SIGINT.sa_mask);
    act_SIGINT.sa_flags = 0;
    sigaction(SIGINT, &act_SIGINT, 0);

    server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(server_sock == -1) {
        std::cout << "listen socket create error" << endl;
        return 1;
    }

    int one = 1;
    if(setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&one, sizeof(one)) == -1) {
        std::cout << "listen socket setsockopt error" << endl;
        return 1;
    }

    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if(bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cout << "listen socket bind error " << PORT << endl;
        return 1;
    }

    std::cout << "listen socket running... " << PORT << endl;

    if(listen(server_sock, MAX_CONNECTIONS) == -1) {
        std::cout << "listen socket listen error " << PORT << endl;
        return 1;
    }

    pid_t pid;
    socklen_t client_addr_size;

    while(true) {
        if(closeServer_flag)  return 0;

        client_addr_size = sizeof(client_addr);
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_size);
        if(client_sock == -1) {
            continue;
        }

        std::cout << "new write socket connected..." << endl;

        thread th = thread(newConnection, client_sock);
        th.detach();
    }
    
}

void close_mainproc(int sig) {
    if(!closeServer_flag) {
        close(server_sock);
        std::cout << endl << "server close... " << PORT << endl;
    }

    closeServer_flag = true;
}

void newConnection(int client_sock) {
    Controller controller(client_sock);
    controller.init();

    close(client_sock);
    std::cout << "write socket disconnected..." << endl;
}