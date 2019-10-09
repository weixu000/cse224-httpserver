#include <sysexits.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sstream>

#include "logger.hpp"
#include "HttpdServer.hpp"
#include "HTTPRequest.hpp"

namespace {
    std::string sockaddrToString(const struct sockaddr &sa) {
        ostringstream ss;
        switch (sa.sa_family) {
            case AF_INET: {
                auto addr_v4 = reinterpret_cast<const sockaddr_in *>(&sa);
                std::array<char, INET_ADDRSTRLEN> buf{};
                inet_ntop(AF_INET, &addr_v4->sin_addr, buf.data(), buf.size());
                ss << buf.data() << ":" << ntohs(addr_v4->sin_port);
                break;
            }

            case AF_INET6: {
                auto addr_v6 = reinterpret_cast<const sockaddr_in6 *>(&sa);
                std::array<char, INET6_ADDRSTRLEN> buf{};
                inet_ntop(AF_INET6, &addr_v6->sin6_addr, buf.data(), buf.size());
                ss << "[" << buf.data() << "]:" << ntohs(addr_v6->sin6_port);
                break;
            }

            default:
                ss << "Unknown AF";
                break;
        }

        return ss.str();
    }

    void handleConection(int sock) {
        const auto request = HTTPRequest(sock);
        if (request.bad())
            return;

        std::string response = "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n";
        send(sock, response.data(), response.size(), 0);
    }
}

HttpdServer::HttpdServer(const INIReader &config) {
    auto log = logger();

    port = config.Get("httpd", "port", "");
    if (port.empty()) {
        log->error("port was not in the config file");
        exit(EX_CONFIG);
    }

    doc_root = config.Get("httpd", "doc_root", "");
    if (doc_root.empty()) {
        log->error("doc_root was not in the config file");
        exit(EX_CONFIG);
    }
}

void HttpdServer::launch() {
    auto log = logger();

    log->info("Launching web server");
    log->info("Port: {}", port);
    log->info("doc_root: {}", doc_root);

    serverListen();

    while (true) {
        sockaddr peer_addr{};
        socklen_t peer_addr_size = sizeof(sockaddr);
        int peer_sock = accept(sock, &peer_addr, &peer_addr_size);
        if (peer_sock == -1) {
            logger()->error("accept() error");
            exit(EXIT_FAILURE);
        }
        logger()->info("Accepted {}", sockaddrToString(peer_addr));

        handleConection(peer_sock);

        close(peer_sock);
        logger()->info("Closed {}", sockaddrToString(peer_addr));
    }
}

void HttpdServer::serverListen() {
    addrinfo hints{}, *result;
    hints.ai_socktype = SOCK_STREAM; /* Stream socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */

    int s = getaddrinfo(nullptr, port.c_str(), &hints, &result);
    if (s != 0) {
        logger()->error("getaddrinfo: {}", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    for (auto rp = result; rp; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype,
                      rp->ai_protocol);
        if (sock == -1)
            continue;

        if (bind(sock, rp->ai_addr, rp->ai_addrlen) == 0 && listen(sock, 10) == 0) {
            logger()->info("listening on {}", sockaddrToString(*rp->ai_addr));
            break;                  /* Success */
        }

        close(sock);
    }

    if (sock == -1) {               /* No address succeeded */
        logger()->error("Could not listen!");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);           /* No longer needed */
}

HttpdServer::~HttpdServer() {
    close(sock);
}
