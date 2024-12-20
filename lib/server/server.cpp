#include "server.hpp"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <netinet/in.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <iostream>
#include <sstream>
#include <exception>
#include <cmath>
#include <algorithm>

#include <thread>

#include "../utils/utils.hpp"

// Max size of a segment
#define MAX_BUFSIZE 1024

TCPServer* TCPServer::singleton = nullptr;

TCPServer::TCPServer(const std::string& ip_addr, short port, int backlog)
    :running(true), handler_set(false)
{
    listener = socket(AF_INET, SOCK_STREAM, 0);
    if(listener < 0) {
        throw TCPServerError("Listening socket creation failed.");
    }

    addr.sin_family = AF_INET;
    if(inet_aton(ip_addr.c_str(), &(addr.sin_addr)) == 0) {
        throw TCPServerError("Invalid server IP address.");
    }
    addr.sin_port = htons(port);

    if(bind(listener, (const struct sockaddr*) &addr, sizeof(addr)) < 0) {
        throw TCPServerError("Binding the socket with address failed.");
    }
    if(listen(listener, backlog) < 0) {
        throw TCPServerError(
            "Server can't listen on the given socket, perhaps the port is unavailable.");
    }

    signal(SIGINT, signal_handler);

    std::ostringstream oss;
    oss << ip_addr << ":" << port;

    info.ip_address = ip_addr;
    info.port = port;
    info.endpoint = oss.str();

    pool = new ThreadPool();
}

TCPServer::~TCPServer()
{
    if(running)
        stop();

    for(int i = 0; i < clients.size(); i++)
        close(clients[i].clientfd);
    
    if(pool->busy_threads() > 0)
        std::cout << "Waiting for connections to close..." << std::endl;

    delete pool;

    std::cout << "\n|=============================|\n"
                << "| Server is terminated.       |"
              << "\n|=============================|\n";
}

void TCPServer::signal_handler(int signum)
{
    singleton->stop();
}

void TCPServer::print_info()
{
    std::string dynamic_line = "| Server IPv4 listening address: " +
                                info.endpoint +
                                std::string(19 - info.endpoint.size(), ' ') +
                                "|\n";

    std::cout << "|===================================================|\n"
              << "| Server is running and ready to obtain requests.   |\n"
              << dynamic_line
              << "|                                                   |\n"
              << "| Printing log information is enabled.              |\n"
              << "| There are two types of logs:                      |\n"
              << "|  - obtained requests.                             |\n"
              << "|  - client's connections / disconnections.         |\n"
              << "|                                                   |\n"
              << "| To terminate the server use Ctrl+C.               |\n"
              << "|===================================================|\n";
}

void TCPServer::run(bool parallel, int num_of_threads)
{
    if(!handler_set) {
        throw TCPServerError("Server can't be started: request handler isn't set.");
    }
    system("clear");
    print_info();

    if(parallel)
        parallel_run(num_of_threads);
    else
        sequential_run();
}

void TCPServer::stop()
{
    if(pool->busy_threads() > 0) {
        std::cout << "\nWARNING: There are tasks that not finished yet." << std::endl;
        std::cout << "Do you want to force the terminataion of server? (y/n): ";
        char c;
        std::cin >> c;
        if(c == 'y') {
            std::cout << "\n|==================================|\n"
                        << "| Server is terminated forcefully. |"
                      << "\n|==================================|\n";
            exit(-1);
        }
    }

    running = false;
    close(listener);   
}

void TCPServer::sequential_run()
{
    fd_set readfds;
    int maxfd;

    while(running) {
        
        FD_ZERO(&readfds);
        FD_SET(listener, &readfds);
        maxfd = listener;
        
        for(auto client : clients) {
            FD_SET(client.clientfd, &readfds);
            maxfd = client.clientfd > maxfd ? client.clientfd : maxfd;
        }

        if(select(maxfd + 1, &readfds, NULL, NULL, NULL) < 0) {
            if(!running)
                return;

            throw TCPServerError("Clients polling is interrupted or something else went wrong.");
        }

        if(FD_ISSET(listener, &readfds)) {
            struct sockaddr_in client_addr;
            socklen_t client_addr_len = sizeof(client_addr);
            int client = accept(listener, (struct sockaddr*) &client_addr, &client_addr_len);

            std::string client_ip = std::string(inet_ntoa(client_addr.sin_addr));
            unsigned short client_port = ntohs(client_addr.sin_port);
            clients.push_back({client, client_ip, client_port});

            std::cout << "Client " << client_ip << ":" << client_port << " connected to the server.\n";
        }

        for(int i = 0; i < clients.size(); i++) {
            if(FD_ISSET(clients[i].clientfd, &readfds)) {
                try {
                    handle_request(clients[i]);
                }
                catch(const TCPServerError& err) {
                    std::cerr << err.what() << std::endl;
                    close(clients[i].clientfd);
                    clients.erase(clients.begin() + i); 
                }
            }
        }
    }

}

void TCPServer::parallel_run(int num_of_threads)
{
    pool->start(num_of_threads);

    while(running) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client = accept(listener, (struct sockaddr*) &client_addr, &client_addr_len);
        if(client < 0)
            break;

        std::string client_ip = std::string(inet_ntoa(client_addr.sin_addr));
        unsigned short client_port = ntohs(client_addr.sin_port);
        std::cout << "Client " << client_ip << ":" << client_port << " connected to the server.\n";

        // each thread has only one client descriptor, so
        // each thread can maintain only one connection and needs to
        // realese the obtained resources by itself.
        auto task = [=] {
            fd_set readfd;

            FD_SET(client, &readfd);

            while(true) {
                if(select(client + 1, &readfd, NULL, NULL, NULL) < 0) {
                    break;
                }

                try {
                    handle_request({client, client_ip, client_port});
                }
                catch(const TCPServerError& err) {
                    std::cerr << err.what() << std::endl;
                    close(client);
                }
            }

            close(client);
        };
        pool->execute_task(task);
    }
}

std::string TCPServer::form_request(const ClientInfo& client)
{
    std::string result = "";
    char buffer[MAX_BUFSIZE];
    
    while(true) {
        int bytes = read(client.clientfd, buffer, MAX_BUFSIZE);
        if(bytes < 0) {
            throw TCPServerError("Something went wrong upon forming the request.");
        }
        else if(bytes == 0) {
            std::ostringstream oss;
            oss << client.ip_addr << ":" << client.port;

            throw TCPServerError("Client " + oss.str() + " closed the connection.");
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

void TCPServer::send_response(const ClientInfo& client, const std::string& data)
{
    std::vector<std::string> segments = chunks(data, MAX_BUFSIZE);
    int total = 0;

    int bytes = 0;
    for(int i = 0; i < segments.size(); i++) {
        bytes += write(client.clientfd, segments[i].c_str(), segments[i].size());
        total += segments[i].size();
    }

    // tells that sending of segments is finished.
    bytes += write(client.clientfd, "\n\n", 2);
    if(bytes < total + 2) {
        throw TCPServerError("Not the entire response was sent. Sending response failed.");
    }
}

void TCPServer::handle_request(const ClientInfo& client)
{
    std::string data = form_request(client);
    std::cout << "Request from " << client.ip_addr << ":" << client.port << ": " << data << std::endl;

    std::string response = handler(data);
    send_response(client, response);
}

void TCPServer::set_handler(std::function<std::string(const std::string&)> _handler)
{
    handler_set = true;
    handler = _handler;
}