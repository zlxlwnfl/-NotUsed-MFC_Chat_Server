#pragma once

#include "Service.h"
#include "RoomManager.h"
#include <jsoncpp/json/json.h>
#include <cstdint>
#include <ctime>
#include <vector>

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

string Service::timeFormat(time_t t) {
    struct tm* tmp;
    struct tm* now = localtime_r(&t, tmp);
    char buf[80];
    string result;

    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", now);
    result = buf;

    delete tmp, now;
    
    return result;
}

string Service::signUp(const string recvMessage) {
    Json::Reader reader;
    Json::FastWriter fastWriter;
    Json::Value recvValue;
    Json::Value sendValue;
    reader.parse(recvMessage, recvValue);

    string userId = recvValue["userId"].asString();
    string userPassword = recvValue["userPassword"].asString();
    string userPasswordCheck = recvValue["userPasswordCheck"].asString();

    mongocxx::collection col = db["userInfo"];

    // is id overlap? (with mongodb)
        //return overlap message
    bsoncxx::stdx::optional<bsoncxx::document::value> overlapIdResult =
        col.find_one(document{} << "userId" << userId << finalize);
    if(overlapIdResult) {
        sendValue["type"] = 0;
        return fastWriter.write(sendValue);
    }

    // add user info to db
        // return sucess message
    bsoncxx::stdx::optional<mongocxx::result::insert_one> createUserInfoResult =
        col.insert_one(document{} << "userId" << userId <<
                                    "userPassword" << userPassword <<
                                    "userNickname" << userId << finalize);
    if(createUserInfoResult) {
        sendValue["type"] = 1;
        return fastWriter.write(sendValue);
    }

    cout << "sign up error" << endl;

    // return error message
    return "";
}

string Service::signIn(const string recvMessage) {
    Json::Reader reader;
    Json::FastWriter fastWriter;
    Json::Value recvValue;
    Json::Value sendValue;
    reader.parse(recvMessage, recvValue);

    string userId = recvValue["userId"].asString();
    string userPassword = recvValue["userPassword"].asString();

    mongocxx::collection col = db["userInfo"];

    // is id and password right? (with mongodb)
    bsoncxx::stdx::optional<bsoncxx::document::value> rightUserInfoResult =
        col.find_one(document{} << "userId" << userId <<
                                "userPassword" << userPassword << finalize);
    if(rightUserInfoResult) { // login success
        sendValue["type"] = 3;
        ID = userId;
        return fastWriter.write(sendValue);
    }else { // login failure
        sendValue["type"] = 2;
        return fastWriter.write(sendValue);
    }

    cout << "sign in error" << endl;

    // return error message
    return "";
}

string Service::createChat(const string recvMessage) {
    Json::Reader reader;
    Json::FastWriter fastWriter;
    Json::Value recvValue;
    Json::Value sendValue;
    reader.parse(recvMessage, recvValue);

    string chatStr = recvValue["chatStr"].asString();

    mongocxx::collection userCol = db["userInfo"];
    mongocxx::collection chatCol = db["chatInfo"];

    string chatId;

    // create chat document to chat col
    auto createChatInfoResult =
        chatCol.insert_one(document{} << "type" << "public" <<
                                    "participant" << open_array << 
                                        ID << close_array <<
                                    "sentence" << open_array << open_document <<
                                        "userId" << ID <<
                                        "date" <<  time(NULL) <<
                                        "chatStr" << chatStr <<
                                        close_document << close_array << finalize);
    if(!createChatInfoResult) {
        cout << "create chat info failure" << endl;
        return "";
    }else {
        chatId = createChatInfoResult->inserted_id().get_oid().value.to_string();
    }

    // add chat document to user col
    auto addChatListToUserInfoResult =
        userCol.update_one(document{} << "userId" << ID << finalize,
                            document{} << "$push" << open_document <<
                                    "userChatList" <<
                                        chatId <<
                                    close_document << finalize);
    if(!addChatListToUserInfoResult) {
        cout << "add chat list to user info failure" << endl;
        return "";
    }

    RoomManager::getInstance().enterNewRoom(chatId, ID);
    
    // return success message
    return chatId;
}

string Service::getChat(const string recvMessage) {
    Json::Reader reader;
    Json::Value recvValue;
    reader.parse(recvMessage, recvValue);

    string chatId = recvValue["chatId"].asString();

    return chatId;
}

void Service::getChatSentence(const string recvMessage) {
    Json::Reader reader;
    Json::Value recvValue;
    reader.parse(recvMessage, recvValue);

    string chatId = recvValue["chatId"].asString();
    string chatStr = recvValue["chatStr"].asString();

    cout << "recvMessage parsing after" << endl;

    RoomManager::getInstance().messageBrodcast(chatId, ID, chatStr);
}

string Service::userChatList(const string recvMessage) {
    Json::Reader reader;
    Json::FastWriter fastWriter;
    Json::Value recvValue;
    Json::Value sendValue;
    reader.parse(recvMessage, recvValue);

    mongocxx::collection userCol = db["userInfo"];
    mongocxx::collection chatCol = db["chatInfo"];

    // find user info from user col
    auto findUserInfoResult =
        userCol.find_one(document{} << "userId" << ID << finalize);
    if(!findUserInfoResult) {
        cout << "find user info failure" << endl;
        return "";
    }

    reader.parse(bsoncxx::to_json(*findUserInfoResult), recvValue);
    if(!recvValue["userChatList"]) {
        cout << "user chat list empty" << endl;
        return "";
    }

    Json::Value userChatList = recvValue["userChatList"];

    vector<string> chatIdVec;

    for(Json::Value::ArrayIndex i = 0; i != userChatList.size(); i++) {
        chatIdVec.push_back(userChatList[i].asString());
    }

    RoomManager::getInstance().setRoomList(chatIdVec, ID);

    Json::Value tmp;

    // find chat info from chat col
    for(string chatId : chatIdVec) {
        bsoncxx::oid chatOid(chatId);

        auto findChatInfoResult =
            chatCol.find_one(document{} << "_id" <<  chatOid << finalize);
        if(!findChatInfoResult) {
            cout << "find chat info failure" << endl;
            return "";
        }

        reader.parse(bsoncxx::to_json(*findChatInfoResult), tmp);
        sendValue["chatInfo"].append(tmp);
    }

    // create sendMessage
    sendValue["type"] = 5;
    
    return fastWriter.write(sendValue);
}