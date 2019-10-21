#include <sstream>

#include "HTTPRequest.hpp"

HTTPRequest::HTTPRequest(const std::string &header) {
    parseHeader(header);
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
        if (key.empty() || value.empty()) {
            throw BadError("Bad request header fields");
        }
        _fields[key] = value;
    }
    if (line != "\r") {
        throw BadError("No empty line at end of header");
    }

    if (!_fields.count("Host")) {
        throw BadError("Host is required");
    }

    if (_HTTPVer != "HTTP/1.1") {
        throw BadError("HTTP/1.1 is required");
    }
}
