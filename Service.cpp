#pragma once

#include "Service.h"
#include <jsoncpp/json/json.h>
#include <cstdint>

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

string Service::SignUp(string& str) {
    Json::Reader reader;
    Json::FastWriter fastWriter;
    Json::Value recvValue;
    Json::Value sendValue;
    reader.parse(str, recvValue);

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

string Service::SignIn(string& str) {
    Json::Reader reader;
    Json::FastWriter fastWriter;
    Json::Value recvValue;
    Json::Value sendValue;
    reader.parse(str, recvValue);

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