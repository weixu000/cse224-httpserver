#include <sys/socket.h>
#include <deque>

#include "spdlog/spdlog.h"

#include "HTTPResponse.hpp"

void sendResponseHeader(int sock,
                        const std::string &http_ver, const std::string &code, const std::string &desc,
                        std::initializer_list<field_pair_t> fields) {
    auto header = http_ver + " " + code + " " + desc + "\r\n";
    for (auto &it:fields) {
        header += it.first + ": " + it.second + "\r\n";
    }
    header += "\r\n";
    if (send(sock, header.data(), header.size(), 0) == -1) {
        spdlog::error("send() error");
        exit(EXIT_FAILURE);
    }
}

void send400ClientError(int sock, bool close) {
    if (close) {
        sendResponseHeader(sock,
                           "HTTP/1.1", "400", "Client Error",
                           {field_pair_t("Connection", "close")});
    } else {
        sendResponseHeader(sock,
                           "HTTP/1.1", "400", "Client Error",
                           {});
    }
}


void send404NotFound(int sock, bool close) {
    if (close) {
        sendResponseHeader(sock,
                           "HTTP/1.1", "404", "Not Found",
                           {field_pair_t("Connection", "close")});
    } else {
        sendResponseHeader(sock,
                           "HTTP/1.1", "404", "Not Found",
                           {field_pair_t("Connection", "keep-alive"),
                            field_pair_t("Content-Length", "0")});
    }
}
