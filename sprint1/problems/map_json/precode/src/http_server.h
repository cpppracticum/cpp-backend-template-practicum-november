#pragma once
#include "sdk.h"

#define BOOST_BEAST_USE_STD_STRING_VIEW

#include <boost/asio/dispatch.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <memory>
#include <string_view>

namespace http_server {

namespace net = boost::asio;
using tcp = net::ip::tcp;
namespace beast = boost::beast;
namespace http = beast::http;

inline void ReportError(beast::error_code ec, std::string_view what) {
    std::cerr << what << ": " << ec.message() << std::endl;
}

class SessionBase : public std::enable_shared_from_this<SessionBase> {
public:
    SessionBase(const SessionBase&) = delete;
    SessionBase& operator=(const SessionBase&) = delete;

    explicit SessionBase(tcp::socket&& socket)
        : stream_(std::move(socket)) {
        std::cout << "=== SessionBase created ===" << std::endl;
    }

    virtual ~SessionBase() = default;

    void Run() {
        std::cout << "=== SessionBase::Run called ===" << std::endl;
        auto self = shared_from_this();
        std::cout << "=== About to post Read ===" << std::endl;
        net::post(stream_.get_executor(), [self]() {
            std::cout << "=== Post lambda called, calling Read ===" << std::endl;
            self->Read();
        });
        std::cout << "=== Post posted ===" << std::endl;
    }

    template <typename Body, typename Fields>
    void Write(http::response<Body, Fields>&& response) {
        std::cout << "=== Write called ===" << std::endl;
        auto safe_response = std::make_shared<http::response<Body, Fields>>(std::move(response));
        auto self = shared_from_this();

        http::async_write(
            stream_, *safe_response,
            [safe_response, self](beast::error_code ec, std::size_t bytes_written) {
                std::cout << "=== async_write completed, ec: " << ec.message() << std::endl;
                self->OnWrite(safe_response->need_eof(), ec, bytes_written);
            });
    }

protected:
    using HttpRequest = http::request<http::string_body>;

    void Close() {
        std::cout << "=== Close called ===" << std::endl;
        beast::error_code ec;
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
    }

private:
    void Read() {
        std::cout << "=== Read: ENTER ===" << std::endl;
        using namespace std::literals;
        request_ = {};
        stream_.expires_after(30s);
        std::cout << "=== Read: before async_read ===" << std::endl;
        
        auto self = shared_from_this();
        http::async_read(
            stream_, buffer_, request_,
            [self](beast::error_code ec, std::size_t bytes_read) {
                self->OnRead(ec, bytes_read);
            });
        std::cout << "=== Read: after async_read ===" << std::endl;
    }

    void OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {
        std::cout << "=== OnRead called, ec: " << ec.message() << std::endl;

        if (ec == http::error::end_of_stream) {
            return Close();
        }
        if (ec) {
            return ReportError(ec, "read");
        }

        HandleRequest(std::move(request_));
    }

    void OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written) {
        std::cout << "=== OnWrite called, close: " << close << std::endl;

        if (ec) {
            return ReportError(ec, "write");
        }

        if (close) {
            return Close();
        }

        Read();
    }

    virtual void HandleRequest(HttpRequest&& request) = 0;

    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    HttpRequest request_;
};

template <typename RequestHandler>
class Session : public SessionBase {
public:
    template <typename Handler>
    Session(tcp::socket&& socket, Handler&& request_handler)
        : SessionBase(std::move(socket))
        , request_handler_(std::forward<Handler>(request_handler)) {
        std::cout << "=== Session created ===" << std::endl;
    }

private:
    void HandleRequest(HttpRequest&& request) override {
        std::cout << "=== Session::HandleRequest called ===" << std::endl;
        request_handler_(std::move(request), [self = this->shared_from_this()](auto&& response) {
            std::cout << "=== Calling Write from handler ===" << std::endl;
            self->Write(std::move(response));
        });
    }

    RequestHandler request_handler_;
};

template <typename RequestHandler>
class Listener : public std::enable_shared_from_this<Listener<RequestHandler>> {
public:
    template <typename Handler>
    Listener(net::io_context& ioc, const tcp::endpoint& endpoint, Handler&& request_handler)
        : ioc_(ioc)
        , acceptor_(net::make_strand(ioc))
        , request_handler_(std::forward<Handler>(request_handler)) {
        std::cout << "=== Listener constructor: binding to " << endpoint << std::endl;
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(net::socket_base::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen(net::socket_base::max_listen_connections);
        std::cout << "=== Listener: listening on port " << endpoint.port() << std::endl;
    }

    void Run() {
        std::cout << "=== Listener::Run called ===" << std::endl;
        DoAccept();
    }

private:
    void DoAccept() {
        std::cout << "=== DoAccept called, waiting for connection ===" << std::endl;
        auto self = this->shared_from_this();
        acceptor_.async_accept(
            net::make_strand(ioc_),
            [self](beast::error_code ec, tcp::socket socket) {
                std::cout << "=== async_accept callback called ===" << std::endl;
                self->OnAccept(ec, std::move(socket));
            });
        std::cout << "=== async_accept posted ===" << std::endl;
    }

    void OnAccept(beast::error_code ec, tcp::socket socket) {
        std::cout << "=== OnAccept called, ec: " << ec.message() << std::endl;

        if (ec) {
            return ReportError(ec, "accept");
        }

        std::cout << "=== New connection accepted, creating session ===" << std::endl;
        std::make_shared<Session<RequestHandler>>(std::move(socket), request_handler_)->Run();

        DoAccept();
    }

    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    RequestHandler request_handler_;
};

template <typename RequestHandler>
void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandler&& handler) {
    std::cout << "=== ServeHttp called ===" << std::endl;
    using MyListener = Listener<std::decay_t<RequestHandler>>;
    std::make_shared<MyListener>(ioc, endpoint, std::forward<RequestHandler>(handler))->Run();
}

}  // namespace http_server
