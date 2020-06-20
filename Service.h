#pragma once

#include <iostream>
#include <string>
#include <unistd.h>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>

using namespace std;

class Service {
private:
    string ID;

    mongocxx::client client{mongocxx::uri{}};
    
    mongocxx::database db;
    
public:
    //: rm(RoomManager::getInstance()) 
    Service() { db = client["mfcChatDb"]; }
    string timeFormat(time_t t);
    string signUp(const string recvMessage);
    string signIn(const string recvMessage);
    string createChat(const string recvMessage);
    string getChat(const string recvMessage);
    void   getChatSentence(const string recvMessage);
    string userChatList(const string recvMessage);
};