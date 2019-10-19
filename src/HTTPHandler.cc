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
                if (doGET(request)) {
                    return;
                }
            } else {
                HTTPResponse::send400ClientError(sock, true);
                return;
            }
        } catch (const BadError &) {
            HTTPResponse::send400ClientError(sock, true);
            return;
        } catch (const IncompleteError &) {
            HTTPResponse::send400ClientError(sock, true);
            return;
        } catch (const TimeoutError &) {
            spdlog::error("Timeout before receiving request");
            return;
        } catch (const EmptyError &) {
            spdlog::error("Connection closed before receiving request");
            return;
        } catch (const std::system_error &e) {
            if (e.code().value() == EPIPE) {
                spdlog::error("Connection closed abruptly");
                return;
            } else {
                throw;
            }
        }
    }
}

bool HTTPHandler::doGET(const HTTPRequest &req) {
    auto close = false;
    if (req.fields().count("Connection") && req.fields().at("Connection") == "close") {
        close = true;
    }

    auto uri = canonicalizeURI(req.URI());
    if (uri.empty()) {
        HTTPResponse::send400ClientError(sock, close);
        return close;
    }
    auto path = server.docRoot() + uri;
    spdlog::info("GET file path: {}", path);

    // Check if the path exists
    struct stat st{};
    if (stat(path.c_str(), &st) == -1) {
        HTTPResponse::send404NotFound(sock, close);
        return close;
    }

    // Find index.html if it is a directory
    if (S_ISDIR(st.st_mode)) {
        path += "/index.html";
        if (stat(path.c_str(), &st) == -1) {
            HTTPResponse::send404NotFound(sock, close);
            return close;
        }
    }

    // Check if it is regular file
    if (!S_ISREG(st.st_mode)) {
        HTTPResponse::send404NotFound(sock, close);
        return close;
    }

    auto fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        HTTPResponse::send404NotFound(sock, close);
        return close;
    }

    auto ext = getFileExtension(path);
    HTTPResponse res("HTTP/1.1", "200", "OK");
    res.set("Last-Modified", timeToHTTPString(st.st_mtime))
            .set("Content-Length", std::to_string(st.st_size));
    if (server.mimeTypes().count(ext)) {
        res.set("Content-Type", server.mimeTypes().at(ext));
    }
    res.set("Connection", close ? "close" : "keep-alive");

    res.send(sock);

    while (true) {
        auto s = sendfile(sock, fd, nullptr, st.st_size);
        if (s == st.st_size) {
            break;
        } else {
            throw std::system_error(errno, std::system_category(), "sendfile() error");
        }
    }

    ::close(fd);
    return close;
}