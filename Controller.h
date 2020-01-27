#pragma once

#include <iostream>
#include <string>
#include <unistd.h>
#include <queue>
#include "Controller.h"
#include "Service.h"

#define BUF_SIZE 1024

using namespace std;

class Controller {
private:
    Service service;

    int write_sock;
    char recvBuf[BUF_SIZE];
    char sendBuf[BUF_SIZE];
    string str;

    queue<string> recvMessageQueue;
    queue<string> sendMessageQueue;

    enum TYPE {
        signUp,
        signIn
    };
public:
    Controller(int write_sock) { this->write_sock = write_sock; }
    void Controlling();
    void TypeParsingAndServiceCall(string& str);
};