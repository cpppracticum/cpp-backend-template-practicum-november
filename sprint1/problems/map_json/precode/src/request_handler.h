#pragma once
#include "http_server.h"
#include "model.h"
#include <functional>

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    void operator()(http::request<http::string_body>&& req, std::function<void(http::response<http::string_body>&&)>&& send);

private:
    http::response<http::string_body> HandleMapsRequest();
    http::response<http::string_body> HandleMapRequest(const std::string& map_id);
    http::response<http::string_body> MakeErrorResponse(http::status status, const std::string& code, const std::string& message);

    model::Game& game_;
};

}  // namespace http_handler
