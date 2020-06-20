#pragma once

#include <iostream>
#include <string>
#include <unistd.h>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>

#include <cpp_redis/cpp_redis>

using namespace std;

class Controller;

class RoomManager {
private:
    mongocxx::instance instance{};
    mongocxx::client client{mongocxx::uri{}};
    
    mongocxx::database db;

    cpp_redis::client conList;
    cpp_redis::client roomList;

    RoomManager() { };

public:
    static RoomManager& getInstance() {
        static RoomManager rm;
        return rm;
    };
    void init();
    void setConList(const Controller* con, const string userId);
    void unsetConList(const string userId);
    void setRoomList(const vector<string>& chatIdVec, const string userId);
    void enterNewRoom(const string chatId, const string userId);
    void exitRoom(const string chatId, const string userId);
    void messageBrodcast(const string chatId, const string userId, const string chatStr);
};