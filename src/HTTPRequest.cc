#include "HTTPRequest.hpp"

HTTPRequest::HTTPRequest(const std::string &header) {
    parseHeader(header);
}

void HTTPRequest::parseHeader(const std::string &header) {
    auto initial_line_end = header.find("\r\n");
    std::string line;
    if (initial_line_end != std::string::npos && initial_line_end > 0) {
        line = header.substr(0, initial_line_end);
        auto i = line.find(' ');
        auto j = line.find(' ', i + 1);
        if (i == std::string::npos || j == std::string::npos) {
            throw BadError("Bad request initial line.");
        }
        _method = line.substr(0, i);
        _uri = line.substr(i + 1, j - (i + 1));
        _HTTPVer = line.substr(j + 1, line.size() - (j + 1));
    } else {
        throw BadError("Empty request initial line.");
    }

    auto cur_line_start = initial_line_end + 2;
    std::string::size_type cur_line_end;
    std::string key, value;
    // there must be empty line here
    while ((cur_line_end = header.find("\r\n", cur_line_start)) != std::string::npos
           && cur_line_end > cur_line_start) {
        line = header.substr(cur_line_start, cur_line_end - cur_line_start);
        auto i = line.find(": ");
        if (i == std::string::npos) {
            throw BadError("Bad request header fields");
        }
        key = line.substr(0, i);
        value = line.substr(i + 2, line.size() - (i + 2));
        if (key.empty() || value.empty()) {
            throw BadError("Bad request header fields");
        }
        _fields[key] = value;
        cur_line_start = cur_line_end + 2;
    }
    if (!_fields.count("Host")) {
        throw BadError("Host is required");
    }

    if (_HTTPVer != "HTTP/1.1") {
        throw BadError("HTTP/1.1 is required");
    }
}
