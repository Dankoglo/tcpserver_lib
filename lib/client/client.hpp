#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "../session/session.hpp"

/*
    Creates session between client and server.
    Creates and maintains interactive shell for forming the requests.
*/

class Client {
    static Client* singleton;
    Client();

    Session* session;
    bool terminate_session;

    void start_shell();

    void stop_session();

    void print_info();

    static void signal_handler(int);
public:
    class ClientError : public std::exception {
        std::string msg;
    public:
        ClientError(const std::string& _msg)
            :msg(_msg)
        {}

        const char* what() const noexcept
        { return msg.c_str(); }
    };

    static Client* instantiate() {
        if(!singleton) {
            singleton = new Client();
        }

        return singleton;
    }

    Client(Client&) = delete;
    Client(const Client&) = delete;
    Client(Client&&) = delete;

    Client& operator=(const Client&) = delete;

    ~Client();

    void create_session(const std::string&, short);
};

#endif // CLIENT_HPP
