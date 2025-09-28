#pragma once
#include "http_server.h"
#include "model.h"
#include <boost/beast/http.hpp>
#include <boost/json.hpp>

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send);

private:
    template <typename Body, typename Allocator, typename Send>
    void HandleApiRequest(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send);
    
    template <typename Body, typename Allocator, typename Send>
    void HandleGetMapsList(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send);
    
    template <typename Body, typename Allocator, typename Send>
    void HandleGetMap(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send);

    template<typename Request>
    http::response<http::string_body> MakeErrorResponse(const Request& req, 
                                                       http::status status, 
                                                       std::string code, 
                                                       std::string message);
    
    std::string ExtractMapId(std::string_view path);

    model::Game& game_;
};

}  // namespace http_handler
