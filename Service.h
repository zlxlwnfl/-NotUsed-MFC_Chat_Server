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
    mongocxx::instance instance{};
    mongocxx::client client{mongocxx::uri{}};
    
    mongocxx::database db;
public:
    Service() { db = client["mfcChatDb"]; }
    string TimeFormat(time_t t);
    string SignUp(string& recvMessage);
    string SignIn(string& recvMessage);
    string CreateChat(string& recvMessage);
    string GetChat(string& recvMessage);
    void   GetChatSentence(string& recvMessage);
    string SendChatData(string chatId, long lastReadTime, map<string, long>& chatListMap);
    string UserChatList(string& recvMessage);
};