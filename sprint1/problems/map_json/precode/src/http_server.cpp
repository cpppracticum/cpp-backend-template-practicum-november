#include "http_server.h"
#include <iostream>

namespace http_server {

Session::Session(tcp::socket&& socket, RequestHandler&& handler)
    : stream_(std::move(socket))
    , handler_(std::move(handler)) {
}

void Session::Run() {
    // Устанавливаем TCP_NODELAY для лучшей производительности
    stream_.socket().set_option(boost::asio::ip::tcp::no_delay(true));
    
    Read();
}

void Session::Read() {
    req_ = {};
    stream_.expires_after(std::chrono::seconds(30));
    
    http::async_read(stream_, buffer_, req_,
        beast::bind_front_handler(&Session::OnRead, shared_from_this()));
}

void Session::OnRead(beast::error_code ec, std::size_t bytes_transferred) {
    if (ec == http::error::end_of_stream) {
        return stream_.socket().close();
    }
    
    if (ec) {
        std::cerr << "Read error: " << ec.message() << std::endl;
        return;
    }
    
    handler_(std::move(req_), [self = shared_from_this()](auto&& response) {
        auto sp = std::make_shared<http::response<http::string_body>>(std::move(response));
        
        http::async_write(self->stream_, *sp,
            beast::bind_front_handler(&Session::OnWrite, self, sp->need_eof()));
    });
}

void Session::OnWrite(beast::error_code ec, std::size_t bytes_transferred, bool close) {
    if (ec) {
        std::cerr << "Write error: " << ec.message() << std::endl;
        return;
    }
    
    if (close) {
        return stream_.socket().close();
    }
    
    Read();
}

Listener::Listener(net::io_context& ioc, tcp::endpoint endpoint, RequestHandler&& handler)
    : ioc_(ioc)
    , acceptor_(net::make_strand(ioc))
    , handler_(std::move(handler)) {
    
    beast::error_code ec;
    
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        throw std::runtime_error("Open error: " + ec.message());
    }
    
    acceptor_.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
        throw std::runtime_error("Set option error: " + ec.message());
    }
    
    acceptor_.bind(endpoint, ec);
    if (ec) {
        throw std::runtime_error("Bind error: " + ec.message());
    }
    
    acceptor_.listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
        throw std::runtime_error("Listen error: " + ec.message());
    }
}

void Listener::Run() {
    DoAccept();
}

void Listener::DoAccept() {
    acceptor_.async_accept(
        net::make_strand(ioc_),
        beast::bind_front_handler(&Listener::OnAccept, shared_from_this()));
}

void Listener::OnAccept(beast::error_code ec, tcp::socket socket) {
    if (ec) {
        std::cerr << "Accept error: " << ec.message() << std::endl;
    } else {
        std::make_shared<Session>(std::move(socket), std::move(handler_))->Run();
    }
    
    DoAccept();
}

void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandler&& handler) {
    std::make_shared<Listener>(ioc, endpoint, std::move(handler))->Run();
}

}  // namespace http_server
