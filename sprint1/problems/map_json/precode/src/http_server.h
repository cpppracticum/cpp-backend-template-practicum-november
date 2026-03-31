#pragma once

#include <iostream>
#include <memory>

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace http_server {

// Базовый класс для сессии
class SessionBase : public std::enable_shared_from_this<SessionBase> {
public:
    SessionBase(tcp::socket&& socket) : stream_(std::move(socket)) {}
    virtual ~SessionBase() = default;
    
    void Run();   // Объявление, реализация в .cpp
    void Close(); // Объявление, реализация в .cpp

protected:
    virtual void HandleRequest(http::request<http::string_body>&& req) = 0;
    
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
};

} // namespace http_server
