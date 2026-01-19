#include <string>
#include "../include/StompProtocol.h"

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



    return frame; //empty frame
    error = "invalid user command";
}


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
