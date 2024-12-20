#include "session.hpp"

#include <sys/socket.h>
#include <sys/signal.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <sstream>
#include <vector>
#include <string>

#include "../utils/utils.hpp"

#define MAX_BUFSIZE 1024

Session::Session(const std::string& service_addr, short service_port)
{
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0) {
        throw SessionError("Session creation failed: socket isn't created.");
    }

    addr.sin_family = AF_INET;
    if(inet_aton(service_addr.c_str(), &(addr.sin_addr)) < 0) {
        throw SessionError("Invalid service IP address.");
    }
    addr.sin_port = htons(service_port);


    std::ostringstream oss;
    oss << service_addr << ":" << service_port;

    service_info.ip_address = service_addr;
    service_info.port = service_port;
    service_info.endpoint = oss.str();
}

Session::~Session()
{ terminate(); }

void Session::connect_to_service()
{
    if(connect(sock, (const sockaddr*) &addr, sizeof(addr)) < 0) {
        throw SessionError("Connecting to the service failed.");
    }
}

void Session::terminate()
{
    close(sock);

    std::cout << "Session terminated.\n";
}

void Session::send_data(const std::string& data)
{
    std::vector<std::string> segments = chunks(data, MAX_BUFSIZE);
    int total = 0;

    int bytes = 0;
    for(int i = 0; i < segments.size(); i++) {
        bytes += write(sock, segments[i].c_str(), segments[i].size());
    }
    
    // tells that sending of segments is finished.
    bytes += write(sock, "\n\n", 2);
    if(bytes < total + 2) {
        throw SessionError("Not the entire data was sent. Sending data failed.");
    }
}

std::string Session::receive_data()
{
    std::string result = "";
    char buffer[MAX_BUFSIZE];
    
    while(true) {
        int bytes = read(sock, buffer, MAX_BUFSIZE);

        if(bytes < 0) {
            throw SessionError("Something went wrong upon getting response from the server.");
        }
        else if(bytes == 0) {
            throw SessionError("Server has closed the connection.");
        }
        else {
            std::string temp(buffer, bytes);
            result += temp;

            // Indicates that the entire data is fully received.
            if(result[result.size() - 1] == '\n' && result[result.size() - 2] == '\n')
            {
                result = std::string(result.begin(), result.end() - 2);
                break;
            }
        }
    }

    return result;
}