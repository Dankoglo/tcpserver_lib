#ifndef SESSION_HPP
#define SESSION_HPP

#include <string>
#include <exception>

#include <arpa/inet.h>

class Session {
    int sock;
    struct sockaddr_in addr;

    void terminate();
public:
    struct ServiceInfo {
        std::string ip_address;
        short port;

        std::string endpoint;
    } service_info;

    class SessionError : public std::exception {
        std::string msg;
    public:
        SessionError(const std::string& _msg)
            :msg(_msg)
        {}

        const char* what() const noexcept
        { return msg.c_str(); }
    };

    Session(const std::string&, short);

    Session(Session&) = delete;
    Session(const Session&) = delete;
    Session(Session&&) = delete;

    Session& operator=(const Session&) = delete;

    ~Session();

    void connect_to_service();

    void send_data(const std::string&);
    std::string receive_data();
};


#endif // SESSION_HPP