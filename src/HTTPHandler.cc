#include <sys/socket.h>
#include <unistd.h>
#include <spdlog/spdlog.h>
#include <deque>
#include <sys/sendfile.h>

#include "HTTPHandler.hpp"
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
#include "utils.hpp"

HTTPHandler::HTTPHandler(const HttpdServer &s, int fd, const sockaddr &addr)
        : sock(fd), peer_addr(addr), server(s) {

}

HTTPHandler::~HTTPHandler() {
    close(sock);
    spdlog::info("Closed {}", sockaddrToString(peer_addr));
}

void HTTPHandler::serve() {
    spdlog::info("Started to serve {}", sockaddrToString(peer_addr));
    while (true) {
        const auto request = HTTPRequest(sock);
        if (request.bad()) {
            send400ClientError(sock, true);
            return;
        }

        if (request.method() == "GET") {
            doGET(request);
            continue;
        } else {
            send400ClientError(sock, true);
            return;
        }
    }
}

void HTTPHandler::doGET(const HTTPRequest &req) {
    spdlog::info("responding to GET");

    auto uri = canonicalizeURI(req.URI());
    if (uri.empty()) {
        send400ClientError(sock, false);
        return;
    }
    auto path = server.docRoot() + uri;
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

    auto ext = getFileExtension(path);
    if (server.mimeTypes().count(ext)) {
        sendResponseHeader(sock,
                           "HTTP/1.1", "200", "OK",
                           {field_pair_t("Connection", "keep-alive"),
                            field_pair_t("Content-Length", std::to_string(buf.st_size)),
                            field_pair_t("Content-Type", server.mimeTypes().at(ext))});
    } else {
        sendResponseHeader(sock,
                           "HTTP/1.1", "200", "OK",
                           {field_pair_t("Connection", "keep-alive"),
                            field_pair_t("Content-Length", std::to_string(buf.st_size))});
    }

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