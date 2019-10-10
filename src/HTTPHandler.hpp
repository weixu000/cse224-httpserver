#ifndef HTTPHANDLER_HPP
#define HTTPHANDLER_HPP

#include "HttpdServer.hpp"
#include "HTTPRequest.hpp"

class HTTPHandler {
public:
    HTTPHandler(const HttpdServer &s, int fd, const sockaddr &addr);

    virtual ~HTTPHandler();

    void serve();

private:
    int sock;
    sockaddr peer_addr;

    const HttpdServer &server;

    void doGET(const HTTPRequest &req);
};

#endif //HTTPHANDLER_HPP
