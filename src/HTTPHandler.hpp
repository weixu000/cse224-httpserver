#ifndef HTTPHANDLER_HPP
#define HTTPHANDLER_HPP

#include "HttpdServer.hpp"
#include "HTTPResponse.hpp"

class EmptyError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

class TimeoutError : EmptyError {
    using EmptyError::EmptyError;
};

class IncompleteError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

class HTTPHandler {
public:
    HTTPHandler(const HttpdServer &s, int fd, const sockaddr &addr);

    virtual ~HTTPHandler();

    void serve();

private:
    int sock;
    sockaddr peer_addr;
    static const unsigned int timeout = 5;

    std::string frameHeader();

    std::string frame_remainder;

    const HttpdServer &server;

    HTTPResponse doGET(const HTTPRequest &req);
};

#endif //HTTPHANDLER_HPP
