#ifndef HTTPDSERVER_HPP
#define HTTPDSERVER_HPP

#include <map>

#include "ThreadPool.hpp"

using mimes_t = std::map<std::string, std::string>;

class HttpdServer {
public:
    HttpdServer(std::string port, std::string root, const std::string &mime_path);

    void launch();

    virtual  ~HttpdServer();

    const std::string &docRoot() const { return _doc_root; }

    const mimes_t &mimeTypes() const { return _mime_types; }

protected:
    std::string _port;

    std::string _doc_root;

    mimes_t _mime_types;

    int sock = -1;

    void serverListen();

    ThreadPool pool;
};

#endif // HTTPDSERVER_HPP
