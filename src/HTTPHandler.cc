#include <sys/socket.h>
#include <unistd.h>
#include <spdlog/spdlog.h>
#include <sys/sendfile.h>

#include "HTTPHandler.hpp"
#include "HTTPRequest.hpp"
#include "HTTPResponse.hpp"
#include "utils.hpp"

HTTPHandler::HTTPHandler(const HttpdServer &s, int fd, const sockaddr &addr)
        : sock(fd), peer_addr(addr), server(s) {
    timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
        throw std::system_error(errno, std::system_category(), "setsockopt() error");
    }
}

HTTPHandler::~HTTPHandler() {
    close(sock);
    spdlog::info("Closed {}", sockaddrToString(peer_addr));
}

void HTTPHandler::serve() {
    spdlog::info("Serving {}", sockaddrToString(peer_addr));
    while (true) {
        try {
            const auto request = HTTPRequest(sock);
            if (request.method() == "GET") {
                doGET(request);
            } else {
                send400ClientError(sock, true);
                return;
            }
        } catch (const BadError &) {
            send400ClientError(sock, true);
            return;
        } catch (const IncompleteError &) {
            send400ClientError(sock, true);
            return;
        } catch (const TimeoutError &) {
            spdlog::error("Timeout before receiving request");
            return;
        } catch (const EmptyError &) {
            spdlog::error("Connection closed before receiving request");
            return;
        } catch (const std::system_error &e) {
            if (e.code().value() == EPIPE) {
                spdlog::error("Connection closed while sending");
                return;
            } else {
                throw;
            }
        }
    }
}

void HTTPHandler::doGET(const HTTPRequest &req) {
    auto uri = canonicalizeURI(req.URI());
    if (uri.empty()) {
        send400ClientError(sock, false);
        return;
    }
    auto path = server.docRoot() + uri;
    spdlog::info("GET file path: {}", path);

    // Check if the path exists
    struct stat st{};
    if (stat(path.c_str(), &st) == -1) {
        send404NotFound(sock, false);
        return;
    }

    // Find index.html if it is a directory
    if (S_ISDIR(st.st_mode)) {
        path += "/index.html";
        if (stat(path.c_str(), &st) == -1) {
            send404NotFound(sock, false);
            return;
        }
    }

    // Check if it is regular file
    if (!S_ISREG(st.st_mode)) {
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
                            field_pair_t("Content-Length", std::to_string(st.st_size)),
                            field_pair_t("Content-Type", server.mimeTypes().at(ext)),
                            field_pair_t("Last-Modified", timeToHTTPString(st.st_mtime)),
                            field_pair_t("Cache-Control", "no-cache")});
    } else {
        sendResponseHeader(sock,
                           "HTTP/1.1", "200", "OK",
                           {field_pair_t("Connection", "keep-alive"),
                            field_pair_t("Content-Length", std::to_string(st.st_size)),
                            field_pair_t("Last-Modified", timeToHTTPString(st.st_mtime)),
                            field_pair_t("Cache-Control", "no-cache")});
    }

    while (true) {
        auto s = sendfile(sock, fd, nullptr, st.st_size);
        if (s == st.st_size) {
            break;
        } else {
            throw std::system_error(errno, std::system_category(), "sendfile() error");
        }
    }

    close(fd);
}