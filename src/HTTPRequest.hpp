#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>


class HTTPRequest {
public:
    HTTPRequest() = default;

    HTTPRequest(int sock);

    bool bad() const { return _method.empty(); }

    const std::string &method() const { return _method; }

    const std::string &URI() const { return _uri; }

    const std::string &HTTPVer() const { return _HTTPVer; }

private:
    std::string _method, _uri, _HTTPVer;
    std::map<std::string, std::string> _fields;
};


#endif //HTTPREQUEST_HPP
