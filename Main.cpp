#include <iostream>
#include <string>
#include <unistd.h>
#include <queue>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include "Controller.h"

#define PORT 8080
#define MAX_CONNECTIONS 100

using namespace std;

void wait_childproc(int sig);
void close_mainproc(int sig);

int listen_sock;
bool closeServer_flag = false;

int main() {

    int write_sock;
    sockaddr_in listen_addr, write_addr;

    struct sigaction act_SIGCHLD;
    act_SIGCHLD.sa_handler = wait_childproc;
    sigemptyset(&act_SIGCHLD.sa_mask);
    act_SIGCHLD.sa_flags = 0;
    sigaction(SIGCHLD, &act_SIGCHLD, 0);

    struct sigaction act_SIGINT;
    act_SIGINT.sa_handler = close_mainproc;
    sigemptyset(&act_SIGINT.sa_mask);
    act_SIGINT.sa_flags = 0;
    sigaction(SIGINT, &act_SIGINT, 0);

    listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(listen_sock == -1) {
        std::cout << "listen socket create error" << endl;
        return 1;
    }

    int one = 1;
    if(setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&one, sizeof(one)) == -1) {
        std::cout << "listen socket setsockopt error" << endl;
        return 1;
    }

    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(PORT);

    if(bind(listen_sock, (struct sockaddr*)&listen_addr, sizeof(listen_addr)) == -1) {
        std::cout << "listen socket bind error " << PORT << endl;
        return 1;
    }

    std::cout << "listen socket running... " << PORT << endl;

    if(listen(listen_sock, MAX_CONNECTIONS) == -1) {
        std::cout << "listen socket listen error " << PORT << endl;
        return 1;
    }

    pid_t pid;
    socklen_t write_addr_size;

    while(true) {
        if(closeServer_flag)  return 0;

        write_addr_size = sizeof(write_addr);
        write_sock = accept(listen_sock, (struct sockaddr*)&write_addr, &write_addr_size);
        if(write_sock == -1) {
            continue;
        }

        std::cout << "new write socket connected..." << endl;

        pid = fork();

        if(pid == -1) { // parent process
            close(write_sock);

            continue;
        }
        if(pid == 0) {  // child process
            close(listen_sock);

            Controller controller(write_sock);
            controller.Controlling();

            close(write_sock);
            std::cout << "write socket disconnected..." << endl;
            return 0;
        }
    }
    
}

void wait_childproc(int sig) {
    pid_t pid;
    int status;

    pid = waitpid(-1, &status, WNOHANG);
    if(WIFEXITED(status)) {
        cout << "removed proc id : " << pid << endl;
    }
}

void close_mainproc(int sig) {
    if(!closeServer_flag) {
        close(listen_sock);
        std::cout << endl << "server close... " << PORT << endl;
    }

    closeServer_flag = true;
}