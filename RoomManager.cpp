#pragma once

#include "RoomManager.h"
#include "Controller.h"
#include <jsoncpp/json/json.h>
#include <cstdint>
#include <ctime>
#include <vector>
#include <cstdint>

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

void RoomManager::init() {
    db = client["mfcChatDb"];
    
    roomList.connect();
    if(!roomList.is_connected()) {
        cout << "redis connect fail" << endl;
    }

    conList.connect();
    if(!conList.is_connected()) {
        cout << "redis connect fail" << endl;
    }
}

void RoomManager::setConList(const Controller* con, const string userId) {
    conList.set(userId, to_string(reinterpret_cast<intptr_t>(con)));
    cout << "set conList : " << userId << endl;
}

void RoomManager::unsetConList(const string userId) {
    conList.del(vector<string>{userId});
    cout << "unset conList : " << userId << endl;
}

void RoomManager::setRoomList(const vector<string>& chatIdVec, const string userId) {
    for(string chatId : chatIdVec) {
        roomList.sadd(chatId, vector<string>{userId});
        cout << "set roomList : " << chatId << " - " << userId << endl;
    }
}

void RoomManager::enterNewRoom(const string chatId, const string userId) {
    roomList.sadd(chatId, vector<string>{userId});
    cout << "enter new room : " << chatId << " - " << userId << endl;
}

void RoomManager::exitRoom(const string chatId, const string userId) {
    roomList.srem(chatId, vector<string>{userId});
    cout << "exit room : " << chatId << " - " << userId << endl;
}

void RoomManager::messageBrodcast(const string chatId, const string userId, const string chatStr) {
    mongocxx::collection chatCol = db["chatInfo"];

    bsoncxx::oid chatOid(chatId);

    // add chat document to user col
    auto addChatSentenceToChatInfoResult =
        chatCol.update_one(document{} << "_id" <<  chatOid << finalize,
                            document{} << "$push" << open_document <<
                                    "sentence" <<
                                        open_document <<
                                            "userId" << userId <<
                                            "date" <<  time(NULL) <<
                                            "chatStr" << chatStr <<
                                        close_document << 
                                    close_document << finalize);
    if(!addChatSentenceToChatInfoResult) {
        cout << "add chat sentence to chat info failure" << endl;
    }
    
    // brodcast chat message to users
    auto userList = roomList.smembers(chatId).get().as_array();

    for(auto user : userList) {
        auto result = conList.get(user.as_string()).get();
        if(result.is_integer()) { // user enter
            Controller* con = reinterpret_cast<Controller*>(result.as_integer());
            con->sendMessageProducer(chatStr);
            cout << "message brodcast : " << chatId << " - " << userId << endl;
        }
        else { // user exit
            roomList.srem(chatId, vector<string>{user.as_string()});
        }
    }
}