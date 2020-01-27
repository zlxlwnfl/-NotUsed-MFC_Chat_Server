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
    string SignUp(string& str);
    string SignIn(string& str);
};