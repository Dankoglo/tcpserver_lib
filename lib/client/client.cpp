#include "client.hpp"

#include <sys/socket.h>
#include <sys/signal.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>

Client* Client::singleton = nullptr;

Client::Client()
    :session(nullptr), terminate_session(false)
{
    // forces write() to return EPIPE instead of generating signal SIGPIPE,
    // so write() can't block.
    signal(SIGPIPE, SIG_IGN);

    signal(SIGINT, signal_handler);
}

void Client::signal_handler(int signum)
{
    // prints the newline character after ^C.
    std::cout << std::endl;
    singleton->stop_session();

    // terminates waiting for input.
    kill(getpid(), SIGQUIT);
}

Client::~Client()
{ stop_session(); }

void Client::start_shell()
{
    system("clear");
    print_info();

    char delim = '\n';
    while(!terminate_session) {
        std::cout << "> ";
        
        std::string data;
        std::getline(std::cin, data, delim);
        if(delim == '#')
        // ignores the newline character after '#'
            std::cin.ignore();

        if(data == "exit" || !std::cin.good()) {
            terminate_session = true;
            continue;
        }
        else if(data == "multiline") {
            std::cout << "|------------------------------------------------------------|\n"
                      << "| You've activated multiline mode.                           |\n"
                      << "| In order to send your request complete the input with '#'. |\n"
                      << "|------------------------------------------------------------|\n";
            delim = '#';
            continue;
        }
        else if(data == "recover") {
            std::cout << "|-----------------------------------------------------------------|\n"
                      << "| You've recovered single line mode.                              |\n"
                      << "| In order to send your request enter the data and press 'Enter'. |\n"
                      << "|-----------------------------------------------------------------|\n";
            delim = '\n';
            continue;
        }

        if(terminate_session)
            return;

        try {
            session->send_data(data);
            std::string response = session->receive_data();
            std::cout << "Response: " << response << std::endl;
        }
        catch(const Session::SessionError& err) {
            std::string prefix = "Connection error: ";
            throw ClientError(prefix + err.what());
        }
    }
}

void Client::print_info()
{
    std::string dynamic_line = "| The server address is " + 
                                (session->service_info).endpoint + 
                                std::string(50 - (session->service_info).endpoint.size(), ' ') +
                                "|\n";

    std::cout << "|=========================================================================|\n"
              << "| Connection to the server is established.                                |\n"
              << dynamic_line
              << "|                                                                         |\n"
              << "| Client is ready. Now you can send your requests to the server.          |\n"
              << "| Enter your data or command and hit the 'Enter' to send it or execute it.|\n"
              << "| When a response is formed it will be printed.                           |\n"
              << "|                                                                         |\n"
              << "| Commands:                                                               |\n" 
              << "|     exit      - terminates session.                                     |\n"
              << "|     multiline - enables multiline mode.                                 |\n"
              << "|     recover   - recovers single-line mode.                              |\n"
              << "|=========================================================================|\n";
}

void Client::create_session(const std::string& service_addr, short service_port)
{
    try {
        session = new Session(service_addr, service_port);
        session->connect_to_service();
    }
    catch(const Session::SessionError& err) {
        std::string prefix = "Session creation failed: ";
        throw ClientError(prefix + err.what());
    }

    try {
        start_shell();
    }
    catch(const ClientError& err) {
        stop_session();

        throw err;
    }

    stop_session();
}

void Client::stop_session()
{
    // if(terminate_session)
    //     return;
        
    terminate_session = true;
    if(session) {
        delete session;
        session = nullptr;
    }
}