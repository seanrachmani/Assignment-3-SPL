#pragma once
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include "../include/ConnectionHandler.h"

struct Frame {
    std::string command;
    std::map<std::string, std::string> headers; //<name,value>
    std::string body;
    std::string toString() {
        std::string res = command + "\n";
        for (const auto& pair : headers) {
            res += pair.first + ":" + pair.second + "\n";
        }
        res += "\n"; 
        res += body;
        res += '\0'; 
        return res;
    }
};

class StompProtocol
{
private:
int subscriptionIdCounter;
int receiptIdCounter;
std::string currentUsername;
bool isConnected;

//map sub Ids in order to send frames as required
std::map<std::string, int> gameToSubId;

//this map connect recipt that we get from server and printing it by handle server frame
std::map<int, std::string> receiptActions; //map recipt by recipt id that the server give to recipt

public:
StompProtocol() : subscriptionIdCounter(0), receiptIdCounter(0), currentUsername(""), isConnected(false) {}

//gets user command
//translate it to Frame which readable for server. 
//in case there is error that has to be printed without connecting to server it, save it in error object that recieved as paramete
//cmd is & for efficienty, error bc were chaning it so we have to
Frame userCmdToFrame(std::string& cmd,std::string& error);

//gets answerFrame from server and handle it according to instructions
std::string handleServerFrame(std::string& serverFrame);

//split command into frame object:
Frame splitFrame(std::string& msg);

};

