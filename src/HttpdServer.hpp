#ifndef HTTPDSERVER_HPP
#define HTTPDSERVER_HPP

#include "inih/INIReader.h"

using mimes_t = std::map<std::string, std::string>;

class HttpdServer {
public:
    explicit HttpdServer(const INIReader &config);

    void launch();

    virtual  ~HttpdServer();

    const std::string &docRoot() const { return _doc_root; }

    const mimes_t &mimeTypes() const { return _mime_types; }

protected:
    std::string port;

    std::string _doc_root;

    mimes_t _mime_types;

    int sock = -1;

    void serverListen();
};

#endif // HTTPDSERVER_HPP
