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
    static const unsigned int timeout = 5;

    const HttpdServer &server;

    bool doGET(const HTTPRequest &req);
};

#endif //HTTPHANDLER_HPP
