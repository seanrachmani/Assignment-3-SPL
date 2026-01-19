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
	public:
	//constructot
	InputClass(ConnectionHandler& handler, StompProtocol& protocol,std::mutex& mutex)
	 : _handler(handler), _protocol(protocol), _mutex(mutex) {}

	void run(){
		while(1){
			//read input and wrap in buffer
			const short bufsize = 1024;
			char buf[bufsize];
			std::cin.getline(buf, bufsize);
			std::string line(buf);
			std::stringstream stream(line);
        	std::string command;
        	stream >> command;
			if(command=="report"){//we need to return lots of send frames
				std::string filename;
				stream >> filename;
				std::string error;
				std::vector<Frame> frames;
				{
					std::lock_guard<std::mutex> lock(_mutex);
					frames = _protocol.parseReportFile(filename, error);
				}
				if (error != "") std::cout << error << std::endl;
				for (Frame& f : frames) {
					std::string fString = f.toString();
					_handler.sendLine(fString);
				} 
			}
			else { //any other user commands
            std::string error;
            Frame frame;
            {
                std::lock_guard<std::mutex> lock(_mutex);
                frame = _protocol.userCmdToFrame(line, error);
            }

            if (error != "") std::cout << error << std::endl;
				if (frame.command != "") {
					std::string frameString = frame.toString();
					if (!_handler.sendLine(frameString)) {
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
				if (!_handler.getLine(answer)) {
				std::cout << "Disconnected. Exiting...\n" << std::endl;
				break;
			}
			//remove /n
			int len = answer.length();
            answer.resize(len - 1);

			//protocol is shared resource
			//synchronized
			{
			std::lock_guard<std::mutex> lock(_mutex);
			std::cout << _protocol.handleServerFrame(answer);
			}
		}
	}
};

int main(int argc, char *argv[]) {
	//check args
	if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " host port" << std::endl << std::endl;
        return -1;
    }
    std::string host = argv[1];
    short port = atoi(argv[2]);
    
	//create ch and protocol
    ConnectionHandler connectionHandler(host, port);
	StompProtocol protocol;
	std::mutex mutex;

	//try to conenct to server
	if (!connectionHandler.connect()) {
        std::cerr << "Could not connect to server" << std::endl;
        return 1;
    }
	std::cout << "Connected to the server!" << std::endl;

	//tasks and threads:
	//create tasks:
	SocketClass socketTask(connectionHandler, protocol, mutex);
    InputClass inputTask(connectionHandler, protocol, mutex);
	
	//threads
	std::thread t1(&SocketClass::run, &socketTask);
    std::thread t2(&InputClass::run, &inputTask);
	t1.join();
	t2.join();
	return 0;

}