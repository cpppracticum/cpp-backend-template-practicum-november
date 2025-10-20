#pragma once
#include "sdk.h"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <functional>
#include <memory>

namespace http_server {

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = net::ip::tcp;

using Request = http::request<http::string_body>;
using RequestHandler = std::function<void(Request&&, 
    std::function<void(http::response<http::string_body>&&)>)>;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket&& socket, RequestHandler&& handler);
    void Run();

private:
    void Read();
    void OnRead(beast::error_code ec, std::size_t bytes_transferred);
    void OnWrite(beast::error_code ec, std::size_t bytes_transferred, bool close);

    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    Request req_;
    RequestHandler handler_;
};

class Listener : public std::enable_shared_from_this<Listener> {
public:
    Listener(net::io_context& ioc, tcp::endpoint endpoint, RequestHandler&& handler);
    void Run();

private:
    void DoAccept();
    void OnAccept(beast::error_code ec, tcp::socket socket);

    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    RequestHandler handler_;
};

void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandler&& handler);

}
