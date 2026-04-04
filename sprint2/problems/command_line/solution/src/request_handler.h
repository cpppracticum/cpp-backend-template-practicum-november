#pragma once
#include "http_server.h"
#include "model.h"
#include <filesystem>
#include <functional>

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game, const std::filesystem::path& static_root)
        : game_{game}, static_root_{static_root} {}

    template<typename Request, typename Send>
    void operator()(Request&& req, Send&& send) {
        std::string target(req.target().data(), req.target().size());
        
        if (target.size() >= 4 && target.substr(0, 4) == "/api") {
            auto response = HandleApiRequest(req);
            send(std::move(response));
        } else {
            auto response = ServeStaticFile(target);
            send(std::move(response));
        }
    }

private:
    http::response<http::string_body> HandleApiRequest(const http::request<http::string_body>& req);
    http::response<http::file_body> ServeStaticFile(const std::string& target);
    
    http::response<http::string_body> HandleMapsRequest();
    http::response<http::string_body> HandleMapRequest(const std::string& map_id);
    http::response<http::string_body> MakeErrorResponse(http::status status, const std::string& code, const std::string& message);
    
    std::string UrlDecode(const std::string& encoded);
    std::string GetMimeType(const std::string& path);
    bool IsPathWithinRoot(const std::filesystem::path& path, const std::filesystem::path& root);

    model::Game& game_;
    std::filesystem::path static_root_;
};

}  // namespace http_handler
