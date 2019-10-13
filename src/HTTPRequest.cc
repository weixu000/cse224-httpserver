#include <sys/socket.h>
#include <sstream>
#include <cerrno>

#include "HTTPRequest.hpp"

HTTPRequest::HTTPRequest(int sock) {
    auto header = frameHeader(sock);
    parseHeader(header);
}

std::string HTTPRequest::frameHeader(int sock) {
    std::string header;
    std::array<char, 100> buf{};
    while (true) {
        auto len = recv(sock, buf.data(), buf.size() - 1, 0);
        if (len == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (header.empty()) {
                    throw TimeoutError("Timeout before receiving any request.");
                } else {
                    throw IncompleteError("Timeout before receiving complete request.");
                }
            } else {
                throw std::system_error(errno, std::system_category(), "recv() error");
            }
        } else if (len == 0) {
            if (!header.empty()) {
                throw EmptyError("Socket closed before receiving any request.");
            } else {
                throw IncompleteError("Socket closed before receiving complete request.");
            }
        } else {
            header.append(buf.data(), len);
            if (header.size() >= 4 && header.substr(header.size() - 4, 4) == "\r\n\r\n") {
                break;
            }
        }
    }
    return header;
}

void HTTPRequest::parseHeader(const std::string &header) {
    std::istringstream ss(header);
    std::string line;
    if (std::getline(ss, line) && line.size() && line[line.size() - 1] == '\r') {
        auto i = line.find(' ');
        auto j = line.find(' ', i + 1);
        if (i == std::string::npos || j == std::string::npos) {
            throw BadError("Bad request initial line.");
        }
        _method = line.substr(0, i);
        _uri = line.substr(i + 1, j - (i + 1));
        _HTTPVer = line.substr(j + 1, line.size() - 1 - (j + 1));
    } else {
        throw BadError("Bad request initial line.");
    }

    while (std::getline(ss, line) && line.size() && line[line.size() - 1] == '\r') {
        if (line == "\r") {
            break;
        }
        auto i = line.find(": ");
        if (i == std::string::npos) {
            throw BadError("Bad request header fields");
        }
        std::string key, value;
        key = line.substr(0, i);
        value = line.substr(i + 2, line.size() - 1 - (i + 2));
        _fields[key] = value;
    }
    if (line != "\r") {
        throw BadError("No empty line at end of header");
    }
}
