#pragma once

#include "Controller.h"
#include "RoomManager.h"
#include "Service.h"
#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <jsoncpp/json/json.h>

bool closeClient_flag = false;

void Controller::init() {
    cout << "new controller init" << endl;

    service = new Service();

    thread recv_th = thread(&Controller::recvMessageConsumer, this);
    thread send_th = thread(&Controller::sendMessageConsumer, this);

    thread control_th = thread(&Controller::controlling, this);
    control_th.join();

    recv_cv.notify_all();
    send_cv.notify_all();

    recv_th.join();
    send_th.join();
}

void Controller::controlling() {
    int recvResult;
    string str;
    
    while(true) {
        if(closeClient_flag)  return;

        if(recvResult = (recv(write_sock, recvBuf, BUF_SIZE, 0)) > 0) {
            str = (string)recvBuf;
            recvMessageProducer(str);
        }else if(recvResult == -1)  return;
    }
}

void Controller::typeParsingAndServiceCall(string str) {
    Json::Reader reader;
    Json::Value value;
    reader.parse(str, value);

    int type = value["type"].asInt();

    cout << "type : " << type << endl;

    string result;

    switch(type) {
    case TYPE::closeClient: {
        closeClient_flag = true;
        break;
    }
    case TYPE::signUp: {
        result = service->signUp(str);
        if(result != "") {
            cout << "service result : " << result << endl;
            ID=result;
            RoomManager::getInstance().setConList(this, ID);
            sendMessageProducer(result);
        }
        break;
    }
    case TYPE::signIn: {
        result = service->signIn(str);
        if(result != "") {
            cout << "service result : " << result << endl;
            sendMessageProducer(result);
        }
        break;
    }
    case TYPE::createChat: {
        string chatId = service->createChat(str);
        break;
    }
    case TYPE::getChat: {
        string chatId = service->getChat(str);
        break;
    }
    case TYPE::getChatSentence: {
        cout << "getChatSentence : " << str << endl;
        service->getChatSentence(str);
        break;
    }
    case TYPE::userChatList: {
        result = service->userChatList(str);
        if(result != "") {
            cout << "service result : " << result << endl;
            sendMessageProducer(result);
        }
        break;
    }
    }
}

void Controller::recvMessageProducer(string message) {
    recv_m.lock();
    recvMessageQueue.push(message);
    recv_m.unlock();

    recv_cv.notify_one();
}

void Controller::sendMessageProducer(string message) {
    if(closeClient_flag)    return;

    send_m.lock();
    sendMessageQueue.push(message);
    send_m.unlock();

    send_cv.notify_one();
}

void Controller::recvMessageConsumer() {
    // recvMessageQueue loop to detect message
    while(true) {
        unique_lock<mutex> lk(recv_m);

        if(closeClient_flag && recvMessageQueue.empty())    return;
        recv_cv.wait(lk, [&] { return !recvMessageQueue.empty(); });
        if(recvMessageQueue.empty())    return;

        string recvMessage = recvMessageQueue.front();
        recvMessageQueue.pop();

        lk.unlock();
        
        typeParsingAndServiceCall(recvMessage);
    }
}

void Controller::sendMessageConsumer() {
    // sendMessageQueue loop to detect message
    while(true) {
        unique_lock<mutex> lk(send_m);

        send_cv.wait(lk, [&] { return !sendMessageQueue.empty(); });
        if(closeClient_flag)    return;

        string sendMessage = sendMessageQueue.front();
        strcpy(sendBuf, sendMessage.c_str());
        sendMessageQueue.pop();

        lk.unlock();

        cout << "server to client send try" << endl;

        if(send(write_sock, sendBuf, BUF_SIZE, 0) == -1)
            cout << "server to client send error" << endl;
    }
}