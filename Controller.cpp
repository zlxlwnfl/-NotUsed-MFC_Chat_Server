#pragma once

#include "Controller.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <jsoncpp/json/json.h>

void Controller::Controlling() {
    int recvResult;
    string result;

    // recv function waits if no data received
    while(true) {

        if(recvResult = (recv(write_sock, recvBuf, BUF_SIZE, MSG_DONTWAIT)) > 0) {
            str = (string)recvBuf;
            recvMessageQueue.push(str);
        }else if(recvResult == 0 || recvResult == -1)    return;

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

            cout << "server to client send try" << endl;

            if(send(write_sock, sendBuf, BUF_SIZE, 0) == -1)
                cout << "server to client send error" << endl;
        }

        map<string, long>::iterator iter;
        if(!chatListMap.empty()) {
            for(iter = chatListMap.begin(); iter != chatListMap.end(); iter++) {
                result = service.SendChatData(iter->first, iter->second, chatListMap);
                if(result != "") {
                    cout << "service result : " << result << endl;
                    sendMessageQueue.push(result);
                }
            }
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
    case TYPE::chat:
        result = service.CreateChat(str);
        if(result != "") {
            chatListMap[result] = 0L;
        }
        break;
    }
}