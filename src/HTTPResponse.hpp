#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <string>

#include "HTTPRequest.hpp"


void sendResponseHeader(int sock,
                        const std::string &http_ver, const std::string &code, const std::string &desc,
                        std::initializer_list<field_pair_t> fields);

void send400ClientError(int sock, bool close);

void send404NotFound(int sock, bool close);

using mimes_t = std::map<std::string, std::string>;

void doGET(int sock, const HTTPRequest &req,
           const std::string &root_dir, const mimes_t &mimes);

#endif //HTTPRESPONSE_HPP
