#include <sysexits.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "spdlog/spdlog.h"

#include "HttpdServer.hpp"
#include "HTTPRequest.hpp"

namespace {
    std::string sockaddrToString(const struct sockaddr &sa) {
        switch (sa.sa_family) {
            case AF_INET: {
                auto addr_v4 = reinterpret_cast<const sockaddr_in *>(&sa);
                std::array<char, INET_ADDRSTRLEN> buf{};
                inet_ntop(AF_INET, &addr_v4->sin_addr, buf.data(), buf.size());
                return std::string(buf.data()) + ":" + std::to_string(ntohs(addr_v4->sin_port));
            }

            case AF_INET6: {
                auto addr_v6 = reinterpret_cast<const sockaddr_in6 *>(&sa);
                std::array<char, INET6_ADDRSTRLEN> buf{};
                inet_ntop(AF_INET6, &addr_v6->sin6_addr, buf.data(), buf.size());
                return std::string(buf.data()) + ":" + std::to_string(ntohs(addr_v6->sin6_port));
            }

            default:
                return "Unknown AF";
        }
    }

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
        }
    }

    void handleConection(int sock) {
        while (true) {
            const auto request = HTTPRequest(sock);
            if (request.bad()) {
                sendResponseHeader(sock,
                                   "HTTP/1.1", "400", "Client Error",
                                   {field_pair_t("Connection", "close")});
                spdlog::info("Response 400");
                return;
            }

            sendResponseHeader(sock,
                               "HTTP/1.1", "404", "Not Found",
                               {field_pair_t("Connection", "keep-alive"),
                                field_pair_t("Content-Length", "0")});
            spdlog::info("Response 404");
        }
    }
}

HttpdServer::HttpdServer(const INIReader &config) {
    port = config.Get("httpd", "port", "");
    if (port.empty()) {
        spdlog::error("port was not in the config file");
        exit(EX_CONFIG);
    }

    doc_root = config.Get("httpd", "doc_root", "");
    if (doc_root.empty()) {
        spdlog::error("doc_root was not in the config file");
        exit(EX_CONFIG);
    }
}

void HttpdServer::launch() {
    spdlog::info("Launching web server");
    spdlog::info("Port: {}", port);
    spdlog::info("doc_root: {}", doc_root);

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

        handleConection(peer_sock);

        close(peer_sock);
        spdlog::info("Closed {}", sockaddrToString(peer_addr));
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
