#ifndef SERVER_HPP
#define SERVER_HPP

#include <arpa/inet.h>

#include <vector>
#include <string>

#include "../pool/thread_pool.hpp"

/*
    Simple TCP server.

    Accepts the connections, handles the request using user-defined handler and
    can process the requests asynchronously and parallel as well.
*/

class TCPServer {
    static TCPServer* singleton;
    static void signal_handler(int);

    TCPServer(const std::string&, short port, int);

    int listener;
    struct sockaddr_in addr;

    struct ClientInfo {
        int clientfd;
        std::string ip_addr;
        unsigned short port;
    };
    std::vector<ClientInfo> clients;

    bool running;
    void stop();
    
    ThreadPool * pool;
    void parallel_run(int);

    void sequential_run();

    std::string form_request(const ClientInfo&);
    void send_response(const ClientInfo&, const std::string&);

    bool handler_set;
    std::function<std::string(const std::string&)> handler;
    void handle_request(const ClientInfo&);

    void print_info();
public:
    class TCPServerError : public std::exception {
        std::string msg;
    public:
        TCPServerError(const std::string& _msg)
            :msg(_msg)
        {}

        const char* what() const noexcept
        { return msg.c_str(); }
    };

    struct Info {
        std::string ip_address;
        short port;

        std::string endpoint;
    } info;

    static TCPServer* instantiate(const std::string& ip_addr = "127.0.0.1", 
                                  short port = INADDR_ANY,
                                  int backlog = 1)
    {
        if(!singleton) {
            singleton = new TCPServer(ip_addr, port, backlog);
        }
        return singleton;
    }

    TCPServer(TCPServer&) = delete;
    TCPServer(const TCPServer&) = delete;
    TCPServer(TCPServer&&) = delete;

    TCPServer& operator=(const TCPServer&) = delete;

    ~TCPServer();

    void run(bool, int num_of_threads = 1);

    void set_handler(std::function<std::string(const std::string&)>);
};


#endif // SERVER_HPP