#pragma once

#include "Service.h"
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

string Service::TimeFormat(time_t t) {
    struct tm* tmp;
    struct tm* now = localtime_r(&t, tmp);
    char buf[80];
    string result;

    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", now);
    result = buf;

    delete tmp, now;
    
    return result;
}

string Service::SignUp(string& recvMessage) {
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
        /*
        cout << "db userInfo list" << endl;
    
        mongocxx::cursor cursor = col.find({});
        for(auto doc : cursor) {
            cout << bsoncxx::to_json(doc) << endl;
        }
        */
    
        sendValue["type"] = 1;
        return fastWriter.write(sendValue);
    }

    cout << "sign up error" << endl;

    // return error message
    return "";
}

string Service::SignIn(string& recvMessage) {
    Json::Reader reader;
    Json::FastWriter fastWriter;
    Json::Value recvValue;
    Json::Value sendValue;
    reader.parse(recvMessage, recvValue);

    string userId = recvValue["userId"].asString();
    string userPassword = recvValue["userPassword"].asString();

    mongocxx::collection col = db["userInfo"];

    // is id and password right? (with mongodb)
        //return overlap message
    bsoncxx::stdx::optional<bsoncxx::document::value> rightUserInfoResult =
        col.find_one(document{} << "userId" << userId <<
                                "userPassword" << userPassword << finalize);
    if(rightUserInfoResult) { // login success
        sendValue["type"] = 3;
        return fastWriter.write(sendValue);
    }else { // login failure
        sendValue["type"] = 2;
        return fastWriter.write(sendValue);
    }

    cout << "sign in error" << endl;

    // return error message
    return "";
}

string Service::CreateChat(string& recvMessage) {
    Json::Reader reader;
    Json::FastWriter fastWriter;
    Json::Value recvValue;
    Json::Value sendValue;
    reader.parse(recvMessage, recvValue);

    string userId = recvValue["userId"].asString();
    string chatStr = recvValue["chatStr"].asString();

    mongocxx::collection userCol = db["userInfo"];
    mongocxx::collection chatCol = db["chatInfo"];

    string chatId;

    // create chat document to chat col
    auto createChatInfoResult =
        chatCol.insert_one(document{} << "type" << "public" <<
                                    "participant" << open_array << 
                                        userId << close_array <<
                                    "sentence" << open_array << open_document <<
                                        "userId" << userId <<
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
        userCol.update_one(document{} << "userId" << userId << finalize,
                            document{} << "$push" << open_document <<
                                    "userChatList" <<
                                        chatId <<
                                    close_document << finalize);
    if(!addChatListToUserInfoResult) {
        cout << "add chat list to user info failure" << endl;
        return "";
    }
    
    // return success message
    return chatId;
}

string Service::GetChat(string& recvMessage) {
    Json::Reader reader;
    Json::Value recvValue;
    reader.parse(recvMessage, recvValue);

    string chatId = recvValue["chatId"].asString();

    return chatId;
}

void Service::GetChatSentence(string& recvMessage) {
    Json::Reader reader;
    Json::Value recvValue;
    reader.parse(recvMessage, recvValue);

    string chatId = recvValue["chatId"].asString();
    string userId = recvValue["userId"].asString();
    string chatStr = recvValue["chatStr"].asString();

    cout << "recvMessage parsing after" << endl;

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
}

string Service::SendChatData(string chatId, long lastReadTime, map<string, long>& chatListMap) {
    Json::Reader reader;
    Json::FastWriter fastWriter;
    Json::Value recvValue;
    Json::Value sendValue;

    mongocxx::collection chatCol = db["chatInfo"];

    bsoncxx::oid chatOid(chatId);

    // get chat info from chat col
    auto getChatInfoResult =
        chatCol.find_one(document{} << "_id" <<  chatOid << finalize);
    if(!getChatInfoResult) {
        cout << "get chat info failure" << endl;
        return "";
    }

    // json parsing
    reader.parse(bsoncxx::to_json(*getChatInfoResult), recvValue);
    Json::Value sentence = recvValue["sentence"];

    sendValue["type"] = 4;
    sendValue["chatId"] = chatId;
    //sendValue["sentence"] = Json::Value(Json::arrayValue);

    Json::Value tmp;

    // get chat data before lastReadTime
    for(Json::Value::ArrayIndex i = 0; i != sentence.size(); i++) {
        if(sentence[i]["date"].asLargestInt() > lastReadTime) {
            cout << "sentence[i] time : " << sentence[i]["date"].asLargestInt() << endl;
            cout << "lastReadTime : " << lastReadTime << endl;

            tmp["userId"] = sentence[i]["userId"].asCString();
            tmp["date"] = sentence[i]["date"].asLargestInt();
            tmp["chatStr"] = sentence[i]["chatStr"].asCString();
            sendValue["sentence"].append(tmp);
        }
    }

    // send data nothing
    if(sendValue["sentence"].empty())
        return "";
    
    // last read time change to now time
    chatListMap[chatId] = time(NULL);

    // return success message
    return fastWriter.write(sendValue);
}

string Service::UserChatList(string& recvMessage) {
    Json::Reader reader;
    Json::FastWriter fastWriter;
    Json::Value recvValue;
    Json::Value sendValue;
    reader.parse(recvMessage, recvValue);

    string userId = recvValue["userId"].asString();

    mongocxx::collection userCol = db["userInfo"];
    mongocxx::collection chatCol = db["chatInfo"];

    // find user info from user col
    auto findUserInfoResult =
        userCol.find_one(document{} << "userId" << userId << finalize);
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