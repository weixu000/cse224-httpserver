#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>

#include "HTTPRequest.hpp"

class HTTPResponse {
public:
    HTTPResponse() = default;

    HTTPResponse(const std::string &http_ver, const std::string &code, const std::string &desc);

    virtual ~HTTPResponse();

    HTTPResponse &set(const std::string &key, const std::string &value);

    HTTPResponse &setBody(int fd, off_t sz);

    void send(int sock);

    static void send400ClientError(int sock, bool close);

    static void send404NotFound(int sock, bool close);

private:
    std::string header;

    off_t body_size = 0;
    int body_fd = -1;
};

#endif //HTTPRESPONSE_HPP
