#pragma once

#include <iostream>
#include <string>
#include <unistd.h>
#include <queue>
#include <vector>
#include <map>
#include <mutex>
#include <condition_variable>

#define BUF_SIZE 1024

using namespace std;

class RoomManager;
class Service;

class Controller {
private:
    Service* service;

    int write_sock;
    char recvBuf[BUF_SIZE];
    char sendBuf[BUF_SIZE];
    string ID;

    mutex recv_m, send_m;
    condition_variable recv_cv, send_cv;

    queue<string> recvMessageQueue;
    queue<string> sendMessageQueue;

    enum TYPE {
        closeClient,
        signUp,
        signIn,
        createChat,
        getChat,
        getChatSentence,
        userChatList
    };

    void typeParsingAndServiceCall(string str);
    void recvMessageConsumer();
    void sendMessageConsumer();

public:
    Controller(int write_sock) { this->write_sock = write_sock; };
    ~Controller() { delete service; };
    void init();
    void controlling();
    void recvMessageProducer(string message);
    void sendMessageProducer(string message);
};