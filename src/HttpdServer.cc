#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fstream>
#include <utility>
#include <csignal>

#include "spdlog/spdlog.h"

#include "HttpdServer.hpp"
#include "HTTPHandler.hpp"
#include "utils.hpp"

HttpdServer::HttpdServer(std::string port, std::string root, const std::string &mime_path)
        : _port(std::move(port)), _doc_root(std::move(root)) {
    if (access(_doc_root.c_str(), R_OK) != 0) {
        throw std::invalid_argument("Cannot access doc_root.");
    }
    if (_doc_root.substr(_doc_root.size() - 1) == "/") {
        _doc_root.erase(_doc_root.size() - 1);
    }

    std::ifstream mime_fs(mime_path);
    std::string ext, mime;
    while (mime_fs >> ext >> mime) {
        _mime_types[ext] = mime;
    }

    // Client closing connection while server sending will send SIGPIPE and terminate program
    // Ignore SIGPIPE
    signal(SIGPIPE, SIG_IGN);
}

void HttpdServer::launch() {
    spdlog::info("Launching web server");
    spdlog::info("Port: {}", _port);
    spdlog::info("doc_root: {}", _doc_root);

    serverListen();

    while (true) {
        sockaddr peer_addr{};
        socklen_t peer_addr_size = sizeof(sockaddr);
        int peer_sock = accept(sock, &peer_addr, &peer_addr_size);
        if (peer_sock == -1) {
            throw std::system_error(errno, std::system_category(), "accept() error");
        }
        spdlog::info("Accepted {}", sockaddrToString(peer_addr));

        pool.addTask([this, peer_addr, peer_sock] {
            HTTPHandler handler(*this, peer_sock, peer_addr);
            handler.serve();
        });
    }
}

void HttpdServer::serverListen() {
    addrinfo hints{}, *result;
    hints.ai_socktype = SOCK_STREAM; /* Stream socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */

    int s = getaddrinfo(nullptr, _port.c_str(), &hints, &result);
    if (s != 0) {
        throw std::system_error(s, std::generic_category(), gai_strerror(s));
    }

    for (auto rp = result; rp; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype,
                      rp->ai_protocol);
        if (sock == -1)
            continue;

        if (bind(sock, rp->ai_addr, rp->ai_addrlen) == 0 && listen(sock, 10) == 0) {
            spdlog::info("listening on {}", sockaddrToString(*rp->ai_addr));
            break;                  /* Success */
        }

        close(sock);
    }

    if (sock == -1) {               /* No address succeeded */
        throw std::runtime_error("Could not listen!");
    }

    freeaddrinfo(result);           /* No longer needed */
}

HttpdServer::~HttpdServer() {
    close(sock);
}
