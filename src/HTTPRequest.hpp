#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <string>
#include <map>

using fields_t = std::map<std::string, std::string>;
using field_pair_t = fields_t::value_type;

class HTTPRequest {
public:
    HTTPRequest() = default;

    HTTPRequest(int sock);

    bool bad() const { return _method.empty(); }

    const std::string &method() const { return _method; }

    const std::string &URI() const { return _uri; }

    const std::string &HTTPVer() const { return _HTTPVer; }

    const fields_t &fields() const { return _fields; }

private:
    std::string _method, _uri, _HTTPVer;
    fields_t _fields;
};


#endif //HTTPREQUEST_HPP
