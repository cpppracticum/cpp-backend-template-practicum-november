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
        : stream_(std::move(socket)) {}

    virtual ~SessionBase() = default;

    void Run() {
        auto self = shared_from_this();
        net::post(stream_.get_executor(), [self]() {
            self->Read();
        });
    }

    template <typename Body, typename Fields>
    void Write(http::response<Body, Fields>&& response) {
        auto safe_response = std::make_shared<http::response<Body, Fields>>(std::move(response));
        auto self = shared_from_this();

        http::async_write(
            stream_, *safe_response,
            [safe_response, self](beast::error_code ec, std::size_t bytes_written) {
                self->OnWrite(safe_response->need_eof(), ec, bytes_written);
            });
    }

protected:
    using HttpRequest = http::request<http::string_body>;

    void Close() {
        beast::error_code ec;
        stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
    }

private:
    void Read() {
        using namespace std::literals;
        request_ = {};
        stream_.expires_after(30s);
        
        auto self = shared_from_this();
        http::async_read(
            stream_, buffer_, request_,
            [self](beast::error_code ec, std::size_t bytes_read) {
                self->OnRead(ec, bytes_read);
            });
    }

    void OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {
        if (ec == http::error::end_of_stream) {
            return Close();
        }
        if (ec) {
            return ReportError(ec, "read");
        }

        HandleRequest(std::move(request_));
    }

    void OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written) {
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
        , request_handler_(std::forward<Handler>(request_handler)) {}

private:
    void HandleRequest(HttpRequest&& request) override {
        request_handler_(std::move(request), [self = this->shared_from_this()](auto&& response) {
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
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(net::socket_base::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen(net::socket_base::max_listen_connections);
    }

    void Run() {
        DoAccept();
    }

private:
    void DoAccept() {
        auto self = this->shared_from_this();
        acceptor_.async_accept(
            net::make_strand(ioc_),
            [self](beast::error_code ec, tcp::socket socket) {
                self->OnAccept(ec, std::move(socket));
            });
    }

    void OnAccept(beast::error_code ec, tcp::socket socket) {
        if (ec) {
            return ReportError(ec, "accept");
        }

        std::make_shared<Session<RequestHandler>>(std::move(socket), request_handler_)->Run();

        DoAccept();
    }

    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    RequestHandler request_handler_;
};

template <typename RequestHandler>
void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandler&& handler) {
    using MyListener = Listener<std::decay_t<RequestHandler>>;
    std::make_shared<MyListener>(ioc, endpoint, std::forward<RequestHandler>(handler))->Run();
}

}  // namespace http_server
