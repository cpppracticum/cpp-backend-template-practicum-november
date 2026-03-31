#include "http_server.h"

#include <iostream>

namespace http_server {

void SessionBase::Run() {
    std::cout << "=== Session Run ===" << std::endl;
    
    http::request<http::string_body> req;
    beast::error_code ec;
    
    http::read(stream_, buffer_, req, ec);
    
    if (ec) {
        std::cerr << "Read error: " << ec.message() << std::endl;
        return;
    }
    
    HandleRequest(std::move(req));
    
    // Продолжаем чтение следующих запросов
    auto self = shared_from_this();
    buffer_.consume(buffer_.size());
    http::async_read(stream_, buffer_, req,
        [self](beast::error_code ec, std::size_t) {
            if (!ec) {
                self->HandleRequest(std::move(req));
            }
        });
}

void SessionBase::Close() {
    std::cout << "=== Close called ===" << std::endl;
    
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
    
    if (ec) {
        std::cerr << "Shutdown error: " << ec.message() << std::endl;
    }
}

} // namespace http_server
