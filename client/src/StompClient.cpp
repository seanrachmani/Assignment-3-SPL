#include <stdlib.h>
#include <iostream>
#include <thread>
#include <vector>   
#include <sstream>  
#include "../include/ConnectionHandler.h"
#include "../include/StompProtocol.h"

//task 1 by thread 1 - reading from the keyboard
//reading input and sending it to the server
class InputClass{
	private:
	ConnectionHandler& _handler;
    StompProtocol& _protocol;
	std::mutex& _mutex;
	std::string& _login_line;
	public:
	//constructot
	InputClass(ConnectionHandler& handler, StompProtocol& protocol,std::mutex& mutex,std::string& loginLine)
	 : _handler(handler), _protocol(protocol), _mutex(mutex) ,_login_line(loginLine){}

	void run(){
		while(1){
			{
				//catch when we logged out
                std::lock_guard<std::mutex> lock(_mutex);
                if (!_protocol.getConnected()) {
                    break;
                }
            }
			//read input and wrap in buffer
			const short bufsize = 1024;
			char buf[bufsize];
			std::cin.getline(buf, bufsize);
			std::string line(buf);
			{
            std::lock_guard<std::mutex> lock(_mutex);
            if (!_protocol.getConnected()){
				//we want to break in order for main to handle login oart
				_login_line = line;
				break;
				}
            }

			std::stringstream stream(line);
        	std::string command;
        	stream >> command;
			if (command == "login") {
                std::cout << "The client is already logged in, log out before trying again" << std::endl;
                continue;
            }
			if(command=="report"){//we need to return lots of send frame
				std::string filename;
				stream >> filename;
				std::string error;
				std::vector<Frame> frames;
				{
					std::lock_guard<std::mutex> lock(_mutex);
					frames = _protocol.parseReportFile(filename, error);
				}
				if (error != "") std::cout << error << std::endl;
				else{
					std::cout << "Report file parsed successfully! Sending " << frames.size() << " events" << std::endl;
				}
				for (Frame& f : frames) {
					std::string fString = f.toString();
					bool sendLine = _handler.sendBytes(fString.c_str(), fString.length());
					if (!sendLine) {
						std::cout << "Disconnected..." << std::endl;
						break;
					}
				} 
			}
			else { //any other user commands
            std::string error;
            Frame frame;
            {
                std::lock_guard<std::mutex> lock(_mutex);
                frame = _protocol.userCmdToFrame(line, error);
            }

            if (error != ""){
				 std::cout << error << std::endl;
			}
			if (frame.command != "") { //we want to send only relevant frames
				std::string frameString = frame.toString();
				bool sendLine = _handler.sendBytes(frameString.c_str(), frameString.length());
				if (!sendLine) {
					std::cout << "Disconnected..." << std::endl;
					break;
					}
				}
			}
		}
	}
};


//task 2 by thread 2 - reading from the socket and print relevant details according to the frame recieved from server
class SocketClass{
	private:
	ConnectionHandler& _handler;
    StompProtocol& _protocol;
	std::mutex& _mutex;
	public:
	//constructot
	SocketClass(ConnectionHandler& handler, StompProtocol& protocol,std::mutex& mutex)
	 : _handler(handler), _protocol(protocol), _mutex(mutex) {}

	//run
	void run(){
		while(1){
			std::string answer;
				if (!_handler.getFrameAscii(answer, '\0')) {
				std::cout << "Disconnected. Exiting...\n" << std::endl;
				{
                    std::lock_guard<std::mutex> lock(_mutex);
                    _protocol.setConnected(false);
                }
				break;
			}

			//protocol is shared resource
			//synchronized
			{
			std::lock_guard<std::mutex> lock(_mutex);
			std::string	result = _protocol.handleServerFrame(answer);
			if (!result.empty()) {
        		std::cout << result << std::endl;
    		}
			if(_protocol.getConnected() == false){
				_handler.close();
				break;
			}
			}
		}
	}
};

int main(int argc, char *argv[]){
	std::string loginLine = "";
    while(1){
		//login handle
		//try to read from user.if you cant break
		//we got login from inout task
		std::string cmd;
		if(loginLine!=""){
			cmd = loginLine;
			loginLine = "";
		}
		else{
			const short bufsize = 1024;
			char buf[bufsize];
			std::cin.getline(buf, bufsize);
			cmd = buf;
		}
        std::stringstream stream(cmd);
        std::string command;
        stream >> command;
        if (command == "login") {
            std::string hostPort;
            stream >> hostPort; 
			//split host port:
			size_t index = hostPort.find(':');
			if(index == std::string::npos){
				std::cout << "Invalid host:port format.Use host:port" << std::endl;
				continue;
			}

			std::string host = hostPort.substr(0, index);
			std::string portString = hostPort.substr(index+1);
			short port = (short)std::stoi(portString);

            //create ch and protocol
    		ConnectionHandler connectionHandler(host, port);
			//try to conenct to server
			if (!connectionHandler.connect()) {
				std::cerr << "Could not connect to server" << std::endl;
				continue;
			}
			StompProtocol protocol;
			std::mutex mutex;
			std::string error;
			Frame connectFrame;
			
			//we dont need a lock since we havne created other thread yet
            connectFrame = protocol.userCmdToFrame(cmd, error);
            if (error != ""){
				 std::cout << error << std::endl;
			}
			if (connectFrame.command == "CONNECT") {
				std::string frameString = connectFrame.toString();
				bool sendLine = connectionHandler.sendBytes(frameString.c_str(), frameString.length());
				if (!sendLine) {
					std::cout << "Disconnected..." << std::endl;
					continue;
				}

				std::string answer;
                if (!connectionHandler.getFrameAscii(answer, '\0')) {
                    std::cout << "Disconnected while waiting for login response." << std::endl;
                    continue;
                }

                std::string result = protocol.handleServerFrame(answer);
                std::cout << result << std::endl; 
					if (protocol.getConnected()) {
					//tasks and threads:
					//create tasks:
					SocketClass socketTask(connectionHandler, protocol, mutex);
					InputClass inputTask(connectionHandler, protocol, mutex,loginLine);
					
					//threads
					//socket thread:
					std::thread t1(&SocketClass::run, &socketTask);
					//main thread which started reading from the keyboard is keeping doing it.
					inputTask.run();
					t1.join();
				}
			}
         }
		 else{
			std::cout << "You must log in first" << std::endl;
		 }
	}
	return 0;
}
	




