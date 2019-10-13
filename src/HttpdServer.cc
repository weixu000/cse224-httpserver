#include <sysexits.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fstream>
#include <signal.h>

#include "spdlog/spdlog.h"

#include "HttpdServer.hpp"
#include "HTTPHandler.hpp"
#include "utils.hpp"

HttpdServer::HttpdServer(const INIReader &config) {
    port = config.Get("httpd", "port", "");
    if (port.empty()) {
        spdlog::error("port was not in the config file");
        exit(EX_CONFIG);
    }

    _doc_root = config.Get("httpd", "doc_root", "");
    if (_doc_root.empty()) {
        spdlog::error("doc_root was not in the config file");
        exit(EX_CONFIG);
    }

    auto mime_path = config.Get("httpd", "mime_types", "");
    if (mime_path.empty()) {
        spdlog::error("mime_types was not in the config file");
        exit(EX_CONFIG);
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
    spdlog::info("Port: {}", port);
    spdlog::info("doc_root: {}", _doc_root);

    serverListen();

    while (true) {
        sockaddr peer_addr{};
        socklen_t peer_addr_size = sizeof(sockaddr);
        int peer_sock = accept(sock, &peer_addr, &peer_addr_size);
        if (peer_sock == -1) {
            spdlog::error("accept() error");
            exit(EXIT_FAILURE);
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

    int s = getaddrinfo(nullptr, port.c_str(), &hints, &result);
    if (s != 0) {
        spdlog::error("getaddrinfo: {}", gai_strerror(s));
        exit(EXIT_FAILURE);
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
        spdlog::error("Could not listen!");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);           /* No longer needed */
}

HttpdServer::~HttpdServer() {
    close(sock);
}
