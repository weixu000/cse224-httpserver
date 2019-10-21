#ifndef UTILS_HPP
#define UTILS_HPP

#include <sys/socket.h>

#include <string>

std::string sockaddrToString(const sockaddr &sa);

std::string getFileExtension(const std::string &path);

std::string normalizeURI(const std::string &uri, const std::string &root);

std::string expandPath(std::string path);

std::string timeToHTTPString(const time_t &t);

#endif //UTILS_HPP
