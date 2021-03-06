#include <cerrno>
#include <sys/socket.h>
#include <unistd.h>
#include <spdlog/spdlog.h>
#include <sys/sendfile.h>

#include "HTTPHandler.hpp"
#include "HTTPRequest.hpp"
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
            const auto req = HTTPRequest(frameHeader());
            if (req.method() == "GET") {
                auto close = req.fields().count("Connection") && req.fields().at("Connection") == "close";
                doGET(req).set("Connection", close ? "close" : "keep-alive").send(sock);
                if (close) {
                    return; // client want close, so we close
                }
            } else {
                HTTPResponse::ClientError().set("Connection", "close").send(sock);
                return;
            }
        } catch (const BadError &) {
            HTTPResponse::ClientError().set("Connection", "close").send(sock);
            return;
        } catch (const IncompleteError &) {
            HTTPResponse::ClientError().set("Connection", "close").send(sock);
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

HTTPResponse HTTPHandler::doGET(const HTTPRequest &req) {
    std::string path;
    try {
        path = normalizeURI(req.URI(), server.docRoot());
    } catch (std::invalid_argument &e) {
        spdlog::error("Incorrect request URI: {}", req.URI());
        return HTTPResponse::ClientError();
    } catch (std::system_error &e) {
        return HTTPResponse::NotFound();
    }
    spdlog::info("GET file path: {}", path);

    // Check if the path exists
    struct stat st{};
    if (stat(path.c_str(), &st) == -1) {
        return HTTPResponse::NotFound();
    }

    // Find index.html if it is a directory
    if (S_ISDIR(st.st_mode)) {
        path += "/index.html";
        if (stat(path.c_str(), &st) == -1) {
            return HTTPResponse::NotFound();
        }
    }

    // Check if it is regular file
    if (!S_ISREG(st.st_mode)) {
        return HTTPResponse::NotFound();
    }

    auto fd = open(path.c_str(), O_RDONLY);
    if (fd == -1) {
        return HTTPResponse::NotFound();
    }

    auto ext = getFileExtension(path);
    HTTPResponse res("HTTP/1.1", "200", "OK");
    res.set("Last-Modified", timeToHTTPString(st.st_mtime))
            .set("Content-Length", std::to_string(st.st_size));
    if (server.mimeTypes().count(ext)) {
        res.set("Content-Type", server.mimeTypes().at(ext));
    } else {
        spdlog::error("Unkown MIME type");
        res.set("Content-Type", "application/octet-stream");
    }
    res.setBody(fd, st.st_size);
    return res;
}

std::string HTTPHandler::frameHeader() {
    std::string header = std::move(frame_remainder);
    std::array<char, 100> buf{};

    auto delimiter = header.find("\r\n\r\n");
    while (delimiter == std::string::npos) {
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
            auto pos = header.size() <= std::string::size_type(len) + 3 ? 0 : header.size() - len - 3;
            delimiter = header.find("\r\n\r\n", pos);
        }
    }
    frame_remainder = header.substr(delimiter + 4);
    header.erase(delimiter + 4);
    return header;
}