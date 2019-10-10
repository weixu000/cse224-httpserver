#include <sys/socket.h>
#include <unistd.h>
#include <deque>
#include <sys/stat.h>
#include <sys/sendfile.h>

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

void doGET(int sock, const HTTPRequest &req, const std::string &root_dir) {
    spdlog::info("responding to GET");

    if (req.URI().empty() || req.URI()[0] != '/') {
        send400ClientError(sock, false);
        return;
    }

    std::deque<std::string> stack;
    std::string::size_type i = 0;
    while (i != req.URI().size()) {
        auto j = req.URI().find('/', i + 1);
        if (j == std::string::npos) {
            j = req.URI().size();
        }

        auto s = req.URI().substr(i + 1, j - (i + 1));
        if (s == ".") {
            continue;
        } else if (s == "..") {
            if (stack.empty()) {
                send400ClientError(sock, false);
                return;
            } else {
                stack.pop_back();
            }
        } else {
            stack.push_back(std::move(s));
        }
        i = j;
    }

    auto path = root_dir;
    for (auto &s:stack) {
        path += '/';
        path += s;
    }

    spdlog::info("GET file path: {}", path);

    if (access(path.c_str(), R_OK) == -1) {
        send404NotFound(sock, false);
        return;
    }

    // Check if the path exists
    struct stat buf{};
    if (stat(path.c_str(), &buf) == -1) {
        send404NotFound(sock, false);
        return;
    }

    // Find index.html if it is a directory
    if (S_ISDIR(buf.st_mode)) {
        path += "/index.html";
        if (stat(path.c_str(), &buf) == -1) {
            send404NotFound(sock, false);
            return;
        }
    }

    // Check if it is regular file
    if (!S_ISREG(buf.st_mode)) {
        send404NotFound(sock, false);
        return;
    }

    auto fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        send404NotFound(sock, false);
        return;
    }

    sendResponseHeader(sock,
                       "HTTP/1.1", "200", "OK",
                       {field_pair_t("Connection", "keep-alive"),
                        field_pair_t("Content-Length", std::to_string(buf.st_size))});
    while (true) {
        auto s = sendfile(sock, fd, nullptr, buf.st_size);
        if (s == buf.st_size) {
            break;
        } else if (s == EAGAIN) {
            spdlog::error("sendfile() failed with EAGAIN");
            continue;
        } else {
            spdlog::error("sendfile() error: {}", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    close(fd);
}
