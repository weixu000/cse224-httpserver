#include <sys/socket.h>
#include <deque>
#include <system_error>

#include "HTTPResponse.hpp"

HTTPResponse::HTTPResponse(const std::string &http_ver, const std::string &code, const std::string &desc)
        : header(http_ver + " " + code + " " + desc + "\r\n") {
    set("Server", "httpd 0.0.0"); // Server is required
}

HTTPResponse &HTTPResponse::set(const std::string &key, const std::string &value) {
    header += key + ": " + value + "\r\n";
    return *this;
}

void HTTPResponse::send(int sock) {
    header += "\r\n";
    if (::send(sock, header.data(), header.size(), 0) == -1) {
        throw std::system_error(errno, std::system_category(), "send() error");
    }
    header.clear();
}

void HTTPResponse::send400ClientError(int sock, bool close) {
    HTTPResponse res("HTTP/1.1", "400", "Client Error");
    res.set("Content-Length", "0").set("Connection", close ? "close" : "keep-alive");
    res.send(sock);
}


void HTTPResponse::send404NotFound(int sock, bool close) {
    HTTPResponse res("HTTP/1.1", "404", "Not Found");
    res.set("Content-Length", "0").set("Connection", close ? "close" : "keep-alive");
    res.send(sock);
}
