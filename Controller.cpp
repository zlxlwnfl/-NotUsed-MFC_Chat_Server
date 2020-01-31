#pragma once

#include "Controller.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <jsoncpp/json/json.h>

bool closeClient_flag = false;

void Controller::Controlling() {
    int recvResult;
    string str;

    while(true) {
        if(closeClient_flag) {
            DeleteSendMessage();
            return;
        }

        if(recvResult = (recv(write_sock, recvBuf, BUF_SIZE, MSG_DONTWAIT)) > 0) {
            str = (string)recvBuf;
            recvMessageQueue.push(str);
        }else if(recvResult == -1)    return;

        // recvMessageQueue loop to detect message
        while(!recvMessageQueue.empty()) {
            string recvMessage = recvMessageQueue.front();
            recvMessageQueue.pop();

            TypeParsingAndServiceCall(recvMessage);
        }

        // sendMessageQueue loop to detect message
        while(!sendMessageQueue.empty()) {
            string* sendMessage = sendMessageQueue.front();
            strcpy(sendBuf, (*sendMessage).c_str());
            sendMessageQueue.pop();
            delete sendMessage;

            cout << "server to client send try" << endl;

            if(send(write_sock, sendBuf, BUF_SIZE, 0) == -1)
                cout << "server to client send error" << endl;
        }

        // chatListMap loop to detect chat data to send
        map<string, long>::iterator iter;
        if(!chatListMap.empty()) {
            for(iter = chatListMap.begin(); iter != chatListMap.end(); iter++) {
                string* result;
                result = new string(service.SendChatData(iter->first, iter->second, chatListMap));
                if(*result != "") {
                    cout << "send chat data push" << endl;
                    cout << "service result : " << *result << endl;
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

    string* result;

    switch(type) {
    case TYPE::closeClient: {
        closeClient_flag = true;
        break;
    }
    case TYPE::signUp: {
        result = new string(service.SignUp(str));
        if(*result != "") {
            cout << "service result : " << *result << endl;
            sendMessageQueue.push(result);
        }
        break;
    }
    case TYPE::signIn: {
        result = new string(service.SignIn(str));
        if(*result != "") {
            cout << "service result : " << *result << endl;
            sendMessageQueue.push(result);
        }
        break;
    }
    case TYPE::createChat: {
        string chatId = service.CreateChat(str);
        if(chatId != "") {
            chatListMap[chatId] = 0L;
        }
        break;
    }
    case TYPE::getChat: {
        string chatId = service.GetChat(str);
        if(chatId != "") {
            chatListMap[chatId] = 0L;
        }
        break;
    }
    case TYPE::getChatSentence: {
        cout << "getChatSentence : " << str << endl;
        service.GetChatSentence(str);
        break;
    }
    case TYPE::userChatList: {
        result = new string(service.UserChatList(str));
        if(*result != "") {
            cout << "service result : " << *result << endl;
            sendMessageQueue.push(result);
        }
        break;
    }
    }
}

void Controller::DeleteSendMessage() {
    if(!sendMessageQueue.empty()) {
        string* sendMessage = sendMessageQueue.front();
        sendMessageQueue.pop();
        delete sendMessage;
    }
}