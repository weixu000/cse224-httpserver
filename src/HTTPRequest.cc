#include <sys/socket.h>
#include <sstream>

#include "HTTPRequest.hpp"
#include "logger.hpp"

HTTPRequest::HTTPRequest(int sock) {
    std::string header;
    std::array<char, 100> buf{};
    while (true) {
        auto len = recv(sock, buf.data(), buf.size() - 1, MSG_DONTWAIT);
        if (len == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            } else {
                logger()->error("recv() error");
                exit(EXIT_FAILURE);
            }
        } else {
            header.append(buf.data(), len);
            if (header.size() >= 4 && header.substr(header.size() - 4, 4) == "\r\n\r\n") {
                break;
            } else if (len == 0) {
                logger()->error("socket closed but header incomplete");
                return;
            }
        }
    }
    logger()->info(header);

    std::istringstream ss(header);
    std::string line;
    if (std::getline(ss, line) && line.size() && line[line.size() - 1] == '\r') {
        auto i = line.find(' ');
        auto j = line.find(' ', i + 1);
        if (i == std::string::npos || j == std::string::npos) {
            logger()->error("Bad request initial line: {}", line);
            return;
        }
        _method = line.substr(0, i);
        _uri = line.substr(i + 1, j - (i + 1));
        _HTTPVer = line.substr(j + 1, line.size() - 1 - (j + 1));
    } else {
        logger()->error("Bad request initial line: {}", line);
        return;
    }
    logger()->info("Method: {}, URI: {}, Version: {}", _method, _uri, _HTTPVer);

    while (std::getline(ss, line) && line.size() && line[line.size() - 1] == '\r') {
        if (line == "\r") {
            break;
        }
        auto i = line.find(": ");
        if (i == std::string::npos) {
            logger()->error("Bad request header fields: {}", line);
            *this = HTTPRequest();
            return;
        }
        std::string key, value;
        key = line.substr(0, i);
        value = line.substr(i + 2, line.size() - 1 - (i + 2));
        _fields[key] = value;
        logger()->info("[{}] = {}", key, value);
    }
    if (line != "\r") {
        logger()->error("Bad request initial line.");
        *this = HTTPRequest();
        return;
    }
}
