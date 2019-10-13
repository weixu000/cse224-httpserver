#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>

#include "HTTPRequest.hpp"

class HTTPResponse {
public:
    HTTPResponse() = default;

    HTTPResponse(const std::string &http_ver, const std::string &code, const std::string &desc);

    HTTPResponse &set(const std::string &key, const std::string &value);

    void send(int sock);

    static void send400ClientError(int sock, bool close);

    static void send404NotFound(int sock, bool close);

private:
    std::string header;
};

#endif //HTTPRESPONSE_HPP
