#include <sys/socket.h>
#include <arpa/inet.h>
#include <wordexp.h>

#include <array>
#include <system_error>

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

std::string normalizeURI(const std::string &uri, const std::string &root) {
    if (uri.empty() || uri[0] != '/') { // must starts with /
        throw std::invalid_argument("Incorrect uri");
    }

    auto path = root + uri;
    auto real_p = realpath(path.c_str(), nullptr);
    if (real_p == nullptr) {
        throw std::system_error(errno, std::system_category(), "realpath() error");
    }
    path = real_p;
    free(real_p);

    if (path.rfind(root, 0) != 0) {
        throw std::invalid_argument("URI escapes root");
    }

    return path;
}

std::string expandPath(std::string path) {
    if (!path.empty() && path[0] == '~') {
        // double quote the input except tilde
        // ~xxx/"xxx/xxx"
        auto tilde_slash = path.find('/');
        if (tilde_slash != std::string::npos) {
            path.insert(tilde_slash + 1, 1, '"');
            path.append(1, '"');
        }
    } else {
        // double quote the input
        path.insert(0, 1, '"');
        path.append(1, '"');
    }

    // tilde expansion and variable substitution
    wordexp_t p;
    auto err = wordexp(path.c_str(), &p, WRDE_NOCMD | WRDE_UNDEF);
    if (err != 0) {
        throw std::system_error(err, std::generic_category(), "wordexp() error");
    }
    if (p.we_wordc != 1) {
        throw std::invalid_argument("Incorrect path to expand");
    }
    path = p.we_wordv[0];
    wordfree(&p);

    // to absolute path and remove trailing slash
    auto real_p = realpath(path.c_str(), nullptr);
    if (real_p == nullptr) {
        // trailing slash of a path to regular file can throw
        throw std::system_error(errno, std::system_category(), "realpath() error");
    }
    path = real_p;
    free(real_p);

    return path;
}

std::string timeToHTTPString(const time_t &t) {
    std::array<char, 35> buf{};
    tm tp;
    gmtime_r(&t, &tp);
    strftime(buf.data(), buf.size(), "%a, %d %b %Y %H:%M:%S %Z", &tp);
    return buf.data();
}
