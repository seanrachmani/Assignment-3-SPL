#include <string>
#include "../include/StompProtocol.h"
#include "../include/event.h"
#include <fstream>

//input thread:
Frame StompProtocol::userCmdToFrame(std::string& cmd,std::string& error) {
    //split user cmd 
    std::stringstream stream (cmd);
    std::string actualCmd;
    stream >> actualCmd;
    Frame frame;
    frame.command = ""; //starting with empty frame

    if(actualCmd =="login"){
        std::string hostPort, username, password;
        stream >> hostPort >> username >> password;
        if (isConnected){
            error = "The client is already logged in, log out before trying again";
            return frame;
        } 
        currentUsername = username;
        frame.command = "CONNECT";
        frame.headers["accept-version"] = "1.2";
        frame.headers["host"] = "stomp.cs.bgu.ac.il";
        frame.headers["login"] = username;
        frame.headers["passcode"] = password;
        return frame;
    }
    if (!isConnected) {
        error = "You must log in first";
        return frame; 
    }

    if(actualCmd == "join"){
        std::string gameName;
        stream >> gameName;
        int id = subscriptionIdCounter;
        subscriptionIdCounter++;
        int receipt = receiptIdCounter;
        receiptIdCounter++;
        gameToSubId[gameName] = id;
        //saving the recipts for handleServer to print
        receiptActions[receipt] = "Joined channel " + gameName;
        frame.command = "SUBSCRIBE";
        frame.headers["destination"] = "/" + gameName; 
        frame.headers["id"] = std::to_string(id);
        frame.headers["receipt"] = std::to_string(receipt);
        return frame;
    }

    if(actualCmd == "exit"){
        std::string gameName;
        stream >> gameName;
        if (gameToSubId.find(gameName) == gameToSubId.end()) {
            error = "You are not subscribed to " + gameName;
            return frame; //empty
         }
         int subscriptionId = gameToSubId[gameName];
         int receipt = receiptIdCounter;
        receiptIdCounter++;
        receiptActions[receipt] = "Exited channel " + gameName;
        frame.command = "UNSUBSCRIBE";
        frame.headers["id"] = std::to_string(subscriptionId); 
        frame.headers["receipt"] = std::to_string(receipt);   
        return frame;
    }

    
    if(actualCmd == "logout"){
        if(!isConnected){
            error = "user is not logged in";
            return frame; //empty
        }
        frame.command = "DISCONNECT";
        receiptIdCounter ++;
        receiptDisconnectCounter = receiptIdCounter;
        frame.headers["receipt"] = std::to_string(receiptIdCounter);
    }

    
    if (actualCmd == "summary") {
        std::string gameName, user, file;
        stream >> gameName >> user >> file;
        if (games.find(gameName) == games.end()) {
            error = "No data found for game " + gameName;
            return frame; //empty frame
        }
        //see get summary function in games calss in stompProtocl.h
        std::string summary = games[gameName].getSummary(user);

        //write to file summary output:
        //create empy file or clean the file we found
        std::ofstream outFile(file); 
        //check we can access file
        if (!outFile.is_open()) {
            error = "Failed to create or open file: " + file;
            return frame;
        }
        //put summary string into file
        outFile << summary;
        outFile.close();

       //we want to return empty frame bc no need to send server anything
        return frame; 
    }


    //we didnt recgonized valid user command
     error = "invalid user command";
    return frame; //empty frame
   
}

//this function handeles user report cmd
std::vector<Frame> StompProtocol::parseReportFile(const std::string& filename, std::string& error) {
    std::vector<Frame> frames;
    if (!isConnected) {
        error = "You must log in first";
        return frames;
    }

    try {
        names_and_events data = parseEventsFile(filename); 
        for (const Event& event : data.events) {
            frames.push_back(buildFrameFromEvent(event));
        }
    } catch (...) { //catch any exception
        error = "Failed to parse file";
    }
    return frames;
}

//take the parser was given to us and make frame out of it using our frame struct in order for server to read
Frame StompProtocol::buildFrameFromEvent(const Event& event) {
    Frame frame;
    frame.command = "SEND";
    frame.headers["destination"] = "/" + event.get_team_a_name() + "_" + event.get_team_b_name();

    std::string body = "";
    body += "user: " + currentUsername + "\n";
    body += "team a: " + event.get_team_a_name() + "\n";
    body += "team b: " + event.get_team_b_name() + "\n";
    body += "event name: " + event.get_name() + "\n";
    body += "time: " + std::to_string(event.get_time()) + "\n";
    
    body += "general game updates:\n";
    for (const auto& pair : event.get_game_updates()) {
        body += pair.first + ": " + pair.second + "\n";
    }

    body += "team a updates:\n";
    for (const auto& pair : event.get_team_a_updates()) {
        body += pair.first + ": " + pair.second + "\n";
    }

    body += "team b updates:\n";
    for (const auto& pair : event.get_team_b_updates()) {
        body += pair.first + ": " + pair.second + "\n";
    }

    body += "description:\n";
    body += event.get_discription();
    
    frame.body = body;
    return frame;
}
//=============================================================================================================
//socket thread:
std::string StompProtocol::handleServerFrame(std::string& serverFrame){
    Frame frame = splitFrame(serverFrame);
    if (frame.command == "CONNECTED") {
        isConnected = true;
        return "Login successful";
    }

    if (frame.command == "ERROR") {
        isConnected = false;
        return frame.headers["message"];
    }
    
    if (frame.command == "RECEIPT") {
        if (frame.headers.count("receipt-id")) { //there is header like this 
            int receiptId = std::stoi(frame.headers["receipt-id"]); //stoi - string to int
            //logout receipt:
            if(receiptDisconnectCounter != -1 && receiptId == receiptDisconnectCounter){
                subscriptionIdCounter = 0;
                receiptIdCounter = 0;
                currentUsername = "";
                isConnected = false;
                receiptDisconnectCounter = -1;
                //no concerns for memory leaks since we have no pointer fields
                gameToSubId.clear();
                games.clear();
                receiptActions.clear();
                //not sure if we have to print logout msg but whatever:
                return "You have successfully logged out";
            }
            //join&exit
            //did we put in userCMDtoFrame something to send?
            if (receiptActions.count(receiptId)) {
                std::string action = receiptActions[receiptId];
                receiptActions.erase(receiptId); //keep our recipt structure updated 
                if (action.find("Joined channel") != std::string::npos) {
                    //we find it
                    return action; 
                }
                if (action.find("Exited channel") != std::string::npos) {
                //keep gameTosub updated by erasing while subscribing
                std::string gameName = action.substr(15); 
                if (gameToSubId.count(gameName)) {
                    gameToSubId.erase(gameName);
                }
                return action; 
                }
            }
        }
    }
    
    if (frame.command == "MESSAGE") {
        std::string gameName = frame.headers["destination"];
        //remove / charr
        if (gameName.length() > 0 && gameName[0] == '/') {
            gameName = gameName.substr(1);
        }
        //save into games.events 
        games[gameName].pushMessageFrame(frame.body);
        return "Received update for " + gameName + ":\n" + frame.body;
        return "";
    }
    //if we got till here the command is not recgonized

    return "";   
}


//split answer frame that was accepted from server
Frame StompProtocol::splitFrame(std::string& msg){
    Frame frame;
    std::stringstream stream(msg);
    std::string currentLine;
    //put first line as frame.command:
    std::getline(stream, frame.command);

    //read headers:
    while (std::getline(stream, currentLine) && currentLine != ""){ //aslong as the line isnt empty
        size_t split = currentLine.find(':');
        size_t splitHeader = currentLine.find(':');
        if(splitHeader != std::string::npos){//we find :
            std::string key = currentLine.substr(0, split); //header name
            std::string val = currentLine.substr(split + 1);
            frame.headers[key] = val;
        }
    }

    //body:
    char c;
    while (stream.get(c)) {
        if (c != '\0') 
            frame.body += c;
    }
    return frame;
}

