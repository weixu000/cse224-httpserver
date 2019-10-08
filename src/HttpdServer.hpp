#ifndef HTTPDSERVER_HPP
#define HTTPDSERVER_HPP

#include "inih/INIReader.h"
#include "logger.hpp"

class HttpdServer {
public:
    explicit HttpdServer(const INIReader &config);

    void launch();

    virtual  ~HttpdServer();

protected:
    std::string port;
    std::string doc_root;

    int sock = -1;

    void serverListen();
};

#endif // HTTPDSERVER_HPP
