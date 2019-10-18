#include <sys/socket.h>
#include <arpa/inet.h>

#include <array>
#include <deque>

#include "utils.hpp"

std::string sockaddrToString(const sockaddr &sa) {
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

std::string getFileExtension(const std::string &path) {
    auto dot = path.rfind('.');
    return dot == std::string::npos ? "" : path.substr(dot);
}

std::string canonicalizeURI(const std::string &uri) {
    if (uri.empty() || uri[0] != '/') {
        return "";
    }

    std::deque<std::string> stack;
    std::string::size_type i = 0;
    while (i != uri.size()) {
        auto j = uri.find('/', i + 1);
        if (j == std::string::npos) {
            j = uri.size();
        }
        auto s = uri.substr(i + 1, j - (i + 1));
        i = j;

        if (s == "." || s == "") {
            continue;
        } else if (s == "..") {
            if (stack.empty()) {
                return "";
            } else {
                stack.pop_back();
            }
        } else {
            stack.push_back(std::move(s));
        }
    }

    std::string ret;
    for (auto &s:stack) {
        ret += '/';
        ret += s;
    }
    return ret.empty() ? "/" : ret;
}

std::string timeToHTTPString(const time_t &t) {
    std::array<char, 35> buf{};
    tm tp;
    gmtime_r(&t, &tp);
    strftime(buf.data(), buf.size(), "%a, %d %b %Y %H:%M:%S %Z GMT", &tp);
    return buf.data();
}
