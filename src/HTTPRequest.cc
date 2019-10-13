#include <sys/socket.h>
#include <sstream>

#include "spdlog/spdlog.h"

#include "HTTPRequest.hpp"

HTTPRequest::HTTPRequest(int sock) {
    std::string header;
    std::array<char, 100> buf{};
    while (true) {
        auto len = recv(sock, buf.data(), buf.size() - 1, MSG_DONTWAIT);
        if (len == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Timeout
                return;
            } else {
                spdlog::error("recv() error");
                exit(EXIT_FAILURE);
            }
        } else if (len == 0) {
            if (!header.empty()) {
                spdlog::error("socket closed but header incomplete");
            }
            return;
        } else {
            header.append(buf.data(), len);
            if (header.size() >= 4 && header.substr(header.size() - 4, 4) == "\r\n\r\n") {
                break;
            }
        }
    }

    std::istringstream ss(header);
    std::string line;
    if (std::getline(ss, line) && line.size() && line[line.size() - 1] == '\r') {
        auto i = line.find(' ');
        auto j = line.find(' ', i + 1);
        if (i == std::string::npos || j == std::string::npos) {
            spdlog::error("Bad request initial line: {}", line);
            return;
        }
        _method = line.substr(0, i);
        _uri = line.substr(i + 1, j - (i + 1));
        _HTTPVer = line.substr(j + 1, line.size() - 1 - (j + 1));
    } else {
        spdlog::error("Bad request initial line: {}", line);
        return;
    }

    while (std::getline(ss, line) && line.size() && line[line.size() - 1] == '\r') {
        if (line == "\r") {
            break;
        }
        auto i = line.find(": ");
        if (i == std::string::npos) {
            spdlog::error("Bad request header fields: {}", line);
            *this = HTTPRequest();
            return;
        }
        std::string key, value;
        key = line.substr(0, i);
        value = line.substr(i + 2, line.size() - 1 - (i + 2));
        _fields[key] = value;
    }
    if (line != "\r") {
        spdlog::error("Bad request initial line.");
        *this = HTTPRequest();
        return;
    }
}
