#pragma once
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include "../include/ConnectionHandler.h"
#include "../include/event.h"

#pragma once
#include <string>
#include <vector>
#include <map>
//=============================================================================================================================
//frame object instaed of annoying strings
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
//=============================================================================================================================
//this class is for summary user command. 
//any message frame the server is sending will be saved here
class Game {
private:
    std::string teamA;
    std::string teamB;
    //maps of <statName, statValue>: for lex order
    std::map<std::string, std::string> generalStats; 
    std::map<std::string, std::string> teamAStats;
    std::map<std::string, std::string> teamBStats;
    //vector of events for chronological order
    //pair<Event, username of the sender> 
    //according to instructions:"you should save reports from different users separately"
    std::vector<std::pair<Event, std::string>> events;

//input: the string that was recievd at the body of the MESSAGE frame was sent from server
//output:event object with info, and the user that sent the event
std::pair<Event, std::string> buildEvent(const std::string& body) {
        std::string teamA, teamB, eventName, description,sender;
        int time = 0;
        std::map<std::string, std::string> generalUpdates, aUpdates, bUpdates;
        std::stringstream stream(body);
        std::string line;
        std::string currentSection = "";
        while (std::getline(stream, line)) {
            //not sure if needed, something about deleting \r that stuck in the ened of the string
            if (!line.empty() && line.back() == '\r') line.pop_back();
            //saving string info into temp varaibales in order to build event
            if (line.find("user: ") == 0) sender = line.substr(6);
            if (line.find("team a: ") == 0) teamA = line.substr(8);
            else if (line.find("team b: ") == 0) teamB = line.substr(8);
            else if (line.find("event name: ") == 0) eventName = line.substr(12);
            else if (line.find("time: ") == 0) time = std::stoi(line.substr(6));
            else if (line == "general game updates:") currentSection = "general";
            else if (line == "team a updates:") currentSection = "team_a";
            else if (line == "team b updates:") currentSection = "team_b";
            else if (line == "description:") currentSection = "description";
            
            else if (currentSection == "description") {
                description += line + "\n";
            }
            else if (line.find(':') != std::string::npos) { //we found
                size_t split = line.find(':');
                std::string key = line.substr(0, split);
                std::string val = line.substr(split + 1);
                //remove sapce after ":" char :
                if (val.length() > 0 && val[0] == ' ') val = val.substr(1);
                if (currentSection == "general") generalUpdates[key] = val;
                else if (currentSection == "team_a") aUpdates[key] = val;
                else if (currentSection == "team_b") bUpdates[key] = val;
            }
        }
        Event event(teamA, teamB, eventName, time, generalUpdates, aUpdates, bUpdates, description);
        return {event,sender}; //in order for us to identify in summary
    }


public:
    Game(std::string a, std::string b) : teamA(a), teamB(b) {}
    Game() : teamA(""), teamB("") {}

    //we will call this function when server send MESSAGE Frame in order for summary to be updated
    void pushMessageFrame(const std::string& body) {
        std::pair<Event, std::string> result = buildEvent(body);
        Event& event = result.first;
        if (teamA.empty()) teamA = event.get_team_a_name();
        if (teamB.empty()) teamB = event.get_team_b_name();
        //save event for summary
        events.push_back(result);//event
        //update stats
        for (auto const& pair : event.get_game_updates()) {
            generalStats[pair.first] = pair.second;
        }
        for (auto const& pair : event.get_team_a_updates()) {
            teamAStats[pair.first] = pair.second;
        }
        for (auto const& pair : event.get_team_b_updates()) {
            teamBStats[pair.first] = pair.second;
        }
    }
    //gets user and return string rperesenting updated from user for the current game
    //we will use this function to print summary into file when we get summarize user cmd
    //as required in the format in the instruction
    std::string getSummary(const std::string& requestedUser) {
        std::string output = teamA + " vs " + teamB + "\n";
        output += "Game stats:\nGeneral stats:\n";
        for (const auto& pair : generalStats) output += pair.first + ": " + pair.second + "\n";

        output += teamA + " stats:\n";
        for (const auto& pair : teamAStats) output += pair.first + ": " + pair.second + "\n";

        output += teamB + " stats:\n";
        for (const auto& pair : teamBStats) output += pair.first + ": " + pair.second + "\n";

        output += "Game event reports:\n";
        for (const auto& pair : events) {
            const Event& event = pair.first;
            const std::string& sender = pair.second;
            if(sender == requestedUser){
                output += std::to_string(event.get_time()) + " - " + event.get_name() + ":\n\n";
                output += event.get_discription() + "\n\n\n";
            }
        }
        return output;
    }
};


//=============================================================================================================================
//actual protocol
class StompProtocol
{
private:
int subscriptionIdCounter;
int receiptIdCounter;
int receiptDisconnectCounter; //help us identify when we have to logout
std::string currentUsername;
bool isConnected;

//map sub Ids in order to send frames as required <gamename aka topic , subID>
std::map<std::string, int> gameToSubId;

//this map connect recipt that we get from server and printing it by handle server frame
std::map<int, std::string> receiptActions; //map recipt by recipt id that the server give to recipt

//map games into game object we created in order to keep stats ete. 
//see Game class above
std::map<std::string, Game> games;

public:
StompProtocol() : subscriptionIdCounter(0), receiptIdCounter(0),receiptDisconnectCounter(-1), currentUsername(""), isConnected(false) {}

//isconnected getter for logout
bool getConnected() {
    return isConnected;
}

//gets user command
//translate it to Frame which readable for server. 
//in case there is error that has to be printed without connecting to server it, save it in error object that recieved as paramete
//cmd is & for efficienty, error bc were chaning it so we have to
Frame userCmdToFrame(std::string& cmd,std::string& error);

//gets report command and return vector of send frames for the server
//save error in case of any error
std::vector<Frame> parseReportFile(const std::string& filename,std::string& error);

//make frame from event 
Frame buildFrameFromEvent(const Event& event);

//gets answerFrame from server and handle it according to instructions
std::string handleServerFrame(std::string& serverFrame);

//split command into frame object:
Frame splitFrame(std::string& msg);

};

