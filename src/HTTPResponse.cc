#include <sys/socket.h>
#include <unistd.h>
#include <sys/sendfile.h>
#include <deque>
#include <system_error>

#include "HTTPResponse.hpp"

HTTPResponse::HTTPResponse(const std::string &http_ver, const std::string &code, const std::string &desc)
        : header(http_ver + " " + code + " " + desc + "\r\n\r\n") {
    set("Server", "httpd 0.0.0"); // Server is required
}

HTTPResponse::~HTTPResponse() {
    close(body_fd);
}

HTTPResponse &HTTPResponse::set(const std::string &key, const std::string &value) {
    header.insert(header.size() - 2, key + ": " + value + "\r\n");
    return *this;
}

HTTPResponse &HTTPResponse::setBody(int fd, off_t sz) {
    close(body_fd);
    body_fd = fd;
    body_size = sz;
    return *this;
}

void HTTPResponse::send(int sock) {
    if (::send(sock, header.data(), header.size(), 0) == -1) {
        throw std::system_error(errno, std::system_category(), "send() error");
    }

    if (body_fd == -1) {
        return;
    }

    off_t offset = 0;
    while (offset < body_size) {
        auto s = sendfile(sock, body_fd, &offset, body_size - offset);
        if (s == -1) {
            throw std::system_error(errno, std::system_category(), "sendfile() error");
        } else {
            offset += s;
        }
    }
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
