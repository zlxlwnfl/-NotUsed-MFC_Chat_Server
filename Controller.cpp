#pragma once

#include "Controller.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <jsoncpp/json/json.h>

void Controller::Controlling() {

    // recv function waits if no data received
    while(recv(write_sock, recvBuf, BUF_SIZE, 0) > 0) {
        str = (string)recvBuf;
        recvMessageQueue.push(str);

        // new thread
        if(!recvMessageQueue.empty()) {
            string recvMessage = recvMessageQueue.front();
            recvMessageQueue.pop();
            TypeParsingAndServiceCall(recvMessage);
        }

        // new thread
        // sendMessageQueue loop to detect message
        if(!sendMessageQueue.empty()) {
            strcpy(sendBuf, sendMessageQueue.front().c_str());
            sendMessageQueue.pop();

            if(send(write_sock, sendBuf, BUF_SIZE, 0) == -1)
                cout << "server to client send error" << endl;
        }
    }

}

void Controller::TypeParsingAndServiceCall(string& str) {
    Json::Reader reader;
    Json::Value value;
    reader.parse(str, value);

    int type = value["type"].asInt();

    string result;
    switch(type) {
    case TYPE::signUp:
        result = service.SignUp(str);
        if(result != "") {
            cout << "service result : " << result << endl;
            sendMessageQueue.push(result);
        }
        break;
    case TYPE::signIn:
        result = service.SignIn(str);
        if(result != "") {
            cout << "service result : " << result << endl;
            sendMessageQueue.push(result);
        }
        break;
    }
}