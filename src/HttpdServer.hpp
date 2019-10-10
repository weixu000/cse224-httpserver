#ifndef HTTPDSERVER_HPP
#define HTTPDSERVER_HPP

#include "inih/INIReader.h"

#include "HTTPResponse.hpp"

class HttpdServer {
public:
    explicit HttpdServer(const INIReader &config);

    void launch();

    virtual  ~HttpdServer();

protected:
    std::string port;
    std::string doc_root;

    mimes_t mime_types;

    int sock = -1;

    void serverListen();
};

#endif // HTTPDSERVER_HPP
